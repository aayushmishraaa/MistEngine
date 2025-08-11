#ifndef WEAPONCOMPONENT_H
#define WEAPONCOMPONENT_H

#include <glm/glm.hpp>

struct WeaponComponent {
    float damage = 25.0f;
    float fireRate = 0.5f; // Time between shots in seconds
    float lastShotTime = 0.0f;
    float range = 100.0f;
    int ammo = 30;
    int maxAmmo = 30;
    float reloadTime = 2.0f;
    float lastReloadTime = 0.0f;
    bool isReloading = false;
    
    // Weapon positioning
    glm::vec3 offset{0.5f, -0.2f, -0.8f}; // Offset from camera/player position
    glm::vec3 muzzleOffset{0.0f, 0.0f, -1.0f}; // Where bullets come from
};

#endif // WEAPONCOMPONENT_H