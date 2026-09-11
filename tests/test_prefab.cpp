#include <catch2/catch_all.hpp>

#include "Assets/PrefabOverrides.h"
#include "Assets/PrefabSerializer.h"
#include "ECS/Components/HierarchyComponent.h"
#include "ECS/Components/LightComponent.h"
#include "ECS/Components/NameComponent.h"
#include "ECS/Components/PrefabComponents.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/EntityName.h"
#include "ECS/Systems/HierarchySystem.h"
#include "test_world.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

// `.mistprefab` — the engine's first reuse unit.
//
// Everything here is pure serialization plus ECS, so it runs headless. The
// cases are chosen around the two things that make prefab systems rot:
// addressing (an override must find the right child, and keep finding it) and
// propagation (a scene must store a reference, not a flattened copy).

namespace {

// A three-entity subtree: Turret -> Barrel -> Muzzle.
Entity buildTurret() {
    Entity turret = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(turret, TransformComponent{});
    gCoordinator.AddComponent(turret, HierarchyComponent{});
    Mist::SetEntityName(gCoordinator, turret, "Turret");

    Entity barrel = gCoordinator.CreateEntity();
    TransformComponent bt;
    bt.position = {0.0f, 2.0f, 0.0f};
    gCoordinator.AddComponent(barrel, bt);
    gCoordinator.AddComponent(barrel, HierarchyComponent{});
    Mist::SetEntityName(gCoordinator, barrel, "Barrel");
    HierarchySystem::Attach(gCoordinator, turret, barrel);

    Entity muzzle = gCoordinator.CreateEntity();
    TransformComponent mt;
    mt.position = {0.0f, 0.0f, 3.0f};
    gCoordinator.AddComponent(muzzle, mt);
    gCoordinator.AddComponent(muzzle, HierarchyComponent{});
    Mist::SetEntityName(gCoordinator, muzzle, "Muzzle");
    HierarchySystem::Attach(gCoordinator, barrel, muzzle);

    return turret;
}

Entity childNamed(Entity parent, const char* name) {
    if (!gCoordinator.HasComponent<HierarchyComponent>(parent)) return Mist::kInvalidEntity;
    for (Entity c : gCoordinator.GetComponent<HierarchyComponent>(parent).children) {
        if (Mist::EntityName(gCoordinator, c) == name) return c;
    }
    return Mist::kInvalidEntity;
}

} // namespace

TEST_CASE("A prefab round-trips a three-entity subtree", "[prefab]") {
    MistTest::ResetGlobalWorld();

    Entity turret = buildTurret();

    bool ok = false;
    const std::string text =
        Mist::Assets::PrefabSerializer::SaveToString(turret, gCoordinator, &ok);
    REQUIRE(ok);

    // No entity ids anywhere: a prefab is position-independent, and ids must
    // never leak between assets.
    REQUIRE(text.find("\"id\"") == std::string::npos);
    REQUIRE(text.find("Turret") != std::string::npos);
    REQUIRE(text.find("Muzzle") != std::string::npos);

    Entity inst =
        Mist::Assets::PrefabSerializer::InstantiateFromString(text, gCoordinator, "t.mistprefab");
    REQUIRE(inst != Mist::kInvalidEntity);
    REQUIRE(inst != turret);

    Entity barrel = childNamed(inst, "Barrel");
    REQUIRE(barrel != Mist::kInvalidEntity);
    Entity muzzle = childNamed(barrel, "Muzzle");
    REQUIRE(muzzle != Mist::kInvalidEntity);

    REQUIRE(gCoordinator.GetComponent<TransformComponent>(barrel).position.y
            == Catch::Approx(2.0f));
    REQUIRE(gCoordinator.GetComponent<TransformComponent>(muzzle).position.z
            == Catch::Approx(3.0f));
}

TEST_CASE("Instances are independent of each other", "[prefab]") {
    // The headline promise: instance four, move one, the others do not care.
    MistTest::ResetGlobalWorld();

    Entity turret = buildTurret();
    bool ok = false;
    const std::string text =
        Mist::Assets::PrefabSerializer::SaveToString(turret, gCoordinator, &ok);
    REQUIRE(ok);

    std::vector<Entity> instances;
    for (int i = 0; i < 4; ++i) {
        Entity e = Mist::Assets::PrefabSerializer::InstantiateFromString(
            text, gCoordinator, "t.mistprefab");
        REQUIRE(e != Mist::kInvalidEntity);
        instances.push_back(e);
    }

    gCoordinator.GetComponent<TransformComponent>(instances[0]).position = {99.0f, 0.0f, 0.0f};

    for (std::size_t i = 1; i < instances.size(); ++i) {
        INFO("instance " << i);
        REQUIRE(gCoordinator.GetComponent<TransformComponent>(instances[i]).position.x
                == Catch::Approx(0.0f));
    }

    // Each instance got its own distinct children, not shared ones.
    Entity b0 = childNamed(instances[0], "Barrel");
    Entity b1 = childNamed(instances[1], "Barrel");
    REQUIRE(b0 != Mist::kInvalidEntity);
    REQUIRE(b1 != Mist::kInvalidEntity);
    REQUIRE(b0 != b1);
}

TEST_CASE("Saving refuses a subtree with duplicate sibling names", "[prefab]") {
    // Has to be fatal. Overrides are addressed by name path, so two siblings
    // called "Barrel" make "Turret/Barrel" ambiguous and an override would
    // silently land on whichever child came first in iteration order. Godot
    // enforces the same rule.
    MistTest::ResetGlobalWorld();

    Entity root = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(root, TransformComponent{});
    gCoordinator.AddComponent(root, HierarchyComponent{});
    Mist::SetEntityName(gCoordinator, root, "Turret");

    for (int i = 0; i < 2; ++i) {
        Entity c = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(c, TransformComponent{});
        gCoordinator.AddComponent(c, HierarchyComponent{});
        Mist::SetEntityName(gCoordinator, c, "Barrel");
        HierarchySystem::Attach(gCoordinator, root, c);
    }

    bool ok = true;
    const std::string text =
        Mist::Assets::PrefabSerializer::SaveToString(root, gCoordinator, &ok);
    REQUIRE_FALSE(ok);
    REQUIRE(text.empty());
}

TEST_CASE("Every spawned entity carries a membership marker", "[prefab]") {
    // SceneSerializer relies on this to skip expanded content, so a gap here
    // means a prefab gets flattened into the scene file.
    MistTest::ResetGlobalWorld();

    Entity turret = buildTurret();
    bool ok = false;
    const std::string text =
        Mist::Assets::PrefabSerializer::SaveToString(turret, gCoordinator, &ok);
    REQUIRE(ok);

    Entity inst =
        Mist::Assets::PrefabSerializer::InstantiateFromString(text, gCoordinator, "t.mistprefab");
    REQUIRE(inst != Mist::kInvalidEntity);

    Entity barrel = childNamed(inst, "Barrel");
    Entity muzzle = childNamed(barrel, "Muzzle");

    REQUIRE(gCoordinator.HasComponent<PrefabMemberComponent>(inst));
    REQUIRE(gCoordinator.HasComponent<PrefabMemberComponent>(barrel));
    REQUIRE(gCoordinator.HasComponent<PrefabMemberComponent>(muzzle));

    // Local paths are relative to the instance root, which is "".
    REQUIRE(gCoordinator.GetComponent<PrefabMemberComponent>(inst).localPath == "");
    REQUIRE(gCoordinator.GetComponent<PrefabMemberComponent>(barrel).localPath == "Barrel");
    REQUIRE(gCoordinator.GetComponent<PrefabMemberComponent>(muzzle).localPath == "Barrel/Muzzle");

    // All three point at the same instance root.
    REQUIRE(gCoordinator.GetComponent<PrefabMemberComponent>(muzzle).instanceRoot == inst);

    // Only the root carries the instance reference.
    REQUIRE(gCoordinator.HasComponent<PrefabInstanceComponent>(inst));
    REQUIRE_FALSE(gCoordinator.HasComponent<PrefabInstanceComponent>(barrel));
}

TEST_CASE("An override applies to the addressed child only", "[prefab][overrides]") {
    MistTest::ResetGlobalWorld();

    Entity turret = buildTurret();
    bool ok = false;
    const std::string text =
        Mist::Assets::PrefabSerializer::SaveToString(turret, gCoordinator, &ok);
    REQUIRE(ok);

    Entity inst =
        Mist::Assets::PrefabSerializer::InstantiateFromString(text, gCoordinator, "t.mistprefab");
    REQUIRE(inst != Mist::kInvalidEntity);

    const std::string overrides = R"([
      {"target": "Barrel", "component": "TransformComponent",
       "fields": {"position": [7.0, 8.0, 9.0]}}
    ])";

    REQUIRE(Mist::Assets::ApplyPrefabOverrides(inst, gCoordinator, overrides) == 1);

    Entity barrel = childNamed(inst, "Barrel");
    Entity muzzle = childNamed(barrel, "Muzzle");

    REQUIRE(gCoordinator.GetComponent<TransformComponent>(barrel).position.x
            == Catch::Approx(7.0f));
    // The sibling path and the root are untouched.
    REQUIRE(gCoordinator.GetComponent<TransformComponent>(muzzle).position.z
            == Catch::Approx(3.0f));
    REQUIRE(gCoordinator.GetComponent<TransformComponent>(inst).position.x
            == Catch::Approx(0.0f));
}

TEST_CASE("An override leaves unlisted fields at the prefab's values",
          "[prefab][overrides]") {
    // Overrides are PARTIAL: listing `position` must not reset rotation and
    // scale to defaults. This reuses ReadFields' missing-key semantics rather
    // than reimplementing them, and this case pins that.
    MistTest::ResetGlobalWorld();

    Entity root = gCoordinator.CreateEntity();
    TransformComponent t;
    t.position = {1.0f, 1.0f, 1.0f};
    t.scale    = {5.0f, 5.0f, 5.0f};
    gCoordinator.AddComponent(root, t);
    gCoordinator.AddComponent(root, HierarchyComponent{});
    Mist::SetEntityName(gCoordinator, root, "Thing");

    bool ok = false;
    const std::string text =
        Mist::Assets::PrefabSerializer::SaveToString(root, gCoordinator, &ok);
    REQUIRE(ok);

    Entity inst =
        Mist::Assets::PrefabSerializer::InstantiateFromString(text, gCoordinator, "t.mistprefab");
    REQUIRE(inst != Mist::kInvalidEntity);

    const std::string overrides =
        R"([{"target": "", "component": "TransformComponent",
             "fields": {"position": [0.0, 0.0, 0.0]}}])";
    REQUIRE(Mist::Assets::ApplyPrefabOverrides(inst, gCoordinator, overrides) == 1);

    const auto& got = gCoordinator.GetComponent<TransformComponent>(inst);
    REQUIRE(got.position.x == Catch::Approx(0.0f));
    REQUIRE(got.scale.x    == Catch::Approx(5.0f));  // NOT reset to 1
}

TEST_CASE("An override targeting a missing child is dropped, not fatal",
          "[prefab][overrides]") {
    // Editing a prefab must never be a scene-breaking operation. If the source
    // loses a child, the override addressed at it is discarded and everything
    // else still applies.
    MistTest::ResetGlobalWorld();

    Entity turret = buildTurret();
    bool ok = false;
    const std::string text =
        Mist::Assets::PrefabSerializer::SaveToString(turret, gCoordinator, &ok);
    REQUIRE(ok);

    Entity inst =
        Mist::Assets::PrefabSerializer::InstantiateFromString(text, gCoordinator, "t.mistprefab");
    REQUIRE(inst != Mist::kInvalidEntity);

    const std::string overrides = R"([
      {"target": "GoneAway", "component": "TransformComponent",
       "fields": {"position": [1.0, 1.0, 1.0]}},
      {"target": "Barrel", "component": "TransformComponent",
       "fields": {"position": [4.0, 0.0, 0.0]}}
    ])";

    // One of two applies; the missing target is skipped with a warning.
    REQUIRE(Mist::Assets::ApplyPrefabOverrides(inst, gCoordinator, overrides) == 1);
    REQUIRE(gCoordinator.GetComponent<TransformComponent>(childNamed(inst, "Barrel")).position.x
            == Catch::Approx(4.0f));
}

TEST_CASE("A malformed override table is survivable", "[prefab][overrides]") {
    MistTest::ResetGlobalWorld();

    Entity root = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(root, TransformComponent{});
    gCoordinator.AddComponent(root, HierarchyComponent{});
    Mist::SetEntityName(gCoordinator, root, "Thing");

    bool ok = false;
    Entity inst = Mist::Assets::PrefabSerializer::InstantiateFromString(
        Mist::Assets::PrefabSerializer::SaveToString(root, gCoordinator, &ok),
        gCoordinator, "t.mistprefab");
    REQUIRE(ok);
    REQUIRE(inst != Mist::kInvalidEntity);

    REQUIRE(Mist::Assets::ApplyPrefabOverrides(inst, gCoordinator, "not json at all") == 0);
    REQUIRE(Mist::Assets::ApplyPrefabOverrides(inst, gCoordinator, "") == 0);
    REQUIRE(Mist::Assets::ApplyPrefabOverrides(inst, gCoordinator, "{}") == 0);
}

TEST_CASE("Recording and clearing overrides round-trips", "[prefab][overrides]") {
    MistTest::ResetGlobalWorld();

    TransformComponent t;
    t.position = {3.0f, 4.0f, 5.0f};
    t.scale    = {2.0f, 2.0f, 2.0f};

    std::string table = "[]";
    table = Mist::Assets::RecordPrefabOverride(table, "Barrel", "TransformComponent",
                                               "position", &t);
    auto list = Mist::Assets::ListPrefabOverrides(table);
    REQUIRE(list.size() == 1);
    REQUIRE(list[0].target    == "Barrel");
    REQUIRE(list[0].component == "TransformComponent");
    REQUIRE(list[0].field     == "position");

    // A second field on the same (target, component) merges into the existing
    // group rather than appending a duplicate entry.
    table = Mist::Assets::RecordPrefabOverride(table, "Barrel", "TransformComponent",
                                               "scale", &t);
    list = Mist::Assets::ListPrefabOverrides(table);
    REQUIRE(list.size() == 2);
    REQUIRE(nlohmann::json::parse(table).size() == 1);

    // Reverting one field leaves the other.
    table = Mist::Assets::ClearPrefabOverride(table, "Barrel", "TransformComponent", "position");
    list = Mist::Assets::ListPrefabOverrides(table);
    REQUIRE(list.size() == 1);
    REQUIRE(list[0].field == "scale");

    // Reverting the last one prunes the group rather than leaving a husk.
    table = Mist::Assets::ClearPrefabOverride(table, "Barrel", "TransformComponent", "scale");
    REQUIRE(Mist::Assets::ListPrefabOverrides(table).empty());
    REQUIRE(nlohmann::json::parse(table).empty());
}

TEST_CASE("Recording an unknown field is a no-op", "[prefab][overrides]") {
    TransformComponent t;
    const std::string before = "[]";
    const std::string after  = Mist::Assets::RecordPrefabOverride(
        before, "", "TransformComponent", "noSuchField", &t);
    REQUIRE(after == before);
}

// --- Scene integration ------------------------------------------------------
//
// These go through real files, because propagation is only observable across a
// save/reload cycle and the point of the design is that the scene holds a
// reference rather than a copy.

#include "Scene/SceneSerializer.h"

#include <filesystem>
#include <fstream>

namespace {

// PathGuard resolves prefab paths under the project root, which defaults to
// the current working directory. ctest runs from the build tree, so a relative
// path here lands somewhere writable and is cleaned up after.
struct ScopedPrefabFile {
    std::filesystem::path path;

    explicit ScopedPrefabFile(const std::string& rel, const std::string& contents)
        : path(std::filesystem::current_path() / rel) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << contents;
    }
    ~ScopedPrefabFile() {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    std::string rel() const {
        return std::filesystem::relative(path, std::filesystem::current_path()).generic_string();
    }
};

} // namespace

TEST_CASE("A scene stores a prefab reference, not the expanded subtree",
          "[prefab][scene]") {
    MistTest::ResetGlobalWorld();

    Entity turret = buildTurret();
    bool ok = false;
    ScopedPrefabFile file("prefabs/test_turret.mistprefab",
                          Mist::Assets::PrefabSerializer::SaveToString(turret, gCoordinator, &ok));
    REQUIRE(ok);

    MistTest::ResetGlobalWorld();
    Entity inst = Mist::Assets::PrefabSerializer::Instantiate(file.rel(), gCoordinator);
    REQUIRE(inst != Mist::kInvalidEntity);

    const std::string sceneText = SceneSerializer::SaveToString(gCoordinator);

    // Three entities exist, but only the instance root is written, and its
    // children's names must not appear — that is what "by reference" means.
    const auto parsed = nlohmann::json::parse(sceneText);
    REQUIRE(parsed["entities"].size() == 1);
    REQUIRE(parsed["entities"][0].contains("prefab"));
    REQUIRE(sceneText.find("Muzzle") == std::string::npos);
}

TEST_CASE("A prefab edit propagates to instances on scene reload",
          "[prefab][scene]") {
    // The entire payoff of storing a reference. Flattening the subtree into the
    // scene would make a prefab a one-time copy, which "Duplicate" already was.
    MistTest::ResetGlobalWorld();

    Entity turret = buildTurret();
    bool ok = false;
    std::string prefabText =
        Mist::Assets::PrefabSerializer::SaveToString(turret, gCoordinator, &ok);
    REQUIRE(ok);

    ScopedPrefabFile file("prefabs/test_propagate.mistprefab", prefabText);

    MistTest::ResetGlobalWorld();
    Entity inst = Mist::Assets::PrefabSerializer::Instantiate(file.rel(), gCoordinator);
    REQUIRE(inst != Mist::kInvalidEntity);
    REQUIRE(gCoordinator.GetComponent<TransformComponent>(childNamed(inst, "Barrel")).position.y
            == Catch::Approx(2.0f));

    const std::string sceneText = SceneSerializer::SaveToString(gCoordinator);

    // Edit the SOURCE prefab: move Barrel from y=2 to y=42.
    {
        auto doc = nlohmann::json::parse(prefabText);
        doc["root"]["children"][0]["transform"]["pos"] = nlohmann::json::array({0.0, 42.0, 0.0});
        std::ofstream out(file.path, std::ios::binary | std::ios::trunc);
        out << doc.dump(2);
    }

    int count = 0;
    REQUIRE(SceneSerializer::LoadFromString(sceneText, gCoordinator, count));

    Entity reloaded = Mist::kInvalidEntity;
    for (Entity e : gCoordinator.GetLivingEntities()) {
        if (gCoordinator.HasComponent<PrefabInstanceComponent>(e)) { reloaded = e; break; }
    }
    REQUIRE(reloaded != Mist::kInvalidEntity);
    REQUIRE(gCoordinator.GetComponent<TransformComponent>(childNamed(reloaded, "Barrel")).position.y
            == Catch::Approx(42.0f));
}

TEST_CASE("An overridden field survives propagation", "[prefab][scene][overrides]") {
    // Godot's rule: the source updates everything EXCEPT what the instance
    // overrode. An override that gets clobbered by propagation is worse than
    // no override at all, because the user's edit silently disappears.
    MistTest::ResetGlobalWorld();

    Entity turret = buildTurret();
    bool ok = false;
    std::string prefabText =
        Mist::Assets::PrefabSerializer::SaveToString(turret, gCoordinator, &ok);
    REQUIRE(ok);

    ScopedPrefabFile file("prefabs/test_override.mistprefab", prefabText);

    MistTest::ResetGlobalWorld();
    Entity inst = Mist::Assets::PrefabSerializer::Instantiate(file.rel(), gCoordinator);
    REQUIRE(inst != Mist::kInvalidEntity);

    // Override Barrel's position on this instance.
    Entity barrel = childNamed(inst, "Barrel");
    auto& bt = gCoordinator.GetComponent<TransformComponent>(barrel);
    bt.position = {0.0f, 7.0f, 0.0f};
    gCoordinator.GetComponent<PrefabInstanceComponent>(inst).overridesJson =
        Mist::Assets::RecordPrefabOverride("[]", "Barrel", "TransformComponent",
                                           "position", &bt);

    const std::string sceneText = SceneSerializer::SaveToString(gCoordinator);
    REQUIRE(sceneText.find("overrides") != std::string::npos);

    // Now move Barrel in the source.
    {
        auto doc = nlohmann::json::parse(prefabText);
        doc["root"]["children"][0]["transform"]["pos"] = nlohmann::json::array({0.0, 99.0, 0.0});
        std::ofstream out(file.path, std::ios::binary | std::ios::trunc);
        out << doc.dump(2);
    }

    int count = 0;
    REQUIRE(SceneSerializer::LoadFromString(sceneText, gCoordinator, count));

    Entity reloaded = Mist::kInvalidEntity;
    for (Entity e : gCoordinator.GetLivingEntities()) {
        if (gCoordinator.HasComponent<PrefabInstanceComponent>(e)) { reloaded = e; break; }
    }
    REQUIRE(reloaded != Mist::kInvalidEntity);

    // The override wins over the source's new value.
    REQUIRE(gCoordinator.GetComponent<TransformComponent>(childNamed(reloaded, "Barrel")).position.y
            == Catch::Approx(7.0f));
}

TEST_CASE("A missing prefab file does not take the scene down with it",
          "[prefab][scene]") {
    MistTest::ResetGlobalWorld();

    // One normal entity plus a reference to a prefab that does not exist.
    const std::string sceneText = R"({
      "version": "1.0",
      "entities": [
        {"id": 1, "name": "Ground", "transform": {"pos":[0,0,0],"rot":[0,0,0],"scale":[1,1,1]}},
        {"id": 2, "name": "Ghost", "prefab": {"path": "prefabs/does_not_exist.mistprefab",
                                              "overrides": []}}
      ]
    })";

    int count = 0;
    REQUIRE(SceneSerializer::LoadFromString(sceneText, gCoordinator, count));

    // The good entity survived; the broken instance was skipped.
    REQUIRE(Mist::FindEntityByName(gCoordinator, "Ground") != Mist::kInvalidEntity);
    REQUIRE(Mist::FindEntityByName(gCoordinator, "Ghost")  == Mist::kInvalidEntity);
}

TEST_CASE("A prefab path escaping the project root is refused", "[prefab][security]") {
    MistTest::ResetGlobalWorld();

    Entity root = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(root, TransformComponent{});
    gCoordinator.AddComponent(root, HierarchyComponent{});
    Mist::SetEntityName(gCoordinator, root, "Thing");

    // Guarded from the first commit rather than retrofitted: materials and
    // packages both had to be fixed after the fact at commit 04b00b4.
    REQUIRE_FALSE(Mist::Assets::PrefabSerializer::Save(
        root, gCoordinator, "../../escaped.mistprefab"));
    REQUIRE_FALSE(Mist::Assets::PrefabSerializer::Save(
        root, gCoordinator, "/tmp/escaped.mistprefab"));

    REQUIRE(Mist::Assets::PrefabSerializer::Instantiate("../../escaped.mistprefab", gCoordinator)
            == Mist::kInvalidEntity);
}
