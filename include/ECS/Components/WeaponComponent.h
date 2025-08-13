#ifndef WEAPONCOMPONENT_H
#define WEAPONCOMPONENT_H

#include <glm/glm.hpp>
#include <string>

enum class WeaponType {
    PISTOL,
    RIFLE,
    SHOTGUN,
    SNIPER
};

struct WeaponComponent {
    WeaponType type = WeaponType::PISTOL;
    std::string name = "Pistol";
    
    // Ammo system
    int currentAmmo = 10;
    int maxAmmo = 10;
    int reserveAmmo = 100;
    float reloadTime = 2.0f;
    float currentReloadTime = 0.0f;
    bool isReloading = false;
    
    // Shooting mechanics
    float fireRate = 0.5f; // Time between shots
    float lastShotTime = 0.0f;
    float damage = 25.0f;
    float range = 100.0f;
    float accuracy = 0.95f; // 0.0 = completely inaccurate, 1.0 = perfect
    
    // Visual/Audio
    glm::vec3 muzzleOffset{0.0f, 0.0f, 0.0f}; // Offset from player position
    bool showMuzzleFlash = false;
    float muzzleFlashTime = 0.0f;
    
    // Weapon state
    bool canShoot = true;
    bool isEquipped = false;
    int ownerId = -1; // Entity ID of the owner (player or enemy)
};

#endif // WEAPONCOMPONENT_H