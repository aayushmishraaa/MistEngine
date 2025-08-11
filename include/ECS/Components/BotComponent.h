#ifndef BOTCOMPONENT_H
#define BOTCOMPONENT_H

#include <glm/glm.hpp>
#include "../Entity.h"

enum class BotState {
    IDLE,
    PATROL,
    CHASE,
    ATTACK,
    DEAD
};

struct BotComponent {
    BotState state = BotState::IDLE;
    Entity target = 0; // Player entity to chase/attack
    bool hasTarget = false;
    
    // AI parameters
    float detectionRange = 15.0f;
    float attackRange = 2.0f;
    float movementSpeed = 3.0f;
    float attackDamage = 20.0f;
    float attackCooldown = 1.5f;
    float lastAttackTime = 0.0f;
    
    // Patrol behavior
    glm::vec3 patrolCenter{0.0f};
    float patrolRadius = 10.0f;
    glm::vec3 currentPatrolTarget{0.0f};
    float patrolWaitTime = 2.0f;
    float timeAtPatrolPoint = 0.0f;
    
    // Navigation
    glm::vec3 targetPosition{0.0f};
    float stateChangeTime = 0.0f;
};

#endif // BOTCOMPONENT_H