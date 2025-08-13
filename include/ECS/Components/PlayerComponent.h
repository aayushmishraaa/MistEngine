#ifndef PLAYERCOMPONENT_H
#define PLAYERCOMPONENT_H

#include <glm/glm.hpp>
#include <vector>

struct PlayerComponent {
    float health = 100.0f;
    float maxHealth = 100.0f;
    float moveSpeed = 5.0f;
    float mouseSensitivity = 0.1f;
    
    // Movement state
    bool isMoving = false;
    glm::vec3 velocity{0.0f};
    
    // Weapon system
    int currentWeapon = 0;
    std::vector<int> weapons; // Entity IDs of equipped weapons
    
    // Player stats
    int score = 0;
    int kills = 0;
    bool isAlive = true;
    
    // Input state
    bool wantsToShoot = false;
    bool wantsToReload = false;
    int wantsToSwitchWeapon = -1; // -1 = no switch, 0+ = weapon index
};

#endif // PLAYERCOMPONENT_H