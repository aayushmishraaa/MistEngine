#include "ECS/Systems/ECSPhysicsSystem.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

// Add the global coordinator declaration
extern Coordinator gCoordinator;

void ECSPhysicsSystem::Update(float deltaTime) {
    for (auto const& entity : m_Entities) {
        auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
        auto& physics = gCoordinator.GetComponent<PhysicsComponent>(entity);

        if (physics.rigidBody && physics.syncTransform) {
            // Update transform from physics
            btTransform trans;
            physics.rigidBody->getMotionState()->getWorldTransform(trans);

            // Update position
            btVector3 origin = trans.getOrigin();
            transform.position = glm::vec3(origin.getX(), origin.getY(), origin.getZ());

            // Update rotation
            btQuaternion rotation = trans.getRotation();
            btScalar yaw, pitch, roll;
            rotation.getEulerZYX(yaw, pitch, roll);
            transform.rotation = glm::vec3(glm::degrees(pitch), glm::degrees(yaw), glm::degrees(roll));
        }
    }
}

