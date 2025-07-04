c++
#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "ECSManager.h"
#include "Mesh.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <bullet/btBulletDynamicsCommon.h>

// Position/Transform Component
struct PositionComponent : public Component {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    PositionComponent(const glm::vec3& pos = glm::vec3(0.0f), const glm::quat& rot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), const glm::vec3& s = glm::vec3(1.0f))
        : position(pos), rotation(rot), scale(s) {}
};

// Renderable Component
struct RenderComponent : public Component {
    Mesh* mesh;
    // Add material/texture information here later

    RenderComponent(Mesh* m)
        : mesh(m) {}
};

// Physics Component
struct PhysicsComponent : public Component {
    btRigidBody* rigidBody;
    // Add other physics-related properties here later (e.g., mass, friction, restitution)

    PhysicsComponent(btRigidBody* body)
        : rigidBody(body) {}
};

#endif // COMPONENTS_H