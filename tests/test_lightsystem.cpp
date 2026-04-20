#include <catch2/catch_all.hpp>

#include "ECS/Coordinator.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/LightComponent.h"
#include "ECS/Systems/LightSystem.h"
#include "LightManager.h"

// LightSystem sync contract. These tests cover the behaviour every
// consumer relies on:
//   * Rebuild-from-scratch each Update so stale entities drop.
//   * Correct field translation (type, color × energy, range, cone).
//   * Omni shadow slot assignment respects MAX_OMNI_SHADOWS and
//     shadowEnabled gating.
// GL-less by design — LightManager's Init() (which creates SSBOs)
// is skipped; we only exercise the CPU-side array that LightSystem
// mutates.

namespace {

struct Fixture {
    Coordinator coord;
    LightManager lights;
    std::shared_ptr<LightSystem> sys;

    Fixture() {
        coord.Init();
        coord.RegisterComponent<TransformComponent>();
        coord.RegisterComponent<LightComponent>();
        sys = coord.RegisterSystem<LightSystem>();
        Signature sig;
        sig.set(coord.GetComponentType<TransformComponent>());
        sig.set(coord.GetComponentType<LightComponent>());
        coord.SetSystemSignature<LightSystem>(sig);
    }

    Entity SpawnLight(MistLightType t, glm::vec3 pos, bool shadow = true) {
        Entity e = coord.CreateEntity();
        TransformComponent tr; tr.position = pos;
        coord.AddComponent(e, tr);
        LightComponent lc;
        lc.type          = t;
        lc.shadowEnabled = shadow;
        coord.AddComponent(e, lc);
        return e;
    }
};

} // namespace

TEST_CASE("LightSystem rebuilds LightManager array every Update", "[lights]") {
    Fixture f;
    Entity a = f.SpawnLight(MistLightType::Omni, {1, 2, 3});
    Entity b = f.SpawnLight(MistLightType::Spot, {4, 5, 6});

    f.sys->Update(f.coord, f.lights);
    REQUIRE(f.lights.GetLightCount() == 2);

    // Destroy an entity and re-run — the manager should drop it.
    f.coord.DestroyEntity(a);
    f.sys->Update(f.coord, f.lights);
    REQUIRE(f.lights.GetLightCount() == 1);
    REQUIRE(int(f.lights.GetLight(0).position.w) == (int)MistLightType::Spot);

    (void)b;
}

TEST_CASE("LightSystem folds energy into color.w and preserves color.rgb", "[lights]") {
    Fixture f;
    Entity e = f.coord.CreateEntity();
    TransformComponent t; t.position = {0, 0, 0};
    f.coord.AddComponent(e, t);
    LightComponent lc;
    lc.type   = MistLightType::Omni;
    lc.color  = {0.25f, 0.5f, 1.0f};
    lc.energy = 4.0f;
    lc.temperatureK = 0.0f;   // disable tint; plain color
    f.coord.AddComponent(e, lc);

    f.sys->Update(f.coord, f.lights);

    const Light& L = f.lights.GetLight(0);
    REQUIRE(L.color.x == Catch::Approx(0.25f));
    REQUIRE(L.color.y == Catch::Approx(0.5f));
    REQUIRE(L.color.z == Catch::Approx(1.0f));
    REQUIRE(L.color.w == Catch::Approx(4.0f));  // energy in .w
}

TEST_CASE("LightSystem assigns omni shadow slots 0..N-1 in order", "[lights][shadows]") {
    Fixture f;
    Entity a = f.SpawnLight(MistLightType::Omni, {0,0,0}, /*shadow*/true);
    Entity b = f.SpawnLight(MistLightType::Omni, {1,0,0}, /*shadow*/true);
    Entity c = f.SpawnLight(MistLightType::Omni, {2,0,0}, /*shadow*/true);

    f.sys->Update(f.coord, f.lights);

    // Slots are assigned in iteration order, but m_Entities is an
    // unordered_set — so we sort and check.
    std::array<int, 3> slots{
        (int)f.lights.GetLight(0).params.z,
        (int)f.lights.GetLight(1).params.z,
        (int)f.lights.GetLight(2).params.z
    };
    std::sort(slots.begin(), slots.end());
    REQUIRE(slots[0] == 0);
    REQUIRE(slots[1] == 1);
    REQUIRE(slots[2] == 2);
    (void)a; (void)b; (void)c;
}

TEST_CASE("LightSystem respects shadowEnabled gating", "[lights][shadows]") {
    Fixture f;
    Entity lit   = f.SpawnLight(MistLightType::Omni, {0,0,0}, /*shadow*/false);
    Entity shdw  = f.SpawnLight(MistLightType::Omni, {1,0,0}, /*shadow*/true);

    f.sys->Update(f.coord, f.lights);

    // The shadow-disabled entity gets slot -1; the enabled one gets a
    // real slot. We can't rely on iteration order, so scan for both.
    int enabledSlot = -2, disabledSlot = -2;
    for (int i = 0; i < f.lights.GetLightCount(); ++i) {
        int s = (int)f.lights.GetLight(i).params.z;
        if (s == -1) disabledSlot = s;
        else         enabledSlot  = s;
    }
    REQUIRE(disabledSlot == -1);
    REQUIRE(enabledSlot  >= 0);
    REQUIRE(enabledSlot  < 4);   // MAX_OMNI_SHADOWS
    (void)lit; (void)shdw;
}

TEST_CASE("Spot cone angles are cos-encoded for shader dot-product", "[lights][spot]") {
    Fixture f;
    Entity e = f.coord.CreateEntity();
    TransformComponent t; t.position = {0, 0, 0};
    f.coord.AddComponent(e, t);
    LightComponent lc;
    lc.type      = MistLightType::Spot;
    lc.innerCone = 30.0f;   // degrees
    lc.outerCone = 45.0f;
    f.coord.AddComponent(e, lc);

    f.sys->Update(f.coord, f.lights);

    const Light& L = f.lights.GetLight(0);
    // direction.w = cos(innerCone), params.y = cos(outerCone).
    REQUIRE(L.direction.w == Catch::Approx(std::cos(glm::radians(30.0f))));
    REQUIRE(L.params.y    == Catch::Approx(std::cos(glm::radians(45.0f))));
    (void)e;
}
