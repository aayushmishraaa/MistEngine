#ifndef PHYSICSCOMPONENT_H
#define PHYSICSCOMPONENT_H

#include "Core/Reflection.h"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <cstdint>

enum class CollisionShape : uint8_t {
    Box = 0,
    Sphere,
    Capsule,
    StaticPlane,
};

struct PhysicsComponent {
    // Runtime-owned Bullet handle. Rebuilt by ECSPhysicsSystem when
    // the shape params hash changes. Never reflected / serialized.
    btRigidBody* rigidBody = nullptr;

    bool syncTransform = true;

    // --- Shape params (authoritative; body rebuilt when these change) ---
    CollisionShape shape = CollisionShape::Box;
    glm::vec3      halfExtents{0.5f, 0.5f, 0.5f};  // Box
    float          radius = 0.5f;                   // Sphere / Capsule
    float          height = 1.0f;                   // Capsule (cylinder height, excluding caps)

    // --- Material params (applied live each tick, no rebuild) ---
    float mass           = 1.0f;   // 0 -> static body
    float friction       = 0.5f;
    float restitution    = 0.2f;
    float linearDamping  = 0.0f;
    float angularDamping = 0.0f;
    bool  kinematic      = false;  // flips CF_KINEMATIC_OBJECT, skips gravity

    // Hash of shape params from the last rebuild. ECSPhysicsSystem
    // recomputes this each frame and rebuilds iff it changed — so
    // sliders dragging identical values don't thrash the body.
    std::uint64_t shapeHash = 0;
};

MIST_REFLECT(PhysicsComponent)
    MIST_FIELD(PhysicsComponent, shape,          ::Mist::PropertyHint::None,  "")
    MIST_FIELD(PhysicsComponent, halfExtents,    ::Mist::PropertyHint::None,  "")
    MIST_FIELD(PhysicsComponent, radius,         ::Mist::PropertyHint::Range, "0.05,10.0")
    MIST_FIELD(PhysicsComponent, height,         ::Mist::PropertyHint::Range, "0.05,10.0")
    MIST_FIELD(PhysicsComponent, mass,           ::Mist::PropertyHint::Range, "0,1000")
    MIST_FIELD(PhysicsComponent, friction,       ::Mist::PropertyHint::Range, "0,2")
    MIST_FIELD(PhysicsComponent, restitution,    ::Mist::PropertyHint::Range, "0,1")
    MIST_FIELD(PhysicsComponent, linearDamping,  ::Mist::PropertyHint::Range, "0,1")
    MIST_FIELD(PhysicsComponent, angularDamping, ::Mist::PropertyHint::Range, "0,1")
    MIST_FIELD(PhysicsComponent, kinematic,      ::Mist::PropertyHint::None,  "")
    MIST_FIELD(PhysicsComponent, syncTransform,  ::Mist::PropertyHint::None,  "")
MIST_REFLECT_END(PhysicsComponent)

#endif // PHYSICSCOMPONENT_H
