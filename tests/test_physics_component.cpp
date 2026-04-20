#include <catch2/catch_all.hpp>

#include "Core/Reflection.h"
#include "ECS/Components/PhysicsComponent.h"
#include "PhysicsSystem.h"

#include <cstring>
#include <set>
#include <string>

// PhysicsComponent is the authoring surface: every field a user can
// edit in the Inspector must be reflected, and the defaults must be
// sane because fresh `Add Component -> Physics` on an entity inherits
// them. These tests pin both contracts so a future refactor doesn't
// silently drop an Inspector field or flip a default.

TEST_CASE("PhysicsComponent defaults are Godot-parity sane", "[physics]") {
    PhysicsComponent pc;
    REQUIRE(pc.rigidBody       == nullptr);
    REQUIRE(pc.syncTransform   == true);
    REQUIRE(pc.shape           == CollisionShape::Box);
    REQUIRE(pc.halfExtents.x   == Catch::Approx(0.5f));
    REQUIRE(pc.halfExtents.y   == Catch::Approx(0.5f));
    REQUIRE(pc.halfExtents.z   == Catch::Approx(0.5f));
    REQUIRE(pc.radius          == Catch::Approx(0.5f));
    REQUIRE(pc.height          == Catch::Approx(1.0f));
    REQUIRE(pc.mass            == Catch::Approx(1.0f));
    REQUIRE(pc.friction        == Catch::Approx(0.5f));
    REQUIRE(pc.restitution     == Catch::Approx(0.2f));
    REQUIRE(pc.linearDamping   == Catch::Approx(0.0f));
    REQUIRE(pc.angularDamping  == Catch::Approx(0.0f));
    REQUIRE(pc.kinematic       == false);
    REQUIRE(pc.shapeHash       == 0ull);
}

TEST_CASE("PhysicsComponent reflects all authoring fields", "[physics][reflection]") {
    const auto* props = Mist::TypeRegistry::Instance().Get("PhysicsComponent");
    REQUIRE(props != nullptr);

    std::set<std::string> names;
    for (const auto& p : *props) names.emplace(p.name);

    // Every field a user can edit at authoring time must be present.
    // `rigidBody` and `shapeHash` are runtime-only — deliberately NOT
    // reflected so the Inspector can't mutate them.
    REQUIRE(names.count("shape")          == 1);
    REQUIRE(names.count("halfExtents")    == 1);
    REQUIRE(names.count("radius")         == 1);
    REQUIRE(names.count("height")         == 1);
    REQUIRE(names.count("mass")           == 1);
    REQUIRE(names.count("friction")       == 1);
    REQUIRE(names.count("restitution")    == 1);
    REQUIRE(names.count("linearDamping")  == 1);
    REQUIRE(names.count("angularDamping") == 1);
    REQUIRE(names.count("kinematic")      == 1);
    REQUIRE(names.count("syncTransform")  == 1);

    REQUIRE(names.count("rigidBody")      == 0);
    REQUIRE(names.count("shapeHash")      == 0);
}

TEST_CASE("PhysicsSystem::ComputeShapeHash reacts to geometry only", "[physics][hash]") {
    // The hash feeds the rebuild-trigger path: geometry changes (shape
    // kind, halfExtents, radius, height, mass) force a body rebuild;
    // material changes do not.
    PhysicsComponent base;
    const auto h0 = PhysicsSystem::ComputeShapeHash(base);

    SECTION("Identical component hashes identically") {
        PhysicsComponent same;
        REQUIRE(PhysicsSystem::ComputeShapeHash(same) == h0);
    }

    SECTION("Shape kind flip changes hash") {
        PhysicsComponent pc = base;
        pc.shape = CollisionShape::Sphere;
        REQUIRE(PhysicsSystem::ComputeShapeHash(pc) != h0);
    }

    SECTION("halfExtents change changes hash") {
        PhysicsComponent pc = base;
        pc.halfExtents = glm::vec3(1.0f, 0.5f, 0.5f);
        REQUIRE(PhysicsSystem::ComputeShapeHash(pc) != h0);
    }

    SECTION("Mass change changes hash (Bullet inertia depends on mass)") {
        PhysicsComponent pc = base;
        pc.mass = 0.0f;
        REQUIRE(PhysicsSystem::ComputeShapeHash(pc) != h0);
    }

    SECTION("Friction / restitution / damping do NOT change hash") {
        PhysicsComponent pc = base;
        pc.friction       = 1.9f;
        pc.restitution    = 0.95f;
        pc.linearDamping  = 0.5f;
        pc.angularDamping = 0.5f;
        pc.kinematic      = true;
        REQUIRE(PhysicsSystem::ComputeShapeHash(pc) == h0);
    }
}
