#ifndef PHYSICSCOMPONENT_H
#define PHYSICSCOMPONENT_H

#include <bullet/btBulletDynamicsCommon.h>

struct PhysicsComponent {
    btRigidBody* rigidBody;
    bool syncTransform = true;
};

#endif // PHYSICSCOMPONENT_H
