#include "ECS/Systems/ProjectileSystem.h"
#include "ECS/Systems/EnemyAISystem.h"
#include "ECS/Systems/PlayerSystem.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/ProjectileComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include <iostream>
#include <algorithm>

extern Coordinator gCoordinator;

ProjectileSystem::ProjectileSystem()
    : m_enemySystem(nullptr)
    , m_playerSystem(nullptr)
{
}

void ProjectileSystem::Init(EnemyAISystem* enemySystem, PlayerSystem* playerSystem) {
    m_enemySystem = enemySystem;
    m_playerSystem = playerSystem;
}

void ProjectileSystem::Update(float deltaTime) {
    // Process all projectiles
    std::vector<Entity> projectilesToDestroy;
    
    for (auto const& entity : m_Entities) {
        try {
            auto& projectile = gCoordinator.GetComponent<ProjectileComponent>(entity);
            auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
            
            if (projectile.hasHit) {
                projectilesToDestroy.push_back(entity);
                continue;
            }
            
            // Store old position for collision checking
            glm::vec3 oldPosition = transform.position;
            
            // Update physics
            UpdateProjectilePhysics(entity, deltaTime);
            
            // Update lifetime
            UpdateProjectileLifetime(entity, deltaTime);
            
            // Check if projectile should be destroyed due to lifetime
            if (projectile.currentLife >= projectile.lifeTime || 
                projectile.traveledDistance >= projectile.maxRange) {
                projectilesToDestroy.push_back(entity);
                continue;
            }
            
            // Check for collisions
            Entity hitEntity = CheckCollision(entity, oldPosition, transform.position);
            if (hitEntity != static_cast<Entity>(-1)) {
                // Handle hit
                DealDamage(entity, hitEntity, transform.position);
                projectile.hasHit = true;
                projectile.hitEntityId = hitEntity;
                projectile.hitPosition = transform.position;
                
                // Create hit effect
                CreateHitEffect(transform.position, hitEntity);
                
                // Destroy projectile (unless it penetrates)
                if (!projectile.penetrating) {
                    projectilesToDestroy.push_back(entity);
                }
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error updating projectile " << entity << ": " << e.what() << std::endl;
            projectilesToDestroy.push_back(entity);
        }
    }
    
    // Clean up destroyed projectiles
    for (Entity projectile : projectilesToDestroy) {
        DestroyProjectile(projectile);
    }
}

void ProjectileSystem::UpdateProjectilePhysics(Entity projectile, float deltaTime) {
    try {
        auto& projComp = gCoordinator.GetComponent<ProjectileComponent>(projectile);
        auto& transform = gCoordinator.GetComponent<TransformComponent>(projectile);
        
        // Apply gravity if enabled
        if (projComp.gravity > 0.0f) {
            projComp.velocity.y -= projComp.gravity * deltaTime;
        }
        
        // Calculate movement
        glm::vec3 movement = projComp.velocity * deltaTime;
        float movementDistance = glm::length(movement);
        
        // Update position
        transform.position += movement;
        
        // Update traveled distance
        projComp.traveledDistance += movementDistance;
        
        // Update direction based on current velocity (for gravity-affected projectiles)
        if (glm::length(projComp.velocity) > 0.0f) {
            projComp.direction = glm::normalize(projComp.velocity);
        }
        
        // Update transform rotation to match direction
        if (glm::length(projComp.direction) > 0.0f) {
            // Calculate rotation from direction vector
            float yaw = atan2(projComp.direction.x, projComp.direction.z);
            float pitch = asin(-projComp.direction.y);
            
            transform.rotation.x = glm::degrees(pitch);
            transform.rotation.y = glm::degrees(yaw);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error updating projectile physics: " << e.what() << std::endl;
    }
}

void ProjectileSystem::UpdateProjectileLifetime(Entity projectile, float deltaTime) {
    try {
        auto& projComp = gCoordinator.GetComponent<ProjectileComponent>(projectile);
        projComp.currentLife += deltaTime;
        
    } catch (const std::exception& e) {
        std::cerr << "Error updating projectile lifetime: " << e.what() << std::endl;
    }
}

Entity ProjectileSystem::CheckCollision(Entity projectileEntity, glm::vec3 oldPos, glm::vec3 newPos) {
    try {
        auto& projComp = gCoordinator.GetComponent<ProjectileComponent>(projectileEntity);

        // Build list of candidate targets from the appropriate system's entity set
        std::vector<Entity> candidates;

        if (projComp.isPlayerProjectile && m_enemySystem) {
            // Player projectiles check enemy entities
            for (auto const& e : m_enemySystem->m_Entities) {
                candidates.push_back(e);
            }
        } else if (!projComp.isPlayerProjectile && m_playerSystem) {
            // Enemy projectiles check player entities
            for (auto const& e : m_playerSystem->m_Entities) {
                candidates.push_back(e);
            }
        }

        for (Entity entity : candidates) {
            if (entity == projectileEntity) continue;

            try {
                auto& targetTransform = gCoordinator.GetComponent<TransformComponent>(entity);

                if (!IsValidTarget(projectileEntity, entity)) {
                    continue;
                }

                float collisionRadius = 0.5f;

                glm::vec3 projDirection = newPos - oldPos;
                glm::vec3 toTarget = targetTransform.position - oldPos;

                float projLength = glm::length(projDirection);
                if (projLength < 0.001f) continue;

                glm::vec3 projNormal = projDirection / projLength;
                float projectedDistance = glm::dot(toTarget, projNormal);

                if (projectedDistance < 0.0f || projectedDistance > projLength) {
                    continue;
                }

                glm::vec3 closestPoint = oldPos + projNormal * projectedDistance;
                float distanceToPath = glm::length(targetTransform.position - closestPoint);

                if (distanceToPath <= collisionRadius) {
                    return entity;
                }

            } catch (...) {
                continue;
            }
        }

        return static_cast<Entity>(-1);

    } catch (const std::exception& e) {
        std::cerr << "Error checking collision: " << e.what() << std::endl;
        return static_cast<Entity>(-1);
    }
}

bool ProjectileSystem::IsValidTarget(Entity projectileEntity, Entity targetEntity) {
    try {
        auto& projComp = gCoordinator.GetComponent<ProjectileComponent>(projectileEntity);
        
        // Don't hit the owner
        if (targetEntity == projComp.ownerId) {
            return false;
        }
        
        // Player projectiles can hit enemies
        if (projComp.isPlayerProjectile) {
            try {
                gCoordinator.GetComponent<EnemyComponent>(targetEntity);
                return true; // Target is an enemy
            } catch (...) {
                // Not an enemy
            }
        } else {
            // Enemy projectiles can hit player
            try {
                gCoordinator.GetComponent<PlayerComponent>(targetEntity);
                return true; // Target is a player
            } catch (...) {
                // Not a player
            }
        }
        
        return false; // Not a valid target
        
    } catch (const std::exception& e) {
        std::cerr << "Error checking valid target: " << e.what() << std::endl;
        return false;
    }
}

void ProjectileSystem::DealDamage(Entity projectileEntity, Entity targetEntity, glm::vec3 hitPoint) {
    try {
        auto& projComp = gCoordinator.GetComponent<ProjectileComponent>(projectileEntity);
        
        // Deal damage to player
        try {
            auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(targetEntity);
            playerComp.health -= projComp.damage;
            
            std::cout << "Player hit! Damage: " << projComp.damage << ", Health: " << playerComp.health << std::endl;
            
            // Update player score if killed by enemy
            if (playerComp.health <= 0 && !projComp.isPlayerProjectile) {
                // Player killed by enemy projectile
                playerComp.isAlive = false;
            }
            
            return;
        } catch (...) {
            // Not a player
        }
        
        // Deal damage to enemy
        try {
            auto& enemyComp = gCoordinator.GetComponent<EnemyComponent>(targetEntity);
            enemyComp.health -= projComp.damage;
            
            std::cout << "Enemy hit! Damage: " << projComp.damage << ", Health: " << enemyComp.health << std::endl;
            
            // Update player score if enemy killed by player
            if (enemyComp.health <= 0 && projComp.isPlayerProjectile) {
                enemyComp.isAlive = false;
                enemyComp.state = EnemyState::DEAD;
                
                // Award points to player
                if (projComp.ownerId != -1) {
                    try {
                        auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(projComp.ownerId);
                        playerComp.score += enemyComp.scoreValue;
                        playerComp.kills++;
                        
                        std::cout << "Enemy killed! Score: " << playerComp.score << ", Kills: " << playerComp.kills << std::endl;
                    } catch (...) {
                        // Owner is not a player
                    }
                }
            }
            
            return;
        } catch (...) {
            // Not an enemy
        }
        
        std::cout << "Projectile hit unknown entity " << targetEntity << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error dealing damage: " << e.what() << std::endl;
    }
}

void ProjectileSystem::CreateHitEffect(glm::vec3 position, Entity hitEntity) {
    // TODO: Create visual/audio effects for projectile hits
    // For now, just log the hit
    std::cout << "Hit effect at (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
}

void ProjectileSystem::CreateMuzzleFlash(glm::vec3 position, glm::vec3 direction) {
    // TODO: Create muzzle flash visual effect
    std::cout << "Muzzle flash at (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
}

void ProjectileSystem::DestroyProjectile(Entity projectileEntity) {
    try {
        gCoordinator.DestroyEntity(projectileEntity);
    } catch (const std::exception& e) {
        std::cerr << "Error destroying projectile: " << e.what() << std::endl;
    }
}