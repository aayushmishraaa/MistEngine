#pragma once
#include <bullet/btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <vector>

class PhysicsSystem {
 public:
    PhysicsSystem();
    ~PhysicsSystem();

    void Initialize();
    void Update(float deltaTime);
    void Cleanup();

    // Add rigid body to world
    btRigidBody* CreateRigidBody(float mass, const glm::vec3& position, const glm::vec3& size);

    // Apply force to a rigid body
    void ApplyForce(btRigidBody* body, const glm::vec3& force);
    
    // Convert between Bullet and GLM
    static glm::mat4 BulletToGLM(const btTransform& transform);
    static btTransform GLMToBullet(const glm::mat4& matrix);

private:
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* overlappingPairCache;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* dynamicsWorld;

    std::vector<btRigidBody*> rigidBodies;
    std::vector<btCollisionShape*> collisionShapes;
};