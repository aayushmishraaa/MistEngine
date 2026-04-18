#include <catch2/catch_all.hpp>

#include "ECS/Components/HierarchyComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Coordinator.h"
#include "ECS/Systems/HierarchySystem.h"

#include <glm/glm.hpp>
#include <memory>

// Each test builds its own Coordinator (not the global one) so they stay
// isolated from whatever the main engine would have set up.

TEST_CASE("GetLivingEntities reflects Coordinator create/destroy", "[ecs][hierarchy]") {
    // The "Entity 0" hierarchy bug was caused by the UI iterating a
    // local counter instead of the authoritative living-entity set.
    // This test locks that down: living set must reflect every create
    // (regardless of which path called CreateEntity), and drop on
    // destroy.
    Coordinator c;
    c.Init();
    Entity a = c.CreateEntity();
    Entity b = c.CreateEntity();
    Entity d = c.CreateEntity();

    const auto& live = c.GetLivingEntities();
    REQUIRE(live.size() == 3);
    REQUIRE(live.count(a) == 1);
    REQUIRE(live.count(b) == 1);
    REQUIRE(live.count(d) == 1);

    c.DestroyEntity(b);
    REQUIRE(live.size() == 2);
    REQUIRE(live.count(b) == 0);
}


namespace {
struct HierarchyFixture {
    Coordinator coord;
    std::shared_ptr<HierarchySystem> sys;

    HierarchyFixture() {
        coord.Init();
        coord.RegisterComponent<TransformComponent>();
        coord.RegisterComponent<HierarchyComponent>();
        sys = coord.RegisterSystem<HierarchySystem>();

        Signature sig;
        sig.set(coord.GetComponentType<TransformComponent>());
        sig.set(coord.GetComponentType<HierarchyComponent>());
        coord.SetSystemSignature<HierarchySystem>(sig);
    }

    Entity MakeEntity(glm::vec3 pos = {0, 0, 0}) {
        Entity e = coord.CreateEntity();
        TransformComponent t;
        t.position = pos;
        coord.AddComponent(e, t);
        coord.AddComponent(e, HierarchyComponent{});
        return e;
    }
};
} // namespace

TEST_CASE("HierarchySystem propagates parent translation to child", "[hierarchy]") {
    HierarchyFixture fx;
    Entity parent = fx.MakeEntity({10, 0, 0});
    Entity child  = fx.MakeEntity({0, 5, 0}); // local offset from parent

    REQUIRE(HierarchySystem::Attach(fx.coord, parent, child));
    fx.sys->UpdateTransforms(fx.coord);

    auto& ct = fx.coord.GetComponent<TransformComponent>(child);
    // Child's world position = parent + local = (10, 5, 0).
    REQUIRE(ct.cachedGlobal[3][0] == Catch::Approx(10.0f));
    REQUIRE(ct.cachedGlobal[3][1] == Catch::Approx(5.0f));
}

TEST_CASE("HierarchySystem handles a three-deep chain", "[hierarchy]") {
    HierarchyFixture fx;
    Entity a = fx.MakeEntity({1, 0, 0});
    Entity b = fx.MakeEntity({2, 0, 0});
    Entity c = fx.MakeEntity({4, 0, 0});

    REQUIRE(HierarchySystem::Attach(fx.coord, a, b));
    REQUIRE(HierarchySystem::Attach(fx.coord, b, c));

    fx.sys->UpdateTransforms(fx.coord);

    auto& ct = fx.coord.GetComponent<TransformComponent>(c);
    // Cumulative: 1 + 2 + 4 = 7 on X.
    REQUIRE(ct.cachedGlobal[3][0] == Catch::Approx(7.0f));
}

TEST_CASE("HierarchySystem Detach returns child to root space", "[hierarchy]") {
    HierarchyFixture fx;
    Entity parent = fx.MakeEntity({10, 0, 0});
    Entity child  = fx.MakeEntity({0, 5, 0});

    REQUIRE(HierarchySystem::Attach(fx.coord, parent, child));
    fx.sys->UpdateTransforms(fx.coord);
    REQUIRE(HierarchySystem::Detach(fx.coord, child));
    fx.sys->UpdateTransforms(fx.coord);

    auto& ct = fx.coord.GetComponent<TransformComponent>(child);
    // Back to pure local (0, 5, 0) — no parent offset any more.
    REQUIRE(ct.cachedGlobal[3][0] == Catch::Approx(0.0f));
    REQUIRE(ct.cachedGlobal[3][1] == Catch::Approx(5.0f));
}

TEST_CASE("HierarchySystem OnReady fires post-order", "[hierarchy][signal]") {
    HierarchyFixture fx;
    Entity a = fx.MakeEntity();
    Entity b = fx.MakeEntity();
    Entity c = fx.MakeEntity();
    REQUIRE(HierarchySystem::Attach(fx.coord, a, b));
    REQUIRE(HierarchySystem::Attach(fx.coord, b, c));

    // OnReady is a process-wide singleton; record the emission order for
    // this test and disconnect when done so later tests stay independent.
    std::vector<Entity> fired;
    auto id = HierarchySystem::OnReady().Connect([&](Entity e) { fired.push_back(e); });

    fx.sys->FireReadyCallbacks(fx.coord);

    // Expected post-order: deepest first (c), then parents up (b, a).
    REQUIRE(fired == std::vector<Entity>{c, b, a});

    // Firing again should emit nothing — every entity's readyFired flag
    // was set the first time.
    fired.clear();
    fx.sys->FireReadyCallbacks(fx.coord);
    REQUIRE(fired.empty());

    HierarchySystem::OnReady().Disconnect(id);
}
