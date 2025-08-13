#ifndef WEAPONSYSTEM_H
#define WEAPONSYSTEM_H

#include "../System.h"
#include "../Components/WeaponComponent.h"
#include <glm/glm.hpp>
#include <memory>

class PlayerSystem;

class WeaponSystem : public System {
public:
    WeaponSystem();
    
    void Init(PlayerSystem* playerSystem);
    void Update(float deltaTime) override;
    
    // Weapon creation
    Entity CreateWeapon(WeaponType type, glm::vec3 position = glm::vec3(0.0f));
    Entity CreatePistol(glm::vec3 position = glm::vec3(0.0f));
    Entity CreateRifle(glm::vec3 position = glm::vec3(0.0f));
    
    // Weapon management
    void EquipWeapon(Entity weaponEntity, Entity ownerEntity);
    void UnequipWeapon(Entity weaponEntity);
    bool CanShoot(Entity weaponEntity) const;
    
    // Actions
    void Shoot(Entity weaponEntity, glm::vec3 origin, glm::vec3 direction);
    void Reload(Entity weaponEntity);
    void UpdateWeaponStates(float deltaTime);
    
private:
    PlayerSystem* m_playerSystem;
    float m_gameTime; // For timing calculations
    
    // Weapon behavior
    void UpdateReloading(Entity weapon, float deltaTime);
    void UpdateMuzzleFlash(Entity weapon, float deltaTime);
    
    // Projectile creation
    Entity CreateProjectile(Entity weaponEntity, glm::vec3 origin, glm::vec3 direction);
    
    // Weapon configuration
    void ConfigureWeaponStats(Entity weapon, WeaponType type);
};

#endif // WEAPONSYSTEM_H