#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include "Physics/BulletOwners.h"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class PhysicsSystem {
public:
    PhysicsSystem();
    ~PhysicsSystem();

    // Non-copyable. The dynamics world + shape/body vectors below would
    // break horribly under a shallow copy.
    PhysicsSystem(const PhysicsSystem&)            = delete;
    PhysicsSystem& operator=(const PhysicsSystem&) = delete;

    void Update(float deltaTime);

    // Return non-owning handles. Bodies stay alive until this PhysicsSystem
    // is destroyed, at which point ScopedRigidBody removes each body from
    // the world before freeing its memory + motion state + collision shape.
    btRigidBody* CreateGroundPlane(const glm::vec3& position);
    btRigidBody* CreateCube(const glm::vec3& position, float mass);
    btRigidBody* CreateSphere(const glm::vec3& position, float radius, float mass);

    void ApplyForce(btRigidBody* body, const glm::vec3& force);

private:
    // Destruction order matters: bodies (which dereference the dispatcher via
    // the world on removal) must die before the world, and the world before
    // the solver/broadphase/dispatcher/collision-config. unique_ptrs
    // declared top-to-bottom are destroyed bottom-to-top, which is exactly
    // the Bullet-required order.
    std::unique_ptr<btDefaultCollisionConfiguration>    m_CollisionConfiguration;
    std::unique_ptr<btCollisionDispatcher>              m_Dispatcher;
    std::unique_ptr<btBroadphaseInterface>              m_Broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_Solver;
    std::unique_ptr<btDiscreteDynamicsWorld>            m_DynamicsWorld;

    std::vector<Mist::Physics::ScopedCollisionShape> m_Shapes;
    std::vector<Mist::Physics::ScopedRigidBody>      m_Bodies;

    btRigidBody* addBody(Mist::Physics::ScopedCollisionShape shape,
                         const glm::vec3& position,
                         float mass);
};

#endif // PHYSICS_SYSTEM_H
