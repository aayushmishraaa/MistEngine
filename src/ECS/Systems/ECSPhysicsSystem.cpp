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
            // Bullet simulates in world space, so a body must be *born* at
            // the entity's world position — otherwise a parented entity
            // spawns its collider at its parent-relative offset.
            //
            // The write-back below still targets `transform.position`, which
            // is the LOCAL field. For a parentless entity those are the same
            // and everything is consistent. A parented *dynamic* body is
            // genuinely unsupported: Bullet owns its world transform and the
            // hierarchy would fight it every tick. Godot resolves this by
            // having RigidBody3D ignore its parent while simulating; doing
            // the same here is a follow-up, not part of this fix.
            physics->EnsureBody(transform.WorldPosition(), pc);
            m_OwnedBodies[entity] = pc.rigidBody;
            // Tag the Bullet body with its owning ECS entity so
            // raycast hits can be resolved back to the scene. Stored
            // as an int via Bullet's user-index slot — entity ids fit
            // in 32 bits so the round-trip is lossless.
            if (pc.rigidBody) {
                pc.rigidBody->setUserIndex(static_cast<int>(entity));
            }
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

            // Decompose the orientation using the SAME convention
            // TransformComponent::GetModelMatrix composes with, which is
            // Rx * Ry * Rz (glm::eulerAngleXYZ).
            //
            // The previous code called btQuaternion::getEulerZYX and then
            // assigned its outputs in the wrong slots as well: that function
            // yields (yawZ, pitchY, rollX), and they were written to
            // (x, y, z) = (pitch, yaw, roll) — so the Y rotation landed on X
            // and the Z rotation landed on Y. Two bugs compounding: a
            // convention mismatch on top of a transposed mapping. Rotating
            // bodies visibly drifted and tumbled on the wrong axes.
            btQuaternion rotation = trans.getRotation();
            const glm::quat q(rotation.getW(), rotation.getX(),
                              rotation.getY(), rotation.getZ());
            float rx = 0.0f, ry = 0.0f, rz = 0.0f;
            glm::extractEulerAngleXYZ(glm::mat4_cast(q), rx, ry, rz);
            transform.rotation = glm::degrees(glm::vec3(rx, ry, rz));
        }
    }
}
