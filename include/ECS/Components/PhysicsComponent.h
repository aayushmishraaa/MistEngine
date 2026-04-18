#ifndef PHYSICSCOMPONENT_H
#define PHYSICSCOMPONENT_H

#include "Core/Reflection.h"

#include <btBulletDynamicsCommon.h>

struct PhysicsComponent {
    btRigidBody* rigidBody = nullptr;
    bool syncTransform = true;
};

MIST_REFLECT(PhysicsComponent)
    MIST_FIELD(PhysicsComponent, syncTransform, ::Mist::PropertyHint::None, "")
MIST_REFLECT_END(PhysicsComponent)

#endif // PHYSICSCOMPONENT_H
