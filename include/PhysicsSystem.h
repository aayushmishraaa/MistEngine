#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include "Physics/BulletOwners.h"
#include "ECS/Components/PhysicsComponent.h"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <memory>
#include <vector>

class PhysicsSystem {
public:
    PhysicsSystem();
    ~PhysicsSystem();

    PhysicsSystem(const PhysicsSystem&)            = delete;
    PhysicsSystem& operator=(const PhysicsSystem&) = delete;

    void Update(float deltaTime);

    // --- Godot-style authoring API ---
    // Computes shape param hash and, if it differs from pc.shapeHash,
    // destroys the old body and builds a new one from the component's
    // fields. Always applies material params (friction, restitution,
    // damping, kinematic flag) live. Called every frame by
    // ECSPhysicsSystem for every physics entity.
    void EnsureBody(const glm::vec3& position, PhysicsComponent& pc);

    // Primitive factory — allocates a raw Bullet shape. Caller owns.
    btCollisionShape* CreateShape(CollisionShape s,
                                  const glm::vec3& halfExtents,
                                  float radius,
                                  float height);

    // One-shot: build a body from the component + position. Tracks
    // the body/shape in internal vectors and returns a non-owning
    // pointer. Caller stores it in pc.rigidBody.
    btRigidBody* CreateBodyFromComponent(const glm::vec3& position,
                                         const PhysicsComponent& pc);

    // Remove a body (and its paired shape) from the world. After
    // this, `body` is a dangling pointer — callers must null their
    // own references.
    void DestroyBody(btRigidBody* body);

    // --- Legacy helpers (thin wrappers over the factory path) ---
    btRigidBody* CreateGroundPlane(const glm::vec3& position);
    btRigidBody* CreateCube(const glm::vec3& position, float mass);
    btRigidBody* CreateSphere(const glm::vec3& position, float radius, float mass);

    void ApplyForce(btRigidBody* body, const glm::vec3& force);

    // Hash of the geometry params that require a body rebuild. Pure
    // function, exposed so tests can verify the rebuild trigger.
    static std::uint64_t ComputeShapeHash(const PhysicsComponent& pc);

private:
    // Destruction order matters — see implementation note.
    std::unique_ptr<btDefaultCollisionConfiguration>     m_CollisionConfiguration;
    std::unique_ptr<btCollisionDispatcher>               m_Dispatcher;
    std::unique_ptr<btBroadphaseInterface>               m_Broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_Solver;
    std::unique_ptr<btDiscreteDynamicsWorld>             m_DynamicsWorld;

    // Parallel vectors: m_Shapes[i] is the collision shape held by the
    // body at m_Bodies[i]. Entries are inserted and erased in lockstep.
    std::vector<Mist::Physics::ScopedCollisionShape> m_Shapes;
    std::vector<Mist::Physics::ScopedRigidBody>      m_Bodies;

    btRigidBody* addBody(Mist::Physics::ScopedCollisionShape shape,
                         const glm::vec3& position,
                         float mass);

    // Apply the material fields (friction / restitution / damping /
    // kinematic) to an already-built body without rebuilding. Cheap.
    static void ApplyMaterial(btRigidBody* body, const PhysicsComponent& pc);
};

#endif // PHYSICS_SYSTEM_H
