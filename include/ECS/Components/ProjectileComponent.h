#ifndef PROJECTILECOMPONENT_H
#define PROJECTILECOMPONENT_H

#include <glm/glm.hpp>
#include "../Entity.h"

struct ProjectileComponent {
    glm::vec3 velocity{0.0f};
    float damage = 25.0f;
    float lifetime = 5.0f; // Time before projectile disappears
    float age = 0.0f;
    Entity owner = 0; // Who fired this projectile
    bool hasOwner = false;
    
    // Visual properties
    glm::vec3 color{1.0f, 1.0f, 0.0f}; // Yellow by default
    float size = 0.1f;
};

#endif // PROJECTILECOMPONENT_H