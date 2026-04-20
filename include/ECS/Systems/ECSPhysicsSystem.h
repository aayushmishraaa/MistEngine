#ifndef ECS_PHYSICSSYSTEM_H
#define ECS_PHYSICSSYSTEM_H

#include "../System.h"
#include "../Coordinator.h"

#include <btBulletDynamicsCommon.h>
#include <unordered_map>

extern Coordinator gCoordinator;

// Two-pass per frame:
//   1. Upsert: for every live entity with PhysicsComponent, call
//      PhysicsSystem::EnsureBody. Creates / rebuilds / live-applies
//      material.
//   2. Sync: copy Bullet's post-step world transform back onto the
//      entity's TransformComponent (classic Bullet -> ECS sync).
//
// Between tick N and tick N+1 an entity might be destroyed. We detect
// that by diffing the owned-body map against m_Entities and free any
// orphan body via PhysicsSystem::DestroyBody — otherwise Bullet keeps
// simulating a ghost collider.
class ECSPhysicsSystem : public System {
public:
    void Update(float deltaTime);

private:
    std::unordered_map<Entity, btRigidBody*> m_OwnedBodies;
};

#endif // ECS_PHYSICSSYSTEM_H
