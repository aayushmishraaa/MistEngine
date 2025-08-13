#ifndef ENEMYCOMPONENT_H
#define ENEMYCOMPONENT_H

#include <glm/glm.hpp>
#include <vector>

enum class EnemyState {
    IDLE,
    PATROL,
    CHASE,
    ATTACK,
    DEAD,
    SEARCHING
};

enum class EnemyType {
    GRUNT,      // Basic enemy
    SOLDIER,    // More health and accuracy
    HEAVY,      // High health, slow, powerful
    SNIPER      // Long range, high damage
};

struct EnemyComponent {
    EnemyType type = EnemyType::GRUNT;
    EnemyState state = EnemyState::PATROL;
    EnemyState previousState = EnemyState::IDLE;
    
    // Health and combat
    float health = 50.0f;
    float maxHealth = 50.0f;
    float damage = 15.0f;
    float attackRange = 20.0f;
    float detectionRange = 30.0f;
    bool isAlive = true;
    
    // AI behavior
    float moveSpeed = 2.0f;
    float attackCooldown = 1.5f;
    float lastAttackTime = 0.0f;
    float stateTimer = 0.0f; // Timer for current state
    
    // Patrol behavior
    std::vector<glm::vec3> patrolPoints;
    int currentPatrolPoint = 0;
    float patrolWaitTime = 2.0f; // Time to wait at each patrol point
    
    // Chase behavior
    glm::vec3 lastKnownPlayerPosition{0.0f};
    float lostPlayerTime = 0.0f;
    float maxSearchTime = 5.0f;
    
    // Target tracking
    int targetPlayerId = -1; // Entity ID of the player being targeted
    float distanceToPlayer = 999.0f;
    bool canSeePlayer = false;
    
    // Weapon
    int weaponId = -1; // Entity ID of equipped weapon
    
    // Score value
    int scoreValue = 10; // Points awarded when killed
};

#endif // ENEMYCOMPONENT_H