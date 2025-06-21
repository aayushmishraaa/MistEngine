
#ifndef MISTENGINE_GAMEOBJECT_H
#define MISTENGINE_GAMEOBJECT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Forward declarations
class PhysicsObject; // We'll link GameObjects to PhysicsObjects
#include "Shader.h"
// class RenderComponent; // If using a component system
// class PhysicsComponent; // If using a component system

class GameObject {
public:
    // Constructor and Destructor
    GameObject();
    virtual ~GameObject();

    // Transformation properties
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    // Get the model matrix for rendering
    glm::mat4 getModelMatrix() const;

    // Update method (virtual to be overridden by derived classes)
    virtual void update(float deltaTime);

    // Render method (virtual to be overridden by derived classes or handled by a Renderer)
 virtual void render(Shader& shader); // Accept a shader reference

    // Link to physics object (for now, a direct pointer)
    PhysicsObject* physicsObject; // TODO: Consider a component system instead

private:
    // Unique identifier (optional, but useful)
    // int m_id;
};

#endif // MISTENGINE_GAMEOBJECT_H