c++
#ifndef MISTENGINE_PHYSICS_H
#define MISTENGINE_PHYSICS_H

#include <glm/glm.hpp>

// Forward declaration of CollisionShape
class CollisionShape;

// Basic structure for physical properties
struct PhysicsProperties {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    float mass;
    glm::vec3 force; // Net force acting on the object

    // Constructor
    PhysicsProperties() : position(0.0f), velocity(0.0f), acceleration(0.0f), mass(1.0f), force(0.0f) {}
};

// Base class for a physical object
class PhysicsObject {
public:
    PhysicsProperties properties;
    CollisionShape* collisionShape; // Pointer to the collision shape

    // Constructor
    PhysicsObject() : collisionShape(nullptr) {}

    // Destructor
    virtual ~PhysicsObject() {}

    // Function to update the object's physics properties
    virtual void update(float deltaTime) = 0; // Pure virtual function for integration

    // Function to apply a force to the object
    void applyForce(const glm::vec3& f) {
        properties.force += f;
    }

    // Function to clear accumulated forces (should be called each frame)
    void clearForces() {
        properties.force = glm::vec3(0.0f);
    }
};

#endif // MISTENGINE_PHYSICS_H