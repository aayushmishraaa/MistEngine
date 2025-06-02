c++
#ifndef MISTENGINE_COLLISION_H
#define MISTENGINE_COLLISION_H

#include <glm/glm.hpp>

// Base class for collision shapes
class CollisionShape {
public:
    enum ShapeType {
        SPHERE,
        BOX
        // Add more shapes here later
    };

    CollisionShape(ShapeType type) : m_type(type) {}
    virtual ~CollisionShape() {}

    ShapeType getType() const { return m_type; }

private:
    ShapeType m_type;
};

// Sphere collision shape
class SphereCollisionShape : public CollisionShape {
public:
    float radius;

    SphereCollisionShape(float r) : CollisionShape(SPHERE), radius(r) {}
};

// Box collision shape (using half-extents)
class BoxCollisionShape : public CollisionShape {
public:
    glm::vec3 halfExtents;

    BoxCollisionShape(const glm::vec3& he) : CollisionShape(BOX), halfExtents(he) {}
};

#endif // MISTENGINE_COLLISION_H