c++
#ifndef MISTENGINE_PHYSICSWORLD_H
#define MISTENGINE_PHYSICSWORLD_H

#include "Physics.h"
#include <vector>

// Forward declaration of CollisionShape (if needed in the future)
// class CollisionShape;

class PhysicsWorld {
public:
    // Constructor
    PhysicsWorld();

    // Destructor
    ~PhysicsWorld();

    // Add a physics object to the world
    void addObject(PhysicsObject* obj);

    // Remove a physics object from the world
    void removeObject(PhysicsObject* obj);

    // Update the physics simulation for a given delta time
    void update(float deltaTime);

    // Set the gravity for the physics world
    void setGravity(const glm::vec3& gravity) { m_gravity = gravity; }

    // Get the current gravity
    const glm::vec3& getGravity() const { return m_gravity; }

private:
    std::vector<PhysicsObject*> m_objects;
    glm::vec3 m_gravity;
};

#endif // MISTENGINE_PHYSICSWORLD_H