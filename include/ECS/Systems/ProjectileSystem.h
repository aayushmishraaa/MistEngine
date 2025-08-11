#ifndef PROJECTILESYSTEM_H
#define PROJECTILESYSTEM_H

#include "../System.h"
#include <glm/glm.hpp>

class ProjectileSystem : public System {
public:
    void Update(float deltaTime);
    void SetGameMode(bool isGameMode) { m_IsGameMode = isGameMode; }
    
    // Create a projectile
    void CreateProjectile(const glm::vec3& origin, const glm::vec3& direction, float speed, float damage, Entity owner);

private:
    bool m_IsGameMode = false;
    
    void UpdateProjectileMovement(float deltaTime);
    void CheckProjectileCollisions();
    void CleanupExpiredProjectiles(float deltaTime);
    void NotifyKill(Entity killerEntity, Entity victimEntity); // Notify when something is killed
};

#endif // PROJECTILESYSTEM_H