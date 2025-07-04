
#ifndef SYSTEMS_H
#define SYSTEMS_H

#include "ECSManager.h"
#include "Components.h"
#include "Renderer.h"
#include "Scene.h"

// Base System class
class System {
public:
    virtual ~System() = default;
    virtual void Update(ECSManager& ecs, float deltaTime) = 0;
};

// Render System
class RenderSystem : public System {
public:
    RenderSystem(Renderer& renderer);
    void Update(ECSManager& ecs, float deltaTime) override;

private:
    Renderer& renderer;
};

// Physics System
class PhysicsSystem : public System {
public:
    PhysicsSystem();
    ~PhysicsSystem();

    void Update(ECSManager& ecs, float deltaTime) override;

    // Methods for creating Bullet Physics rigid bodies, returning btRigidBody*
    // These will be called when adding PhysicsComponent to entities
    btRigidBody* CreateGroundPlane(const glm::vec3& position);
    btRigidBody* CreateCube(const glm::vec3& position, float mass);

    // Method to apply force to a rigid body (called from input handling)
    void ApplyForce(btRigidBody* body, const glm::vec3& force);


private:
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* overlappingPairCache;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* dynamicsWorld;

    // Keep track of allocated collision shapes and motion states
    // These are managed by the PhysicsSystem as they are Bullet Physics specific
    std::vector<btCollisionShape*> collisionShapes;
    std::vector<btDefaultMotionState*> motionStates;
};

#endif // SYSTEMS_H