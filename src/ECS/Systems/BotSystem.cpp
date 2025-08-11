#include "ECS/Systems/BotSystem.h"
#include "ECS/Systems/ProjectileSystem.h"
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
            MarkBotAsDead(entity);
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
    
    Entity player = FindNearestPlayer(transform.position, bot.detectionRange);
    if (player != 0) {
        bot.target = player;
        bot.hasTarget = true;
        bot.state = BotState::CHASE;
        bot.stateChangeTime = 0.0f;
        std::cout << "Bot " << entity << " spotted player " << player << "!" << std::endl;
        return;
    }
    
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
    
    Entity player = FindNearestPlayer(transform.position, bot.detectionRange);
    if (player != 0) {
        bot.target = player;
        bot.hasTarget = true;
        bot.state = BotState::CHASE;
        bot.stateChangeTime = 0.0f;
        std::cout << "Bot " << entity << " spotted player while patrolling!" << std::endl;
        return;
    }
    
    float distanceToPatrolPoint = glm::distance(transform.position, bot.currentPatrolTarget);
    if (distanceToPatrolPoint > 1.0f) {
        MoveTowards(entity, bot.currentPatrolTarget, bot.movementSpeed * 0.5f, deltaTime);
    } else {
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
    
    if (distanceToPlayer > bot.detectionRange * 1.5f) {
        bot.hasTarget = false;
        bot.state = BotState::IDLE;
        bot.stateChangeTime = 0.0f;
        return;
    }
    
    if (distanceToPlayer <= bot.attackRange) {
        bot.state = BotState::ATTACK;
        bot.stateChangeTime = 0.0f;
        return;
    }
    
    try {
        auto& playerTransform = gCoordinator.GetComponent<TransformComponent>(bot.target);
        MoveTowards(entity, playerTransform.position, bot.movementSpeed, deltaTime);
    } catch (...) {
        bot.hasTarget = false;
        bot.state = BotState::IDLE;
        bot.stateChangeTime = 0.0f;
    }
}

void BotSystem::HandleAttackState(Entity entity, float deltaTime) {
    auto& bot = gCoordinator.GetComponent<BotComponent>(entity);
    auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
    
    if (!bot.hasTarget) {
        bot.state = BotState::IDLE;
        bot.stateChangeTime = 0.0f;
        return;
    }
    
    float distanceToPlayer = GetDistanceToPlayer(entity, bot.target);
    
    if (distanceToPlayer > bot.attackRange * 1.2f) {
        bot.state = BotState::CHASE;
        bot.stateChangeTime = 0.0f;
        return;
    }
    
    float currentTime = glfwGetTime();
    if (currentTime - bot.lastAttackTime >= bot.attackCooldown) {
        bot.lastAttackTime = currentTime;
        
        try {
            auto& playerTransform = gCoordinator.GetComponent<TransformComponent>(bot.target);
            glm::vec3 fireDirection = glm::normalize(playerTransform.position - transform.position);
            glm::vec3 fireOrigin = transform.position + glm::vec3(0.0f, 1.0f, 0.0f);
            
            if (m_ProjectileSystem) {
                m_ProjectileSystem->CreateProjectile(fireOrigin, fireDirection, 30.0f, bot.attackDamage, entity);
                std::cout << "Bot " << entity << " fired projectile at player!" << std::endl;
            } else {
                auto& playerHealth = gCoordinator.GetComponent<HealthComponent>(bot.target);
                playerHealth.TakeDamage(bot.attackDamage);
                std::cout << "Bot " << entity << " attacks player for " << bot.attackDamage << " damage!" << std::endl;
            }
        } catch (...) {}
    }
}

void BotSystem::HandleDeadState(Entity entity, float deltaTime) {
    // Bot is dead, just stay in dead state
}

void BotSystem::MarkBotAsDead(Entity entity) {
    try {
        auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
        transform.rotation.x = 90.0f; // Rotate to show death
        
        try {
            auto& physics = gCoordinator.GetComponent<PhysicsComponent>(entity);
            if (physics.rigidBody) {
                btTransform worldTransform = physics.rigidBody->getWorldTransform();
                btQuaternion deathRotation;
                deathRotation.setEulerZYX(0, 0, glm::radians(90.0f));
                worldTransform.setRotation(deathRotation);
                physics.rigidBody->setWorldTransform(worldTransform);
                physics.rigidBody->setActivationState(DISABLE_SIMULATION);
            }
        } catch (...) {}
        
        std::cout << "Bot " << entity << " marked as dead - rotated 90 degrees" << std::endl;
    } catch (...) {}
}

Entity BotSystem::FindNearestPlayer(const glm::vec3& botPosition, float maxDistance) {
    Entity nearestPlayer = 0;
    float nearestDistance = maxDistance;
    
    for (Entity entity = 0; entity < 5000; ++entity) {
        try {
            auto& player = gCoordinator.GetComponent<PlayerComponent>(entity);
            auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
            
            float distance = glm::distance(botPosition, transform.position);
            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearestPlayer = entity;
            }
        } catch (...) {}
    }
    
    return nearestPlayer;
}

float BotSystem::GetDistanceToPlayer(Entity bot, Entity player) {
    try {
        auto& botTransform = gCoordinator.GetComponent<TransformComponent>(bot);
        auto& playerTransform = gCoordinator.GetComponent<TransformComponent>(player);
        return glm::distance(botTransform.position, playerTransform.position);
    } catch (...) {
        return 999999.0f;
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
    direction.y = 0.0f;
    
    if (glm::length(direction) > 0.0f) {
        direction = glm::normalize(direction);
        
        try {
            auto& physics = gCoordinator.GetComponent<PhysicsComponent>(entity);
            if (physics.rigidBody) {
                btVector3 targetVelocity(direction.x * speed, 0.0f, direction.z * speed);
                btVector3 currentVelocity = physics.rigidBody->getLinearVelocity();
                targetVelocity.setY(currentVelocity.getY());
                
                physics.rigidBody->setLinearVelocity(targetVelocity);
                physics.rigidBody->activate(true);
                
                return;
            }
        } catch (...) {}
        
        // Fallback to direct transform movement
        glm::vec3 movement = direction * speed * deltaTime;
        transform.position += movement;
    }
}