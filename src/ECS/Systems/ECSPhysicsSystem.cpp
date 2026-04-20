#include "ECS/Systems/ECSPhysicsSystem.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "Core/ServiceLocator.h"
#include "PhysicsSystem.h"

#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

extern Coordinator gCoordinator;

void ECSPhysicsSystem::Update(float deltaTime) {
    (void)deltaTime;

    PhysicsSystem* physics = ServiceLocator::Instance().GetPhysicsSystem();

    // --- Orphan cleanup. m_Entities is kept live by SystemManager; any
    // entity in m_OwnedBodies but not in m_Entities was destroyed between
    // ticks and still has a Bullet body attached — free it now before we
    // step the world again.
    if (physics) {
        for (auto it = m_OwnedBodies.begin(); it != m_OwnedBodies.end(); ) {
            if (m_Entities.find(it->first) == m_Entities.end()) {
                physics->DestroyBody(it->second);
                it = m_OwnedBodies.erase(it);
            } else {
                ++it;
            }
        }
    }

    // --- Upsert pass. Build / rebuild / live-apply for each physics entity.
    if (physics) {
        for (auto const& entity : m_Entities) {
            auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
            auto& pc        = gCoordinator.GetComponent<PhysicsComponent>(entity);
            physics->EnsureBody(transform.position, pc);
            m_OwnedBodies[entity] = pc.rigidBody;
        }
    }

    // --- Bullet -> ECS sync. Happens after upsert so freshly-created
    // bodies still write their spawn transform back (motion state is
    // initialised from position, so this is a no-op on creation frame).
    for (auto const& entity : m_Entities) {
        auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
        auto& pc        = gCoordinator.GetComponent<PhysicsComponent>(entity);

        if (pc.rigidBody && pc.syncTransform) {
            btTransform trans;
            pc.rigidBody->getMotionState()->getWorldTransform(trans);

            btVector3 origin = trans.getOrigin();
            transform.position = glm::vec3(origin.getX(), origin.getY(), origin.getZ());

            btQuaternion rotation = trans.getRotation();
            btScalar yaw, pitch, roll;
            rotation.getEulerZYX(yaw, pitch, roll);
            transform.rotation = glm::vec3(glm::degrees(pitch),
                                            glm::degrees(yaw),
                                            glm::degrees(roll));
        }
    }
}
