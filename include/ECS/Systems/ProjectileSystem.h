#ifndef PROJECTILESYSTEM_H
#define PROJECTILESYSTEM_H

#include "../System.h"
#include <glm/glm.hpp>
#include <vector>

class ProjectileSystem : public System {
public:
    ProjectileSystem();
    
    void Update(float deltaTime) override;
    
    // Projectile management
    void DestroyProjectile(Entity projectileEntity);
    
    // Hit detection
    Entity CheckCollision(Entity projectileEntity, glm::vec3 oldPos, glm::vec3 newPos);
    bool RaycastHit(glm::vec3 origin, glm::vec3 direction, float maxDistance, Entity& hitEntity, glm::vec3& hitPoint);
    
    // Damage dealing
    void DealDamage(Entity projectileEntity, Entity targetEntity, glm::vec3 hitPoint);
    
private:
    // Physics updates
    void UpdateProjectilePhysics(Entity projectile, float deltaTime);
    void UpdateProjectileLifetime(Entity projectile, float deltaTime);
    
    // Collision detection helpers
    bool CheckEntityCollision(Entity projectile, Entity target, glm::vec3 projPos);
    bool IsValidTarget(Entity projectileEntity, Entity targetEntity);
    
    // Visual effects
    void CreateHitEffect(glm::vec3 position, Entity hitEntity);
    void CreateMuzzleFlash(glm::vec3 position, glm::vec3 direction);
};

#endif // PROJECTILESYSTEM_H