
#include "GameObject.h"
#include "Physics.h" // Include Physics.h to access PhysicsObject

GameObject::GameObject()
    : position(0.0f), rotation(0.0f), scale(1.0f), physicsObject(nullptr)
{
    // Initialize other members
}

GameObject::~GameObject()
{
    // Clean up the associated physics object if owned by the GameObject
    // However, in our current Scene structure, the PhysicsWorld owns the PhysicsObjects.
    // So, we should NOT delete physicsObject here.
}

glm::mat4 GameObject::getModelMatrix() const
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);
    return model;
}

void GameObject::update(float deltaTime)
{
    // Default update logic
    // If using a physics object, update the game object's position from the physics object
    if (physicsObject) {
        position = physicsObject->properties.position;
        // TODO: Handle rotation synchronization from physics object (more complex with rigid bodies)
    }
}

void GameObject::render(Shader& shader)
{
    // Default render logic (empty - derived classes will implement or use a Renderer)
}