#include "ECS/Systems/BotSystem.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/BotComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/HealthComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include <random>
#include <iostream>
#include <GLFW/glfw3.h>

extern Coordinator gCoordinator;

void BotSystem::Update(float deltaTime) {
    if (!m_IsGameMode) return;
    
    UpdateBotAI(deltaTime);
}

void BotSystem::UpdateBotAI(float deltaTime) {
    for (auto const& entity : m_Entities) {
        HandleBotState(entity, deltaTime);
    }
}

void BotSystem::HandleBotState(Entity entity, float deltaTime) {
    auto& bot = gCoordinator.GetComponent<BotComponent>(entity);
    auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
    
    // Check if bot is dead
    try {
        auto& health = gCoordinator.GetComponent<HealthComponent>(entity);
        if (health.isDead && bot.state != BotState::DEAD) {
            bot.state = BotState::DEAD;
            bot.stateChangeTime = 0.0f;
            std::cout << "Bot " << entity << " died!" << std::endl;
        }
    } catch (...) {
        // No health component
    }
    
    bot.stateChangeTime += deltaTime;
    
    switch (bot.state) {
        case BotState::IDLE:
            HandleIdleState(entity, deltaTime);
            break;
        case BotState::PATROL:
            HandlePatrolState(entity, deltaTime);
            break;
        case BotState::CHASE:
            HandleChaseState(entity, deltaTime);
            break;
        case BotState::ATTACK:
            HandleAttackState(entity, deltaTime);
            break;
        case BotState::DEAD:
            HandleDeadState(entity, deltaTime);
            break;
    }
}

void BotSystem::HandleIdleState(Entity entity, float deltaTime) {
    auto& bot = gCoordinator.GetComponent<BotComponent>(entity);
    auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
    
    // Look for nearby players
    Entity player = FindNearestPlayer(transform.position, bot.detectionRange);
    if (player != 0) {
        bot.target = player;
        bot.hasTarget = true;
        bot.state = BotState::CHASE;
        bot.stateChangeTime = 0.0f;
        std::cout << "Bot " << entity << " spotted player " << player << "!" << std::endl;
        return;
    }
    
    // After some idle time, start patrolling
    if (bot.stateChangeTime > 2.0f) {
        bot.state = BotState::PATROL;
        bot.patrolCenter = transform.position;
        bot.currentPatrolTarget = GetRandomPatrolPoint(bot.patrolCenter, bot.patrolRadius);
        bot.stateChangeTime = 0.0f;
    }
}

void BotSystem::HandlePatrolState(Entity entity, float deltaTime) {
    auto& bot = gCoordinator.GetComponent<BotComponent>(entity);
    auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
    
    // Look for nearby players
    Entity player = FindNearestPlayer(transform.position, bot.detectionRange);
    if (player != 0) {
        bot.target = player;
        bot.hasTarget = true;
        bot.state = BotState::CHASE;
        bot.stateChangeTime = 0.0f;
        std::cout << "Bot " << entity << " spotted player " << player << " while patrolling!" << std::endl;
        return;
    }
    
    // Move towards patrol point
    float distanceToPatrolPoint = glm::distance(transform.position, bot.currentPatrolTarget);
    if (distanceToPatrolPoint > 1.0f) {
        MoveTowards(entity, bot.currentPatrolTarget, bot.movementSpeed * 0.5f, deltaTime); // Slower patrol speed
    } else {
        // Reached patrol point, wait a bit then pick new point
        bot.timeAtPatrolPoint += deltaTime;
        if (bot.timeAtPatrolPoint > bot.patrolWaitTime) {
            bot.currentPatrolTarget = GetRandomPatrolPoint(bot.patrolCenter, bot.patrolRadius);
            bot.timeAtPatrolPoint = 0.0f;
        }
    }
}

void BotSystem::HandleChaseState(Entity entity, float deltaTime) {
    auto& bot = gCoordinator.GetComponent<BotComponent>(entity);
    auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
    
    if (!bot.hasTarget) {
        bot.state = BotState::IDLE;
        bot.stateChangeTime = 0.0f;
        return;
    }
    
    float distanceToPlayer = GetDistanceToPlayer(entity, bot.target);
    
    // Check if player is still in detection range (with some hysteresis)
    if (distanceToPlayer > bot.detectionRange * 1.5f) {
        bot.hasTarget = false;
        bot.state = BotState::IDLE;
        bot.stateChangeTime = 0.0f;
        std::cout << "Bot " << entity << " lost sight of player" << std::endl;
        return;
    }
    
    // If close enough to attack, switch to attack state
    if (distanceToPlayer <= bot.attackRange) {
        bot.state = BotState::ATTACK;
        bot.stateChangeTime = 0.0f;
        return;
    }
    
    // Chase the player
    try {
        auto& playerTransform = gCoordinator.GetComponent<TransformComponent>(bot.target);
        MoveTowards(entity, playerTransform.position, bot.movementSpeed, deltaTime);
    } catch (...) {
        // Player entity no longer exists
        bot.hasTarget = false;
        bot.state = BotState::IDLE;
        bot.stateChangeTime = 0.0f;
    }
}

void BotSystem::HandleAttackState(Entity entity, float deltaTime) {
    auto& bot = gCoordinator.GetComponent<BotComponent>(entity);
    
    if (!bot.hasTarget) {
        bot.state = BotState::IDLE;
        bot.stateChangeTime = 0.0f;
        return;
    }
    
    float distanceToPlayer = GetDistanceToPlayer(entity, bot.target);
    
    // If player moved away, go back to chasing
    if (distanceToPlayer > bot.attackRange * 1.2f) {
        bot.state = BotState::CHASE;
        bot.stateChangeTime = 0.0f;
        return;
    }
    
    // Attack!
    float currentTime = glfwGetTime();
    if (currentTime - bot.lastAttackTime >= bot.attackCooldown) {
        bot.lastAttackTime = currentTime;
        
        // Deal damage to player
        try {
            auto& playerHealth = gCoordinator.GetComponent<HealthComponent>(bot.target);
            playerHealth.TakeDamage(bot.attackDamage);
            std::cout << "Bot " << entity << " attacks player for " << bot.attackDamage << " damage! Player health: " << playerHealth.currentHealth << std::endl;
        } catch (...) {
            // Player has no health component
        }
    }
}

void BotSystem::HandleDeadState(Entity entity, float deltaTime) {
    // Bot is dead, maybe respawn after some time or just stay dead
    // For now, just stay dead
}

Entity BotSystem::FindNearestPlayer(const glm::vec3& botPosition, float maxDistance) {
    Entity nearestPlayer = 0;
    float nearestDistance = maxDistance;
    
    // Search for entities with PlayerComponent
    for (Entity entity = 0; entity < 5000; ++entity) { // Max entity check
        try {
            auto& player = gCoordinator.GetComponent<PlayerComponent>(entity);
            auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
            
            float distance = glm::distance(botPosition, transform.position);
            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearestPlayer = entity;
            }
        } catch (...) {
            // Entity doesn't have player component
        }
    }
    
    return nearestPlayer;
}

float BotSystem::GetDistanceToPlayer(Entity bot, Entity player) {
    try {
        auto& botTransform = gCoordinator.GetComponent<TransformComponent>(bot);
        auto& playerTransform = gCoordinator.GetComponent<TransformComponent>(player);
        return glm::distance(botTransform.position, playerTransform.position);
    } catch (...) {
        return 999999.0f; // Very large distance if components missing
    }
}

glm::vec3 BotSystem::GetRandomPatrolPoint(const glm::vec3& center, float radius) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    
    glm::vec3 randomDir(dis(gen), 0.0f, dis(gen));
    if (glm::length(randomDir) > 0.0f) {
        randomDir = glm::normalize(randomDir);
    }
    
    float distance = std::uniform_real_distribution<float>(radius * 0.3f, radius)(gen);
    return center + randomDir * distance;
}

void BotSystem::MoveTowards(Entity entity, const glm::vec3& target, float speed, float deltaTime) {
    auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
    
    glm::vec3 direction = target - transform.position;
    direction.y = 0.0f; // Keep movement on ground plane
    
    if (glm::length(direction) > 0.0f) {
        direction = glm::normalize(direction);
        glm::vec3 movement = direction * speed * deltaTime;
        
        // Apply to physics body if available
        try {
            auto& physics = gCoordinator.GetComponent<PhysicsComponent>(entity);
            if (physics.rigidBody) {
                btVector3 btVelocity(movement.x / deltaTime, 0.0f, movement.z / deltaTime);
                btVector3 currentVelocity = physics.rigidBody->getLinearVelocity();
                btVelocity.setY(currentVelocity.getY()); // Preserve Y velocity
                physics.rigidBody->setLinearVelocity(btVelocity);
            }
        } catch (...) {
            // No physics component, apply to transform directly
            transform.position += movement;
        }
    }
}