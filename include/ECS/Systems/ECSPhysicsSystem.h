#ifndef ECS_PHYSICSSYSTEM_H
#define ECS_PHYSICSSYSTEM_H

#include "../System.h"
#include "../Coordinator.h"

extern Coordinator gCoordinator;

class ECSPhysicsSystem : public System {
public:
    void Update(float deltaTime);
};

#endif // ECS_PHYSICSSYSTEM_H
