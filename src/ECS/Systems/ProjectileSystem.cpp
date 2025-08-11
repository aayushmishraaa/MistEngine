#include "ECS/Systems/ProjectileSystem.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/ProjectileComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/HealthComponent.h"
#include "ECS/Components/BotComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "Mesh.h"
#include "ShapeGenerator.h"
#include <iostream>

extern Coordinator gCoordinator;

void ProjectileSystem::Update(float deltaTime) {
    if (!m_IsGameMode) return;
    
    UpdateProjectileMovement(deltaTime);
    CheckProjectileCollisions();
    CleanupExpiredProjectiles(deltaTime);
}

void ProjectileSystem::CreateProjectile(const glm::vec3& origin, const glm::vec3& direction, float speed, float damage, Entity owner) {
    Entity projectileEntity = gCoordinator.CreateEntity();
    
    // Transform
    TransformComponent transform;
    transform.position = origin;
    transform.scale = glm::vec3(0.1f, 0.1f, 0.1f); // Small projectile
    gCoordinator.AddComponent(projectileEntity, transform);
    
    // Projectile component
    ProjectileComponent projectile;
    projectile.velocity = direction * speed;
    projectile.damage = damage;
    projectile.owner = owner;
    projectile.hasOwner = true;
    projectile.lifetime = 5.0f;
    projectile.age = 0.0f;
    projectile.color = glm::vec3(1.0f, 1.0f, 0.0f); // Yellow projectile
    gCoordinator.AddComponent(projectileEntity, projectile);
    
    // Create visual representation - small glowing sphere
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    generateCubeMesh(vertices, indices); // Use small cube for now
    std::vector<Texture> textures;
    Mesh* mesh = new Mesh(vertices, indices, textures);
    
    RenderComponent render;
    render.renderable = mesh;
    render.visible = true;
    gCoordinator.AddComponent(projectileEntity, render);
    
    std::cout << "Projectile created by entity " << owner << " at position (" << 
                 origin.x << ", " << origin.y << ", " << origin.z << ")" << std::endl;
}

void ProjectileSystem::UpdateProjectileMovement(float deltaTime) {
    for (auto const& entity : m_Entities) {
        auto& projectile = gCoordinator.GetComponent<ProjectileComponent>(entity);
        auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
        
        // Move projectile
        transform.position += projectile.velocity * deltaTime;
        
        // Age projectile
        projectile.age += deltaTime;
    }
}

void ProjectileSystem::CheckProjectileCollisions() {
    for (auto const& projectileEntity : m_Entities) {
        auto& projectile = gCoordinator.GetComponent<ProjectileComponent>(projectileEntity);
        auto& projectileTransform = gCoordinator.GetComponent<TransformComponent>(projectileEntity);
        
        // Check collision against all entities with health components
        for (Entity entity = 0; entity < 5000; ++entity) {
            if (entity == projectileEntity) continue; // Don't hit self
            if (projectile.hasOwner && entity == projectile.owner) continue; // Don't hit owner
            
            try {
                auto& health = gCoordinator.GetComponent<HealthComponent>(entity);
                auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
                
                // Skip dead entities
                if (health.isDead) continue;
                
                // Simple sphere collision (1.5 unit radius for targets)
                float distance = glm::distance(projectileTransform.position, transform.position);
                if (distance <= 1.5f) {
                    // Hit!
                    health.TakeDamage(projectile.damage);
                    
                    // Check if target died from this hit
                    bool wasKilled = health.isDead;
                    
                    // Check if it's a bot or player
                    std::string targetType = "unknown";
                    bool isBot = false;
                    try {
                        auto& player = gCoordinator.GetComponent<PlayerComponent>(entity);
                        targetType = "Player";
                    } catch (...) {
                        try {
                            auto& bot = gCoordinator.GetComponent<BotComponent>(entity);
                            targetType = "Bot";
                            isBot = true;
                        } catch (...) {}
                    }
                    
                    std::cout << "Projectile hit " << targetType << " " << entity << " for " << projectile.damage << 
                                 " damage! Health: " << health.currentHealth << std::endl;
                    
                    // If a bot was killed by player projectile, increment kill count
                    if (wasKilled && isBot && projectile.hasOwner) {
                        try {
                            auto& killer = gCoordinator.GetComponent<PlayerComponent>(projectile.owner);
                            killer.killCount++;
                            std::cout << "Player killed bot " << entity << "! Kill count: " << killer.killCount << std::endl;
                        } catch (...) {
                            // Killer is not a player
                        }
                    }
                    
                    // Destroy projectile
                    gCoordinator.DestroyEntity(projectileEntity);
                    return; // Exit early since projectile is destroyed
                }
            } catch (...) {
                // Entity doesn't have health component
            }
        }
    }
}

void ProjectileSystem::CleanupExpiredProjectiles(float deltaTime) {
    std::vector<Entity> toDestroy;
    
    for (auto const& entity : m_Entities) {
        auto& projectile = gCoordinator.GetComponent<ProjectileComponent>(entity);
        
        if (projectile.age >= projectile.lifetime) {
            toDestroy.push_back(entity);
        }
    }
    
    // Destroy expired projectiles
    for (Entity entity : toDestroy) {
        gCoordinator.DestroyEntity(entity);
        std::cout << "Projectile " << entity << " expired and was destroyed" << std::endl;
    }
}