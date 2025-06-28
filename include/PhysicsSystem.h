
#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

class PhysicsSystem {
public:
    PhysicsSystem();
    ~PhysicsSystem();

    void Update(float deltaTime);

    btRigidBody* CreateGroundPlane(const glm::vec3& position);
    btRigidBody* CreateCube(const glm::vec3& position, float mass);

    void ApplyForce(btRigidBody* body, const glm::vec3& force);
    // Add other physics related methods

private:
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* overlappingPairCache;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* dynamicsWorld;

    // Keep track of allocated collision shapes and motion states
    std::vector<btCollisionShape*> collisionShapes;
    std::vector<btDefaultMotionState*> motionStates;
};

#endif // PHYSICS_SYSTEM_H
