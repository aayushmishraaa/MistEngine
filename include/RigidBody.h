c++
#ifndef MISTENGINE_RIGIDBODY_H
#define MISTENGINE_RIGIDBODY_H

#include "Physics.h"

// A concrete implementation of PhysicsObject using Euler integration
class RigidBody : public PhysicsObject {
public:
    // Constructor
    RigidBody() : PhysicsObject() {}

    // Destructor
    ~RigidBody() override {}

    // Implement the pure virtual update function using Euler integration
    void update(float deltaTime) override {
        // Apply gravity (example)
        // properties.applyForce(glm::vec3(0.0f, -9.81f * properties.mass, 0.0f));

        // Calculate acceleration from net force
        properties.acceleration = properties.force / properties.mass;

        // Update velocity
        properties.velocity += properties.acceleration * deltaTime;

        // Update position
        properties.position += properties.velocity * deltaTime;

        // Clear forces for the next frame
        clearForces();
    }
};

#endif // MISTENGINE_RIGIDBODY_H