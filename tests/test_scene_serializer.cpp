#include <catch2/catch_all.hpp>

#include <nlohmann/json.hpp>

#include <string>

// The old hand-rolled JsonValue parser is gone as of scene v0.5; we now
// delegate to nlohmann/json, which has its own depth + size guards. These
// tests ensure the behaviours we depend on at the library level stay
// stable — full scene round-trip tests will land once AssetRegistry
// mesh loading from on-disk files is wired (currently tests would need
// a live GL context to build a Mesh).

TEST_CASE("nlohmann::json parses a minimal scene shell", "[json]") {
    auto j = nlohmann::json::parse(R"({"version":"0.5","entities":[]})");
    REQUIRE(j.contains("version"));
    REQUIRE(j["version"] == "0.5");
    REQUIRE(j["entities"].is_array());
    REQUIRE(j["entities"].empty());
}

TEST_CASE("nlohmann::json rejects deeply nested input", "[json][security]") {
    // 200 opening brackets — nlohmann's default max depth is 512 but the
    // parser refuses malformed/truncated input regardless. This test
    // documents the behaviour we rely on: the engine surfaces an exception
    // rather than hanging or crashing.
    std::string bomb(200, '[');
    bool threw = false;
    try {
        auto v = nlohmann::json::parse(bomb);
        (void)v;
    } catch (const std::exception&) {
        threw = true;
    }
    REQUIRE(threw);
}

TEST_CASE("nlohmann::json throws on number overflow", "[json]") {
    // nlohmann rejects scientific-notation overflow rather than
    // materialising `inf`. SceneSerializer wraps `in >> root` in a try/catch
    // so this surfaces as a typed error instead of a crash; the test
    // documents the behaviour contract callers rely on.
    bool threw = false;
    try {
        auto j = nlohmann::json::parse(R"({"n": 1e99999})");
        (void)j;
    } catch (const std::exception&) {
        threw = true;
    }
    REQUIRE(threw);
}

// --- Real round-trip -------------------------------------------------------
//
// The note above said full round-trip tests had to wait for a GL context,
// because resolving a mesh ref builds a Mesh (VAO/VBO). That is true for
// RenderComponent — but Transform, Physics, Light and Hierarchy need no GL at
// all, and they cover the four defects that made save/load lossy:
//
//   1. Save walked a dense 0..entityCount range instead of the living set, so
//      Lua-authored scenes (i.e. the default one) saved almost nothing.
//   2. Load never cleared the world, so opening a scene duplicated it.
//   3. Hierarchy parent links were written as raw ids and never remapped
//      against the fresh ids Load allocates.
//   4. PhysicsComponent::shape is an enum, and enums were invisible to
//      reflection, so the field was dropped entirely.

#include "ECS/Components/HierarchyComponent.h"
#include "ECS/Components/LightComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Coordinator.h"
#include "ECS/Systems/HierarchySystem.h"
#include "Scene/SceneSerializer.h"

#include <algorithm>
#include <filesystem>
#include <vector>

// SceneSerializer ignores its `Coordinator&` parameter and operates on the
// global instance, so the tests have to as well.
extern Coordinator gCoordinator;

#include "test_world.h"

namespace {
// Kept as a name the existing cases already call. The body moved to
// tests/test_world.h so every file that drives gCoordinator resets it the
// same way — NameComponent had to join the registration set, and a
// per-file copy meant whichever file reset last decided what was registered.
void ResetGlobalCoordinator() { MistTest::ResetGlobalWorld(); }

Entity MakeNode(glm::vec3 pos) {
    Entity e = gCoordinator.CreateEntity();
    TransformComponent t;
    t.position = pos;
    gCoordinator.AddComponent(e, t);
    gCoordinator.AddComponent(e, HierarchyComponent{});
    return e;
}
} // namespace

TEST_CASE("Scene round-trips entities created outside the UI", "[scene][serializer]") {
    ResetGlobalCoordinator();

    // Three entities, none of which ever bumped UIManager's m_EntityCounter —
    // exactly the situation Lua's spawn_* helpers produce.
    Entity a = MakeNode({1.0f, 2.0f, 3.0f});
    Entity b = MakeNode({4.0f, 5.0f, 6.0f});
    Entity c = MakeNode({7.0f, 8.0f, 9.0f});
    (void)c;

    PhysicsComponent pc;
    pc.shape  = CollisionShape::Capsule;   // non-default, and an enum
    pc.radius = 0.75f;
    pc.mass   = 3.0f;
    gCoordinator.AddComponent(a, pc);

    LightComponent lc;
    lc.type   = MistLightType::Spot;
    lc.energy = 4.5f;
    gCoordinator.AddComponent(b, lc);

    REQUIRE(HierarchySystem::Attach(gCoordinator, a, b));

    const std::string path = "scenes/_test_roundtrip.mist";
    // entityCount is passed as 0 on purpose: the old implementation used it as
    // the iteration bound and would therefore write zero entities.
    REQUIRE(SceneSerializer::Save(path, gCoordinator, 0));

    ResetGlobalCoordinator();
    REQUIRE(gCoordinator.GetLivingEntities().empty());

    int loadedCount = 0;
    REQUIRE(SceneSerializer::Load(path, gCoordinator, loadedCount));

    std::vector<Entity> live(gCoordinator.GetLivingEntities().begin(),
                             gCoordinator.GetLivingEntities().end());
    std::sort(live.begin(), live.end());
    REQUIRE(live.size() == 3);

    // Find the entity carrying physics and check the enum survived.
    int withPhysics = 0;
    for (Entity e : live) {
        if (!gCoordinator.HasComponent<PhysicsComponent>(e)) continue;
        ++withPhysics;
        const auto& p = gCoordinator.GetComponent<PhysicsComponent>(e);
        REQUIRE(p.shape  == CollisionShape::Capsule);
        REQUIRE(p.radius == Catch::Approx(0.75f));
        REQUIRE(p.mass   == Catch::Approx(3.0f));
        REQUIRE(p.rigidBody == nullptr);   // rebuilt by ECSPhysicsSystem
    }
    REQUIRE(withPhysics == 1);

    int withLight = 0;
    for (Entity e : live) {
        if (!gCoordinator.HasComponent<LightComponent>(e)) continue;
        ++withLight;
        const auto& l = gCoordinator.GetComponent<LightComponent>(e);
        REQUIRE(l.type   == MistLightType::Spot);
        REQUIRE(l.energy == Catch::Approx(4.5f));
    }
    REQUIRE(withLight == 1);

    std::filesystem::remove(path);
}

TEST_CASE("Load remaps hierarchy onto the new entity ids", "[scene][serializer][hierarchy]") {
    ResetGlobalCoordinator();
    Entity parent = MakeNode({10.0f, 0.0f, 0.0f});
    Entity child  = MakeNode({0.0f, 5.0f, 0.0f});
    REQUIRE(HierarchySystem::Attach(gCoordinator, parent, child));

    const std::string path = "scenes/_test_hierarchy.mist";
    REQUIRE(SceneSerializer::Save(path, gCoordinator, 0));

    ResetGlobalCoordinator();

    // Offset the id allocator so the ids Load hands out CANNOT coincide with
    // the ones in the file. Without this the test would pass even with no
    // remapping at all, because a fresh coordinator restarts at 0 and would
    // reproduce the saved ids by luck. Destroyed ids go to the back of the
    // free queue, so creating and destroying five entities advances the front.
    for (int i = 0; i < 5; ++i) {
        Entity tmp = gCoordinator.CreateEntity();
        gCoordinator.DestroyEntity(tmp);
    }

    int count = 0;
    REQUIRE(SceneSerializer::Load(path, gCoordinator, count));

    // The ids really did move.
    for (Entity e : gCoordinator.GetLivingEntities()) {
        REQUIRE(e >= 5);
    }

    // Exactly one parent/child pair, and the link points at a LIVE entity —
    // not at whatever id happened to be written in the file.
    int roots = 0, children = 0;
    for (Entity e : gCoordinator.GetLivingEntities()) {
        const auto& h = gCoordinator.GetComponent<HierarchyComponent>(e);
        if (h.parent == HierarchyComponent::kNoParent) {
            ++roots;
            REQUIRE(h.children.size() == 1);
            // Both sides of the relationship agree.
            REQUIRE(gCoordinator.GetComponent<HierarchyComponent>(h.children[0]).parent == e);
        } else {
            ++children;
            REQUIRE(gCoordinator.GetLivingEntities().count(h.parent) == 1);
        }
    }
    REQUIRE(roots == 1);
    REQUIRE(children == 1);

    std::filesystem::remove(path);
}

TEST_CASE("Load replaces the scene instead of appending", "[scene][serializer]") {
    ResetGlobalCoordinator();
    MakeNode({0.0f, 0.0f, 0.0f});
    MakeNode({1.0f, 1.0f, 1.0f});

    const std::string path = "scenes/_test_clear.mist";
    REQUIRE(SceneSerializer::Save(path, gCoordinator, 0));
    REQUIRE(gCoordinator.GetLivingEntities().size() == 2);

    // Load into the SAME populated coordinator. Before the fix this produced
    // four entities — the scene duplicated on top of itself.
    int count = 0;
    REQUIRE(SceneSerializer::Load(path, gCoordinator, count));
    REQUIRE(gCoordinator.GetLivingEntities().size() == 2);

    // And again, to be sure it is idempotent rather than merely off-by-one.
    REQUIRE(SceneSerializer::Load(path, gCoordinator, count));
    REQUIRE(gCoordinator.GetLivingEntities().size() == 2);

    std::filesystem::remove(path);
}

// --- Real round-trip, headless ---------------------------------------------
//
// The note at the top of this file said full round-trip tests had to wait for
// a GL context, because resolving a mesh ref builds a Mesh (VAO/VBO). That is
// true only for RenderComponent. Name, Transform, Physics, Light and Hierarchy
// need no GL at all, and SaveToString/LoadFromString need no filesystem — so
// the serializer is now testable without either.
//
// These cases pin entity identity specifically: before NameComponent existed,
// names lived in UIManager's private map, were never written, and every entity
// came back as "Entity 7" after a reload.

#include "ECS/Components/HierarchyComponent.h"
#include "ECS/Components/NameComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Coordinator.h"
#include "ECS/EntityName.h"
#include "ECS/Systems/HierarchySystem.h"
#include "Scene/SceneSerializer.h"

TEST_CASE("Entity names survive a scene round-trip", "[scene][serializer][identity]") {
    ResetGlobalCoordinator();

    Entity ground = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(ground, TransformComponent{});
    Mist::SetEntityName(gCoordinator, ground, "Ground");

    Entity pillar = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(pillar, TransformComponent{});
    Mist::SetEntityName(gCoordinator, pillar, "Pillar 03");

    const std::string text = SceneSerializer::SaveToString(gCoordinator);
    REQUIRE(text.find("Ground") != std::string::npos);

    int count = 0;
    REQUIRE(SceneSerializer::LoadFromString(text, gCoordinator, count));

    // Ids are remapped on load, so look the entities back up by the thing
    // that is now stable: their names.
    Entity g = Mist::FindEntityByName(gCoordinator, "Ground");
    Entity p = Mist::FindEntityByName(gCoordinator, "Pillar 03");
    REQUIRE(g != Mist::kInvalidEntity);
    REQUIRE(p != Mist::kInvalidEntity);
    REQUIRE(g != p);
    REQUIRE(Mist::EntityName(gCoordinator, g) == "Ground");
}

TEST_CASE("Unnamed entities fall back to a placeholder, not an empty string",
          "[scene][serializer][identity]") {
    ResetGlobalCoordinator();

    Entity e = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(e, TransformComponent{});

    // No NameComponent at all -> placeholder, and nothing is written to the
    // file, so an untitled entity does not bloat the scene with empty names.
    REQUIRE(Mist::EntityName(gCoordinator, e) == "Entity " + std::to_string(e));
    REQUIRE_FALSE(Mist::HasEntityName(gCoordinator, e));

    const std::string text = SceneSerializer::SaveToString(gCoordinator);
    REQUIRE(text.find("\"name\"") == std::string::npos);
}

TEST_CASE("Names survive alongside a reparented hierarchy",
          "[scene][serializer][identity][hierarchy]") {
    ResetGlobalCoordinator();

    Entity parent = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(parent, TransformComponent{});
    gCoordinator.AddComponent(parent, HierarchyComponent{});
    Mist::SetEntityName(gCoordinator, parent, "Turret");

    Entity child = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(child, TransformComponent{});
    gCoordinator.AddComponent(child, HierarchyComponent{});
    Mist::SetEntityName(gCoordinator, child, "Barrel");
    REQUIRE(HierarchySystem::Attach(gCoordinator, parent, child));

    const std::string text = SceneSerializer::SaveToString(gCoordinator);
    int count = 0;
    REQUIRE(SceneSerializer::LoadFromString(text, gCoordinator, count));

    Entity t = Mist::FindEntityByName(gCoordinator, "Turret");
    Entity b = Mist::FindEntityByName(gCoordinator, "Barrel");
    REQUIRE(t != Mist::kInvalidEntity);
    REQUIRE(b != Mist::kInvalidEntity);

    // The parent link is rebuilt through the old->new id remap; the names are
    // what let the test assert it landed on the right entity.
    REQUIRE(gCoordinator.HasComponent<HierarchyComponent>(b));
    REQUIRE(gCoordinator.GetComponent<HierarchyComponent>(b).parent == t);
}

TEST_CASE("Renaming an entity persists across a round-trip",
          "[scene][serializer][identity]") {
    ResetGlobalCoordinator();

    Entity e = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(e, TransformComponent{});
    Mist::SetEntityName(gCoordinator, e, "Cube");
    Mist::SetEntityName(gCoordinator, e, "Player Spawn");

    int count = 0;
    REQUIRE(SceneSerializer::LoadFromString(
        SceneSerializer::SaveToString(gCoordinator), gCoordinator, count));

    REQUIRE(Mist::FindEntityByName(gCoordinator, "Cube") == Mist::kInvalidEntity);
    REQUIRE(Mist::FindEntityByName(gCoordinator, "Player Spawn") != Mist::kInvalidEntity);
}
