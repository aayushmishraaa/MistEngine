#include "ECS/ComponentArray.h"
#include "ECS/Coordinator.h"

#include <catch2/catch_all.hpp>

// Independent coordinator for ECS tests — we deliberately don't touch the
// global gCoordinator so tests remain isolated and can run in any order.

struct TestPosition {
    float x = 0.f;
    float y = 0.f;
};

struct TestTag {
    int value = 0;
};

TEST_CASE("Coordinator::CreateEntity issues distinct IDs", "[ecs]") {
    // Note on recycling: EntityManager pre-populates a queue with every
    // possible entity ID, so a freshly-destroyed ID goes to the *back* of the
    // pool. Asserting "next CreateEntity returns the destroyed id" doesn't
    // match that design, and forcing recycling would require exhausting
    // MAX_ENTITIES. The property we actually care about here is distinctness.
    Coordinator coord;
    coord.Init();

    Entity a = coord.CreateEntity();
    Entity b = coord.CreateEntity();
    Entity c = coord.CreateEntity();

    REQUIRE(a != b);
    REQUIRE(b != c);
    REQUIRE(a != c);

    coord.DestroyEntity(b);

    // Destroying + re-creating shouldn't collide with still-living IDs.
    for (int i = 0; i < 16; ++i) {
        Entity e = coord.CreateEntity();
        REQUIRE(e != a);
        REQUIRE(e != c);
    }
}

TEST_CASE("ComponentArray removes middle entries without corrupting neighbors", "[ecs]") {
    // Regression guard for the off-by-one in the swap-and-pop removal path.
    ComponentArray<TestPosition> arr;

    arr.InsertData(10, TestPosition{1.f, 1.f});
    arr.InsertData(20, TestPosition{2.f, 2.f});
    arr.InsertData(30, TestPosition{3.f, 3.f});

    REQUIRE(arr.Size() == 3);

    arr.RemoveData(20);

    REQUIRE(arr.Size() == 2);
    REQUIRE(arr.HasData(10));
    REQUIRE_FALSE(arr.HasData(20));
    REQUIRE(arr.HasData(30));

    auto& p10 = arr.GetData(10);
    auto& p30 = arr.GetData(30);
    REQUIRE(p10.x == Catch::Approx(1.f));
    REQUIRE(p30.x == Catch::Approx(3.f));
}

TEST_CASE("ComponentArray::RemoveData is safe on unknown entities", "[ecs]") {
    ComponentArray<TestTag> arr;
    arr.InsertData(1, TestTag{42});
    // Pre-fix, calling RemoveData on an entity that isn't there could still
    // trip up the internal book-keeping on subsequent inserts.
    arr.RemoveData(999);
    REQUIRE(arr.Size() == 1);
    REQUIRE(arr.HasData(1));
    REQUIRE(arr.GetData(1).value == 42);
}

TEST_CASE("AddComponent + RemoveComponent flip the entity's signature bit", "[ecs]") {
    Coordinator coord;
    coord.Init();
    coord.RegisterComponent<TestPosition>();

    Entity e = coord.CreateEntity();
    coord.AddComponent(e, TestPosition{5.f, 6.f});

    REQUIRE(coord.HasComponent<TestPosition>(e));
    REQUIRE(coord.GetComponent<TestPosition>(e).x == Catch::Approx(5.f));

    coord.RemoveComponent<TestPosition>(e);
    REQUIRE_FALSE(coord.HasComponent<TestPosition>(e));
}
