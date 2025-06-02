c++
#include "PhysicsWorld.h"
#include <algorithm> // For std::remove

PhysicsWorld::PhysicsWorld()
    : m_gravity(0.0f, -9.81f, 0.0f) // Default gravity (e.g., Earth's gravity)
{
}

PhysicsWorld::~PhysicsWorld()
{
    // Clean up allocated physics objects
    for (PhysicsObject* obj : m_objects) {
        delete obj;
    }
    m_objects.clear();
}

void PhysicsWorld::addObject(PhysicsObject* obj)
{
    if (obj) {
        m_objects.push_back(obj);
    }
}

void PhysicsWorld::removeObject(PhysicsObject* obj)
{
    // Remove the object from the vector
    m_objects.erase(std::remove(m_objects.begin(), m_objects.end(), obj), m_objects.end());

    // Note: This does not delete the object. The caller is responsible for deleting the object.
    // We might want to reconsider ownership later, but for now, assume the caller manages object lifetime.
}

void PhysicsWorld::update(float deltaTime)
{
    // Apply gravity to all objects and update them
    for (PhysicsObject* obj : m_objects) {
        // Apply gravity force
        obj->applyForce(m_gravity * obj->properties.mass);

        // Update the object's physics properties
        obj->update(deltaTime);
    }

    // TODO: Implement collision detection and response in a later phase
}