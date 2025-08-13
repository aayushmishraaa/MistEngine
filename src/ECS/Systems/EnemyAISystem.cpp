#include "ECS/Systems/EnemyAISystem.h"
#include "ECS/Systems/PlayerSystem.h"
#include "ECS/Systems/WeaponSystem.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/WeaponComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "PhysicsSystem.h"
#include <iostream>
#include <algorithm>
#include <cmath>

// Forward declarations to avoid OpenGL header conflicts
struct Vertex;
struct Texture;
extern void generateCubeMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);

// Forward declare to avoid including headers with OpenGL conflicts
extern Coordinator gCoordinator;
extern PhysicsSystem* g_physicsSystem;

// We need access to RenderComponent and Mesh, but need to be careful about OpenGL headers
// Include them at the function level to minimize conflicts
class Mesh;
struct RenderComponent {
    void* renderable;
    bool visible;
};

EnemyAISystem::EnemyAISystem() 
    : m_playerSystem(nullptr)
    , m_weaponSystem(nullptr)
{
}

void EnemyAISystem::Init(PlayerSystem* playerSystem, WeaponSystem* weaponSystem) {
    m_playerSystem = playerSystem;
    m_weaponSystem = weaponSystem;
}

void EnemyAISystem::Update(float deltaTime) {
    // Update all enemies
    for (auto const& entity : m_Entities) {
        try {
            auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(entity);
            
            // Skip dead enemies
            if (!enemyComp.isAlive || enemyComp.state == EnemyState::DEAD) {
                continue;
            }
            
            // Update enemy AI state
            UpdateEnemyState(entity, deltaTime);
            
        } catch (const std::exception& e) {
            std::cerr << "Error updating enemy " << entity << ": " << e.what() << std::endl;
        }
    }
}

Entity EnemyAISystem::CreateEnemy(EnemyType type, glm::vec3 position, const std::vector<glm::vec3>& patrolPoints) {
    Entity enemy = gCoordinator.CreateEntity();
    
    // Add Transform component
    TransformComponent transform;
    transform.position = position;
    transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    transform.scale = glm::vec3(1.0f);
    gCoordinator.AddComponent(enemy, transform);
    
    // Add Enemy component
    EnemyComponent enemyComp;
    enemyComp.type = type;
    enemyComp.state = EnemyState::PATROL;
    enemyComp.patrolPoints = patrolPoints;
    
    // If no patrol points provided, create a simple patrol around spawn
    if (patrolPoints.empty()) {
        enemyComp.patrolPoints = {
            position,
            position + glm::vec3(5.0f, 0.0f, 0.0f),
            position + glm::vec3(5.0f, 0.0f, 5.0f),
            position + glm::vec3(0.0f, 0.0f, 5.0f)
        };
    }
    
    ConfigureEnemyStats(enemy, type);
    gCoordinator.AddComponent(enemy, enemyComp);
    
    // Create visuals (includes physics for visibility)
    CreateEnemyVisuals(enemy, type);
    
    // NOTE: Physics is created in CreateEnemyVisuals to ensure visibility
    
    // Equip default weapon
    EquipDefaultWeapon(enemy);
    
    std::cout << "Created enemy of type " << static_cast<int>(type) << " at position (" 
              << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
    
    return enemy;
}

Entity EnemyAISystem::CreateGrunt(glm::vec3 position) {
    return CreateEnemy(EnemyType::GRUNT, position);
}

Entity EnemyAISystem::CreateSoldier(glm::vec3 position) {
    return CreateEnemy(EnemyType::SOLDIER, position);
}

void EnemyAISystem::ConfigureEnemyStats(Entity enemy, EnemyType type) {
    try {
        auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(enemy);
        
        switch (type) {
            case EnemyType::GRUNT:
                enemyComp.health = 50.0f;
                enemyComp.maxHealth = 50.0f;
                enemyComp.damage = 15.0f;
                enemyComp.moveSpeed = 2.0f;
                enemyComp.attackRange = 15.0f;
                enemyComp.detectionRange = 25.0f;
                enemyComp.attackCooldown = 2.0f;
                enemyComp.scoreValue = 10;
                break;
                
            case EnemyType::SOLDIER:
                enemyComp.health = 75.0f;
                enemyComp.maxHealth = 75.0f;
                enemyComp.damage = 25.0f;
                enemyComp.moveSpeed = 2.5f;
                enemyComp.attackRange = 20.0f;
                enemyComp.detectionRange = 30.0f;
                enemyComp.attackCooldown = 1.5f;
                enemyComp.scoreValue = 20;
                break;
                
            case EnemyType::HEAVY:
                enemyComp.health = 150.0f;
                enemyComp.maxHealth = 150.0f;
                enemyComp.damage = 40.0f;
                enemyComp.moveSpeed = 1.5f;
                enemyComp.attackRange = 25.0f;
                enemyComp.detectionRange = 35.0f;
                enemyComp.attackCooldown = 3.0f;
                enemyComp.scoreValue = 50;
                break;
                
            case EnemyType::SNIPER:
                enemyComp.health = 40.0f;
                enemyComp.maxHealth = 40.0f;
                enemyComp.damage = 60.0f;
                enemyComp.moveSpeed = 1.8f;
                enemyComp.attackRange = 50.0f;
                enemyComp.detectionRange = 60.0f;
                enemyComp.attackCooldown = 4.0f;
                enemyComp.scoreValue = 30;
                break;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error configuring enemy stats: " << e.what() << std::endl;
    }
}

void EnemyAISystem::CreateEnemyVisuals(Entity enemy, EnemyType type) {
    try {
        // Add a simple physics cube that will be visible
        extern PhysicsSystem* g_physicsSystem;
        if (!g_physicsSystem) {
            std::cerr << "No physics system for enemy visuals" << std::endl;
            return;
        }
        
        auto& transform = gCoordinator.GetComponent<TransformComponent>(enemy);
        
        // Create a physics cube which will be automatically rendered
        btRigidBody* body = g_physicsSystem->CreateCube(transform.position, 1.0f);
        if (body) {
            PhysicsComponent physics;
            physics.rigidBody = body;
            physics.syncTransform = true;
            gCoordinator.AddComponent(enemy, physics);
            
            std::cout << "Enemy physics cube created for visibility" << std::endl;
        }
        
        // Configure transform scale based on enemy type for visual differentiation
        switch (type) {
            case EnemyType::GRUNT:
                transform.scale = glm::vec3(0.8f, 1.8f, 0.6f); // Red-ish (small and tall)
                break;
            case EnemyType::SOLDIER:
                transform.scale = glm::vec3(1.0f, 1.8f, 0.8f); // Blue-ish (standard)
                break;
            case EnemyType::HEAVY:
                transform.scale = glm::vec3(1.4f, 2.0f, 1.2f); // Green-ish (large and bulky)
                break;
            case EnemyType::SNIPER:
                transform.scale = glm::vec3(0.7f, 2.2f, 0.5f); // Yellow-ish (tall and slim)
                break;
        }
        
        std::cout << "Enemy " << static_cast<int>(type) << " visuals configured with physics body" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error creating enemy visuals: " << e.what() << std::endl;
    }
}

void EnemyAISystem::CreateEnemyPhysics(Entity enemy, glm::vec3 position) {
    // This method is now integrated into CreateEnemyVisuals
    // Kept for potential future use or debugging
    std::cout << "CreateEnemyPhysics called but physics is handled in CreateEnemyVisuals" << std::endl;
}

void EnemyAISystem::EquipDefaultWeapon(Entity enemy) {
    if (!m_weaponSystem) return;
    
    try {
        auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(enemy);
        auto& transform = gCoordinator.GetComponent<TransformComponent>(enemy);
        
        // Create appropriate weapon based on enemy type
        Entity weapon;
        switch (enemyComp.type) {
            case EnemyType::GRUNT:
            case EnemyType::SOLDIER:
                weapon = m_weaponSystem->CreatePistol(transform.position);
                break;
            case EnemyType::HEAVY:
                weapon = m_weaponSystem->CreateRifle(transform.position);
                break;
            case EnemyType::SNIPER:
                weapon = m_weaponSystem->CreateWeapon(WeaponType::SNIPER, transform.position);
                break;
            default:
                weapon = m_weaponSystem->CreatePistol(transform.position);
                break;
        }
        
        // Equip the weapon
        m_weaponSystem->EquipWeapon(weapon, enemy);
        enemyComp.weaponId = weapon;
        
    } catch (const std::exception& e) {
        std::cerr << "Error equipping default weapon: " << e.what() << std::endl;
    }
}

void EnemyAISystem::UpdateEnemyState(Entity enemy, float deltaTime) {
    try {
        auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(enemy);
        
        // Update state timer
        enemyComp.stateTimer += deltaTime;
        
        // Get player if available
        Entity player = static_cast<Entity>(-1);
        if (m_playerSystem && m_playerSystem->HasPlayer()) {
            player = m_playerSystem->GetPlayer();
        }
        
        // Update distance to player and visibility
        if (player != static_cast<Entity>(-1)) {
            enemyComp.distanceToPlayer = GetDistanceToPlayer(enemy, player);
            enemyComp.canSeePlayer = CanSeePlayer(enemy, player);
            
            if (enemyComp.canSeePlayer) {
                auto& playerTransform = gCoordinator.GetComponent<TransformComponent>(player);
                enemyComp.lastKnownPlayerPosition = playerTransform.position;
                enemyComp.lostPlayerTime = 0.0f;
            } else {
                enemyComp.lostPlayerTime += deltaTime;
            }
        }
        
        // State machine
        switch (enemyComp.state) {
            case EnemyState::PATROL:
                UpdatePatrolBehavior(enemy, deltaTime);
                
                // Transition to chase if player is detected
                if (player != static_cast<Entity>(-1) && enemyComp.canSeePlayer && 
                    enemyComp.distanceToPlayer <= enemyComp.detectionRange) {
                    enemyComp.state = EnemyState::CHASE;
                    enemyComp.targetPlayerId = player;
                    enemyComp.stateTimer = 0.0f;
                }
                break;
                
            case EnemyState::CHASE:
                UpdateChaseBehavior(enemy, deltaTime);
                
                // Transition to attack if in range
                if (player != static_cast<Entity>(-1) && IsPlayerInAttackRange(enemy, player)) {
                    enemyComp.state = EnemyState::ATTACK;
                    enemyComp.stateTimer = 0.0f;
                }
                // Transition to search if lost player
                else if (!enemyComp.canSeePlayer && enemyComp.lostPlayerTime > 1.0f) {
                    enemyComp.state = EnemyState::SEARCHING;
                    enemyComp.stateTimer = 0.0f;
                }
                break;
                
            case EnemyState::ATTACK:
                UpdateAttackBehavior(enemy, deltaTime);
                
                // Transition back to chase if player moves out of range
                if (player != static_cast<Entity>(-1) && !IsPlayerInAttackRange(enemy, player)) {
                    enemyComp.state = EnemyState::CHASE;
                    enemyComp.stateTimer = 0.0f;
                }
                break;
                
            case EnemyState::SEARCHING:
                UpdateSearchBehavior(enemy, deltaTime);
                
                // Transition back to chase if player is spotted
                if (player != static_cast<Entity>(-1) && enemyComp.canSeePlayer) {
                    enemyComp.state = EnemyState::CHASE;
                    enemyComp.stateTimer = 0.0f;
                }
                // Transition back to patrol if search time exceeded
                else if (enemyComp.stateTimer > enemyComp.maxSearchTime) {
                    enemyComp.state = EnemyState::PATROL;
                    enemyComp.stateTimer = 0.0f;
                }
                break;
                
            case EnemyState::DEAD:
                // Do nothing, enemy is dead
                break;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error updating enemy state: " << e.what() << std::endl;
    }
}

void EnemyAISystem::UpdatePatrolBehavior(Entity enemy, float deltaTime) {
    try {
        auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(enemy);
        
        if (enemyComp.patrolPoints.empty()) return;
        
        glm::vec3 targetPos = GetNextPatrolPosition(enemy);
        MoveTowards(enemy, targetPos, deltaTime);
        
        // Check if reached patrol point
        auto& transform = gCoordinator.GetComponent<TransformComponent>(enemy);
        float distanceToTarget = glm::length(transform.position - targetPos);
        
        if (distanceToTarget < 1.0f) {
            // Wait at patrol point
            if (enemyComp.stateTimer > enemyComp.patrolWaitTime) {
                enemyComp.currentPatrolPoint = (enemyComp.currentPatrolPoint + 1) % enemyComp.patrolPoints.size();
                enemyComp.stateTimer = 0.0f;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error updating patrol behavior: " << e.what() << std::endl;
    }
}

void EnemyAISystem::UpdateChaseBehavior(Entity enemy, float deltaTime) {
    try {
        auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(enemy);
        
        // Chase towards last known player position
        MoveTowards(enemy, enemyComp.lastKnownPlayerPosition, deltaTime);
        
    } catch (const std::exception& e) {
        std::cerr << "Error updating chase behavior: " << e.what() << std::endl;
    }
}

void EnemyAISystem::UpdateAttackBehavior(Entity enemy, float deltaTime) {
    try {
        auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(enemy);
        
        // Update attack cooldown
        enemyComp.lastAttackTime += deltaTime;
        
        // Attack if cooldown is ready
        if (enemyComp.lastAttackTime >= enemyComp.attackCooldown) {
            Entity player = enemyComp.targetPlayerId;
            if (player != static_cast<Entity>(-1)) {
                AttackPlayer(enemy, player);
                enemyComp.lastAttackTime = 0.0f;
            }
        }
        
        // Face the player (simple rotation towards player)
        if (enemyComp.targetPlayerId != static_cast<Entity>(-1)) {
            try {
                auto& transform = gCoordinator.GetComponent<TransformComponent>(enemy);
                auto& playerTransform = gCoordinator.GetComponent<TransformComponent>(enemyComp.targetPlayerId);
                
                glm::vec3 direction = GetDirectionToTarget(transform.position, playerTransform.position);
                float yaw = atan2(direction.x, direction.z);
                transform.rotation.y = glm::degrees(yaw);
                
            } catch (...) {
                // Player might not exist anymore
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error updating attack behavior: " << e.what() << std::endl;
    }
}

void EnemyAISystem::UpdateSearchBehavior(Entity enemy, float deltaTime) {
    try {
        auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(enemy);
        
        // Move towards last known player position
        MoveTowards(enemy, enemyComp.lastKnownPlayerPosition, deltaTime);
        
    } catch (const std::exception& e) {
        std::cerr << "Error updating search behavior: " << e.what() << std::endl;
    }
}

bool EnemyAISystem::CanSeePlayer(Entity enemy, Entity player) {
    try {
        auto& enemyTransform = gCoordinator.GetComponent<TransformComponent>(enemy);
        auto& playerTransform = gCoordinator.GetComponent<TransformComponent>(player);
        auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(enemy);
        
        // Simple line-of-sight check based on distance
        float distance = glm::length(playerTransform.position - enemyTransform.position);
        return distance <= enemyComp.detectionRange;
        
        // TODO: Implement proper raycast for line-of-sight
        
    } catch (const std::exception& e) {
        std::cerr << "Error checking if enemy can see player: " << e.what() << std::endl;
        return false;
    }
}

bool EnemyAISystem::IsPlayerInAttackRange(Entity enemy, Entity player) {
    try {
        auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(enemy);
        float distance = GetDistanceToPlayer(enemy, player);
        return distance <= enemyComp.attackRange;
        
    } catch (const std::exception& e) {
        std::cerr << "Error checking attack range: " << e.what() << std::endl;
        return false;
    }
}

float EnemyAISystem::GetDistanceToPlayer(Entity enemy, Entity player) {
    try {
        auto& enemyTransform = gCoordinator.GetComponent<TransformComponent>(enemy);
        auto& playerTransform = gCoordinator.GetComponent<TransformComponent>(player);
        
        return glm::length(playerTransform.position - enemyTransform.position);
        
    } catch (const std::exception& e) {
        std::cerr << "Error getting distance to player: " << e.what() << std::endl;
        return 999.0f;
    }
}

glm::vec3 EnemyAISystem::GetNextPatrolPosition(Entity enemy) {
    try {
        auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(enemy);
        
        if (enemyComp.patrolPoints.empty()) {
            auto& transform = gCoordinator.GetComponent<TransformComponent>(enemy);
            return transform.position;
        }
        
        return enemyComp.patrolPoints[enemyComp.currentPatrolPoint];
        
    } catch (const std::exception& e) {
        std::cerr << "Error getting patrol position: " << e.what() << std::endl;
        return glm::vec3(0.0f);
    }
}

void EnemyAISystem::MoveTowards(Entity enemy, glm::vec3 targetPosition, float deltaTime) {
    try {
        auto& transform = gCoordinator.GetComponent<TransformComponent>(enemy);
        auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(enemy);
        
        glm::vec3 direction = GetDirectionToTarget(transform.position, targetPosition);
        glm::vec3 movement = direction * enemyComp.moveSpeed * deltaTime;
        
        transform.position += movement;
        
        // Update physics if present
        try {
            auto& physics = gCoordinator.GetComponent<PhysicsComponent>(enemy);
            if (physics.rigidBody) {
                btTransform physicsTransform;
                physicsTransform.setOrigin(btVector3(transform.position.x, transform.position.y, transform.position.z));
                physics.rigidBody->setWorldTransform(physicsTransform);
                physics.rigidBody->activate(true);
            }
        } catch (...) {
            // No physics component
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error moving enemy: " << e.what() << std::endl;
    }
}

glm::vec3 EnemyAISystem::GetDirectionToTarget(glm::vec3 from, glm::vec3 to) {
    glm::vec3 direction = to - from;
    if (glm::length(direction) > 0.0f) {
        direction = glm::normalize(direction);
        direction.y = 0.0f; // Keep movement on ground plane
        if (glm::length(direction) > 0.0f) {
            direction = glm::normalize(direction);
        }
    }
    return direction;
}

void EnemyAISystem::AttackPlayer(Entity enemy, Entity player) {
    try {
        auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(enemy);
        auto& enemyTransform = gCoordinator.GetComponent<TransformComponent>(enemy);
        auto& playerTransform = gCoordinator.GetComponent<TransformComponent>(player);
        
        // Calculate shoot direction
        glm::vec3 shootDirection = GetDirectionToTarget(enemyTransform.position, playerTransform.position);
        glm::vec3 shootOrigin = enemyTransform.position;
        shootOrigin.y += 1.5f; // Shoulder height
        
        // Shoot weapon if available
        if (m_weaponSystem && enemyComp.weaponId != static_cast<Entity>(-1)) {
            m_weaponSystem->Shoot(enemyComp.weaponId, shootOrigin, shootDirection);
            std::cout << "Enemy " << enemy << " shoots at player!" << std::endl;
        } else {
            // Direct damage as fallback
            try {
                auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(player);
                playerComp.health -= enemyComp.damage;
                std::cout << "Enemy " << enemy << " attacks player for " << enemyComp.damage << " damage!" << std::endl;
            } catch (...) {
                // Player component not found
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error attacking player: " << e.what() << std::endl;
    }
}

int EnemyAISystem::GetAliveEnemyCount() const {
    int count = 0;
    for (auto const& entity : m_Entities) {
        try {
            auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(entity);
            if (enemyComp.isAlive && enemyComp.state != EnemyState::DEAD) {
                count++;
            }
        } catch (...) {
            // Entity might not exist anymore
        }
    }
    return count;
}

std::vector<Entity> EnemyAISystem::GetEnemiesInRoom(int roomIndex) const {
    std::vector<Entity> enemiesInRoom;
    
    // TODO: Implement room-based enemy tracking
    // For now, return all alive enemies
    for (auto const& entity : m_Entities) {
        try {
            auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(entity);
            if (enemyComp.isAlive && enemyComp.state != EnemyState::DEAD) {
                enemiesInRoom.push_back(entity);
            }
        } catch (...) {
            // Entity might not exist anymore
        }
    }
    
    return enemiesInRoom;
}

// Add a new method to handle enemy taking damage
void EnemyAISystem::DamageEnemy(Entity enemy, float damage, Entity attacker) {
    try {
        auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(enemy);
        
        if (!enemyComp.isAlive || enemyComp.state == EnemyState::DEAD) {
            return; // Already dead
        }
        
        // Apply damage
        enemyComp.health -= damage;
        
        std::cout << "Enemy " << enemy << " takes " << damage << " damage! Health: " 
                  << enemyComp.health << "/" << enemyComp.maxHealth << std::endl;
        
        // Check if enemy dies
        if (enemyComp.health <= 0.0f) {
            KillEnemy(enemy, attacker);
        } else {
            // Enemy is hurt but not dead - become aggressive
            if (enemyComp.state == EnemyState::PATROL) {
                enemyComp.state = EnemyState::CHASE;
                enemyComp.targetPlayerId = attacker;
                enemyComp.stateTimer = 0.0f;
                std::cout << "Enemy " << enemy << " becomes aggressive and starts chasing!" << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error damaging enemy: " << e.what() << std::endl;
    }
}

void EnemyAISystem::KillEnemy(Entity enemy, Entity killer) {
    try {
        auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(enemy);
        
        // Mark enemy as dead
        enemyComp.isAlive = false;
        enemyComp.state = EnemyState::DEAD;
        enemyComp.health = 0.0f;
        
        std::cout << "Enemy " << enemy << " has been killed!" << std::endl;
        
        // Award score to killer if it's the player
        if (m_playerSystem && m_playerSystem->HasPlayer()) {
            Entity player = m_playerSystem->GetPlayer();
            if (killer == player) {
                try {
                    auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(player);
                    playerComp.score += enemyComp.scoreValue;
                    playerComp.kills++;
                    std::cout << "Player gains " << enemyComp.scoreValue << " points! Total score: " << playerComp.score << std::endl;
                } catch (...) {
                    // Player component not found
                }
            }
        }
        
        // Make the enemy fall over by disabling physics or changing its appearance
        try {
            auto& physics = gCoordinator.GetComponent<PhysicsComponent>(enemy);
            if (physics.rigidBody) {
                // Make the dead enemy fall over
                physics.rigidBody->applyTorqueImpulse(btVector3(5.0f, 0.0f, 0.0f));
            }
        } catch (...) {
            // No physics component
        }
        
        // Change visual appearance to indicate death (make it darker/smaller)
        try {
            auto& transform = gCoordinator.GetComponent<TransformComponent>(enemy);
            transform.scale *= 0.8f; // Make it slightly smaller
            std::cout << "Dead enemy visual updated" << std::endl;
        } catch (...) {
            // No transform component
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error killing enemy: " << e.what() << std::endl;
    }
}

// Additional methods for completeness
void EnemyAISystem::SetPatrolPoints(Entity enemy, const std::vector<glm::vec3>& points) {
    try {
        auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(enemy);
        enemyComp.patrolPoints = points;
        enemyComp.currentPatrolPoint = 0;
    } catch (const std::exception& e) {
        std::cerr << "Error setting patrol points: " << e.what() << std::endl;
    }
}

void EnemyAISystem::SetTarget(Entity enemy, Entity target) {
    try {
        auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(enemy);
        enemyComp.targetPlayerId = target;
        enemyComp.state = EnemyState::CHASE;
        enemyComp.stateTimer = 0.0f;
    } catch (const std::exception& e) {
        std::cerr << "Error setting target: " << e.what() << std::endl;
    }
}