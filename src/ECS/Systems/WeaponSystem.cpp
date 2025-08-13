#include "ECS/Systems/WeaponSystem.h"
#include "ECS/Systems/PlayerSystem.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/WeaponComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/ProjectileComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include <iostream>
#include <algorithm>

extern Coordinator gCoordinator;

WeaponSystem::WeaponSystem() 
    : m_playerSystem(nullptr)
    , m_gameTime(0.0f)
{
}

void WeaponSystem::Init(PlayerSystem* playerSystem) {
    m_playerSystem = playerSystem;
}

void WeaponSystem::Update(float deltaTime) {
    m_gameTime += deltaTime;
    
    // Update all weapons
    for (auto const& entity : m_Entities) {
        UpdateReloading(entity, deltaTime);
        UpdateMuzzleFlash(entity, deltaTime);
    }
    
    // Handle player weapon actions
    if (m_playerSystem && m_playerSystem->HasPlayer()) {
        Entity player = m_playerSystem->GetPlayer();
        
        try {
            auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(player);
            auto& playerTransform = gCoordinator.GetComponent<TransformComponent>(player);
            
            if (!playerComp.isAlive) return;
            
            // Handle shooting
            if (playerComp.wantsToShoot && !playerComp.weapons.empty()) {
                Entity currentWeapon = playerComp.weapons[playerComp.currentWeapon];
                
                if (CanShoot(currentWeapon)) {
                    // Calculate shoot direction from camera/player facing direction
                    // For now, shoot straight forward (we'll improve this later)
                    glm::vec3 shootOrigin = playerTransform.position;
                    shootOrigin.y += 1.5f; // Shoulder height
                    
                    glm::vec3 shootDirection(0.0f, 0.0f, -1.0f); // Forward
                    
                    Shoot(currentWeapon, shootOrigin, shootDirection);
                }
            }
            
            // Handle reloading
            if (playerComp.wantsToReload && !playerComp.weapons.empty()) {
                Entity currentWeapon = playerComp.weapons[playerComp.currentWeapon];
                Reload(currentWeapon);
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error processing player weapon actions: " << e.what() << std::endl;
        }
    }
}

Entity WeaponSystem::CreateWeapon(WeaponType type, glm::vec3 position) {
    Entity weapon = gCoordinator.CreateEntity();
    
    // Add Transform component
    TransformComponent transform;
    transform.position = position;
    gCoordinator.AddComponent(weapon, transform);
    
    // Add Weapon component
    WeaponComponent weaponComp;
    weaponComp.type = type;
    
    // Configure weapon stats based on type
    ConfigureWeaponStats(weapon, type);
    
    gCoordinator.AddComponent(weapon, weaponComp);
    
    std::cout << "Created weapon of type " << static_cast<int>(type) << " with ID: " << weapon << std::endl;
    
    return weapon;
}

Entity WeaponSystem::CreatePistol(glm::vec3 position) {
    return CreateWeapon(WeaponType::PISTOL, position);
}

Entity WeaponSystem::CreateRifle(glm::vec3 position) {
    return CreateWeapon(WeaponType::RIFLE, position);
}

void WeaponSystem::ConfigureWeaponStats(Entity weapon, WeaponType type) {
    try {
        auto& weaponComp = gCoordinator.GetComponent<WeaponComponent>(weapon);
        
        switch (type) {
            case WeaponType::PISTOL:
                weaponComp.name = "Pistol";
                weaponComp.currentAmmo = 12;
                weaponComp.maxAmmo = 12;
                weaponComp.reserveAmmo = 84;
                weaponComp.reloadTime = 1.5f;
                weaponComp.fireRate = 0.3f;
                weaponComp.damage = 25.0f;
                weaponComp.range = 50.0f;
                weaponComp.accuracy = 0.9f;
                break;
                
            case WeaponType::RIFLE:
                weaponComp.name = "Assault Rifle";
                weaponComp.currentAmmo = 30;
                weaponComp.maxAmmo = 30;
                weaponComp.reserveAmmo = 120;
                weaponComp.reloadTime = 2.0f;
                weaponComp.fireRate = 0.15f;
                weaponComp.damage = 35.0f;
                weaponComp.range = 100.0f;
                weaponComp.accuracy = 0.8f;
                break;
                
            case WeaponType::SHOTGUN:
                weaponComp.name = "Shotgun";
                weaponComp.currentAmmo = 6;
                weaponComp.maxAmmo = 6;
                weaponComp.reserveAmmo = 24;
                weaponComp.reloadTime = 2.5f;
                weaponComp.fireRate = 0.8f;
                weaponComp.damage = 60.0f;
                weaponComp.range = 25.0f;
                weaponComp.accuracy = 0.6f;
                break;
                
            case WeaponType::SNIPER:
                weaponComp.name = "Sniper Rifle";
                weaponComp.currentAmmo = 5;
                weaponComp.maxAmmo = 5;
                weaponComp.reserveAmmo = 20;
                weaponComp.reloadTime = 3.0f;
                weaponComp.fireRate = 1.5f;
                weaponComp.damage = 100.0f;
                weaponComp.range = 200.0f;
                weaponComp.accuracy = 0.95f;
                break;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error configuring weapon stats: " << e.what() << std::endl;
    }
}

void WeaponSystem::EquipWeapon(Entity weaponEntity, Entity ownerEntity) {
    try {
        auto& weaponComp = gCoordinator.GetComponent<WeaponComponent>(weaponEntity);
        weaponComp.ownerId = ownerEntity;
        weaponComp.isEquipped = true;
        
        // Add weapon to owner's weapon list
        try {
            auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(ownerEntity);
            playerComp.weapons.push_back(weaponEntity);
            
            // If this is the first weapon, make it current
            if (playerComp.weapons.size() == 1) {
                playerComp.currentWeapon = 0;
            }
            
            std::cout << "Equipped " << weaponComp.name << " to player" << std::endl;
            
        } catch (...) {
            // Owner is not a player, could be an enemy
            std::cout << "Equipped " << weaponComp.name << " to entity " << ownerEntity << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error equipping weapon: " << e.what() << std::endl;
    }
}

void WeaponSystem::UnequipWeapon(Entity weaponEntity) {
    try {
        auto& weaponComp = gCoordinator.GetComponent<WeaponComponent>(weaponEntity);
        
        Entity ownerId = weaponComp.ownerId;
        weaponComp.ownerId = -1;
        weaponComp.isEquipped = false;
        
        // Remove from owner's weapon list
        if (ownerId != -1) {
            try {
                auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(ownerId);
                auto it = std::find(playerComp.weapons.begin(), playerComp.weapons.end(), weaponEntity);
                if (it != playerComp.weapons.end()) {
                    playerComp.weapons.erase(it);
                    
                    // Adjust current weapon index if needed
                    if (playerComp.currentWeapon >= (int)playerComp.weapons.size()) {
                        playerComp.currentWeapon = std::max(0, (int)playerComp.weapons.size() - 1);
                    }
                }
                
            } catch (...) {
                // Owner is not a player
            }
        }
        
        std::cout << "Unequipped " << weaponComp.name << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error unequipping weapon: " << e.what() << std::endl;
    }
}

bool WeaponSystem::CanShoot(Entity weaponEntity) const {
    try {
        auto& weaponComp = gCoordinator.GetComponent<WeaponComponent>(weaponEntity);
        
        // Check if weapon can shoot
        if (!weaponComp.canShoot || !weaponComp.isEquipped) {
            return false;
        }
        
        // Check ammo
        if (weaponComp.currentAmmo <= 0) {
            return false;
        }
        
        // Check reload state
        if (weaponComp.isReloading) {
            return false;
        }
        
        // Check fire rate
        if (m_gameTime - weaponComp.lastShotTime < weaponComp.fireRate) {
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error checking if weapon can shoot: " << e.what() << std::endl;
        return false;
    }
}

void WeaponSystem::Shoot(Entity weaponEntity, glm::vec3 origin, glm::vec3 direction) {
    try {
        auto& weaponComp = gCoordinator.GetComponent<WeaponComponent>(weaponEntity);
        
        if (!CanShoot(weaponEntity)) {
            return;
        }
        
        // Consume ammo
        weaponComp.currentAmmo--;
        weaponComp.lastShotTime = m_gameTime;
        
        // Apply accuracy (add random spread)
        glm::vec3 shootDirection = direction;
        float spread = (1.0f - weaponComp.accuracy) * 0.2f; // Max spread of 0.2 radians
        
        if (spread > 0.0f) {
            // Add random spread to direction
            float spreadX = ((rand() % 1000) / 1000.0f - 0.5f) * spread * 2.0f;
            float spreadY = ((rand() % 1000) / 1000.0f - 0.5f) * spread * 2.0f;
            
            shootDirection.x += spreadX;
            shootDirection.y += spreadY;
            shootDirection = glm::normalize(shootDirection);
        }
        
        // Create projectile
        Entity projectile = CreateProjectile(weaponEntity, origin, shootDirection);
        
        // Visual effects
        weaponComp.showMuzzleFlash = true;
        weaponComp.muzzleFlashTime = 0.1f; // Show for 0.1 seconds
        
        std::cout << "Fired " << weaponComp.name << " (" << weaponComp.currentAmmo 
                  << "/" << weaponComp.maxAmmo << " ammo remaining)" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error shooting weapon: " << e.what() << std::endl;
    }
}

Entity WeaponSystem::CreateProjectile(Entity weaponEntity, glm::vec3 origin, glm::vec3 direction) {
    try {
        auto& weaponComp = gCoordinator.GetComponent<WeaponComponent>(weaponEntity);
        
        Entity projectile = gCoordinator.CreateEntity();
        
        // Add Transform component
        TransformComponent transform;
        transform.position = origin;
        transform.scale = glm::vec3(0.1f, 0.1f, 0.5f); // Small bullet size
        gCoordinator.AddComponent(projectile, transform);
        
        // Add Projectile component
        ProjectileComponent projComp;
        projComp.type = ProjectileType::BULLET;
        projComp.direction = direction;
        projComp.velocity = direction * projComp.speed;
        projComp.damage = weaponComp.damage;
        projComp.maxRange = weaponComp.range;
        projComp.ownerId = weaponComp.ownerId;
        projComp.isPlayerProjectile = true; // Assume player for now
        gCoordinator.AddComponent(projectile, projComp);
        
        // Add simple physics (optional, for collision)
        // TODO: Add physics component for collision detection
        
        return projectile;
        
    } catch (const std::exception& e) {
        std::cerr << "Error creating projectile: " << e.what() << std::endl;
        return static_cast<Entity>(-1);
    }
}

void WeaponSystem::Reload(Entity weaponEntity) {
    try {
        auto& weaponComp = gCoordinator.GetComponent<WeaponComponent>(weaponEntity);
        
        // Check if reload is possible
        if (weaponComp.isReloading) {
            std::cout << weaponComp.name << " is already reloading" << std::endl;
            return;
        }
        
        if (weaponComp.currentAmmo >= weaponComp.maxAmmo) {
            std::cout << weaponComp.name << " is already fully loaded" << std::endl;
            return;
        }
        
        if (weaponComp.reserveAmmo <= 0) {
            std::cout << weaponComp.name << " has no reserve ammo" << std::endl;
            return;
        }
        
        // Start reload
        weaponComp.isReloading = true;
        weaponComp.currentReloadTime = weaponComp.reloadTime;
        
        std::cout << "Reloading " << weaponComp.name << " (" << weaponComp.reloadTime << "s)" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error reloading weapon: " << e.what() << std::endl;
    }
}

void WeaponSystem::UpdateReloading(Entity weapon, float deltaTime) {
    try {
        auto& weaponComp = gCoordinator.GetComponent<WeaponComponent>(weapon);
        
        if (!weaponComp.isReloading) {
            return;
        }
        
        weaponComp.currentReloadTime -= deltaTime;
        
        if (weaponComp.currentReloadTime <= 0.0f) {
            // Reload complete
            int ammoNeeded = weaponComp.maxAmmo - weaponComp.currentAmmo;
            int ammoToTake = std::min(ammoNeeded, weaponComp.reserveAmmo);
            
            weaponComp.currentAmmo += ammoToTake;
            weaponComp.reserveAmmo -= ammoToTake;
            
            weaponComp.isReloading = false;
            weaponComp.currentReloadTime = 0.0f;
            
            std::cout << weaponComp.name << " reloaded (" << weaponComp.currentAmmo 
                      << "/" << weaponComp.maxAmmo << ", " << weaponComp.reserveAmmo << " reserve)" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error updating reload: " << e.what() << std::endl;
    }
}

void WeaponSystem::UpdateMuzzleFlash(Entity weapon, float deltaTime) {
    try {
        auto& weaponComp = gCoordinator.GetComponent<WeaponComponent>(weapon);
        
        if (weaponComp.showMuzzleFlash) {
            weaponComp.muzzleFlashTime -= deltaTime;
            
            if (weaponComp.muzzleFlashTime <= 0.0f) {
                weaponComp.showMuzzleFlash = false;
                weaponComp.muzzleFlashTime = 0.0f;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error updating muzzle flash: " << e.what() << std::endl;
    }
}