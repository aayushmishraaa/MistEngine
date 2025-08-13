#ifndef PROJECTILECOMPONENT_H
#define PROJECTILECOMPONENT_H

#include <glm/glm.hpp>

enum class ProjectileType {
    BULLET,
    ROCKET,
    GRENADE
};

struct ProjectileComponent {
    ProjectileType type = ProjectileType::BULLET;
    
    // Physics
    glm::vec3 velocity{0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};
    float speed = 50.0f;
    float gravity = 0.0f; // 0 = no gravity (bullets), positive = affected by gravity
    
    // Damage
    float damage = 25.0f;
    float explosionRadius = 0.0f; // 0 = no explosion
    bool penetrating = false; // Can hit multiple targets
    
    // Lifetime
    float lifeTime = 5.0f; // How long before it despawns
    float currentLife = 0.0f;
    float maxRange = 100.0f;
    float traveledDistance = 0.0f;
    
    // Ownership and targeting
    int ownerId = -1; // Entity ID of whoever shot this projectile
    bool isPlayerProjectile = false; // True if shot by player
    
    // Visual effects
    bool hasTrail = false;
    glm::vec3 trailColor{1.0f, 1.0f, 0.0f}; // Yellow trail
    
    // Hit detection
    bool hasHit = false;
    glm::vec3 hitPosition{0.0f};
    int hitEntityId = -1;
};

#endif // PROJECTILECOMPONENT_H