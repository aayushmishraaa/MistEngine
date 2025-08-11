#include "ECS/Systems/WeaponSystem.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/WeaponComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/HealthComponent.h"
#include "ECS/Components/ProjectileComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include <iostream>

extern Coordinator gCoordinator;
extern class PhysicsSystem* g_physicsSystem; // We'll need to set this up

void WeaponSystem::Init(GLFWwindow* window, Camera* camera) {
    m_Window = window;
    m_Camera = camera;
}

void WeaponSystem::Update(float deltaTime) {
    if (!m_IsGameMode) return;
    
    ProcessShooting(deltaTime);
    ProcessReloading(deltaTime);
}

void WeaponSystem::ProcessShooting(float deltaTime) {
    if (!m_Window || !m_Camera) return;
    
    // Check for left mouse button press (using glfwGetMouseButton)
    // Note: We need to include GLFW or use a different timing mechanism
    bool shooting = false; // Placeholder for now
    
    if (shooting) {
        for (auto const& entity : m_Entities) {
            auto& weapon = gCoordinator.GetComponent<WeaponComponent>(entity);
            
            // Use deltaTime-based timing instead of glfwGetTime
            static float timeSinceLastShot = 0.0f;
            timeSinceLastShot += deltaTime;
            
            // Check if weapon can fire
            if (timeSinceLastShot < weapon.fireRate) continue;
            if (weapon.isReloading) continue;
            if (weapon.ammo <= 0) {
                // Auto reload when out of ammo
                weapon.isReloading = true;
                weapon.lastReloadTime = 0.0f; // Reset to 0, will be updated in ProcessReloading
                std::cout << "Reloading..." << std::endl;
                continue;
            }
            
            // Fire!
            timeSinceLastShot = 0.0f;
            weapon.ammo--;
            
            // Calculate fire origin and direction
            glm::vec3 fireOrigin = m_Camera->Position + weapon.muzzleOffset;
            glm::vec3 fireDirection = m_Camera->Front;
            
            // Perform raycast for hitscan weapon
            Entity hitEntity;
            glm::vec3 hitPoint;
            if (PerformRaycast(fireOrigin, fireDirection, weapon.range, hitEntity, hitPoint)) {
                // Hit something!
                try {
                    auto& health = gCoordinator.GetComponent<HealthComponent>(hitEntity);
                    health.TakeDamage(weapon.damage);
                    std::cout << "Hit entity " << hitEntity << " for " << weapon.damage << " damage! Health: " << health.currentHealth << std::endl;
                } catch (...) {
                    // Entity doesn't have health component
                    std::cout << "Hit entity " << hitEntity << " (no health)" << std::endl;
                }
            }
            
            std::cout << "BANG! Ammo: " << weapon.ammo << "/" << weapon.maxAmmo << std::endl;
        }
    }
}

void WeaponSystem::ProcessReloading(float deltaTime) {
    // Check for R key press to reload - temporarily disabled due to GLFW dependency
    static bool rKeyPressed = false;
    bool rKeyDown = false; // Placeholder
    
    if (rKeyDown && !rKeyPressed) {
        for (auto const& entity : m_Entities) {
            auto& weapon = gCoordinator.GetComponent<WeaponComponent>(entity);
            
            if (!weapon.isReloading && weapon.ammo < weapon.maxAmmo) {
                weapon.isReloading = true;
                weapon.lastReloadTime = 0.0f;
                std::cout << "Reloading..." << std::endl;
            }
        }
    }
    rKeyPressed = rKeyDown;
    
    // Update reload progress
    for (auto const& entity : m_Entities) {
        auto& weapon = gCoordinator.GetComponent<WeaponComponent>(entity);
        
        if (weapon.isReloading) {
            weapon.lastReloadTime += deltaTime;
            if (weapon.lastReloadTime >= weapon.reloadTime) {
                weapon.ammo = weapon.maxAmmo;
                weapon.isReloading = false;
                std::cout << "Reload complete! Ammo: " << weapon.ammo << "/" << weapon.maxAmmo << std::endl;
            }
        }
    }
}

bool WeaponSystem::PerformRaycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, Entity& hitEntity, glm::vec3& hitPoint) {
    // Simple raycast - check against all entities with health components
    float closestDistance = maxDistance;
    bool foundHit = false;
    
    // Get all entities with health components (potential targets)
    for (Entity entity = 0; entity < 5000; ++entity) { // Max entity check - not ideal but works for now
        try {
            auto& health = gCoordinator.GetComponent<HealthComponent>(entity);
            auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
            
            // Skip dead entities
            if (health.isDead) continue;
            
            // Simple sphere collision check (assuming 1 unit radius)
            float radius = 1.0f;
            glm::vec3 toTarget = transform.position - origin;
            float distance = glm::length(toTarget);
            
            // Project point onto ray
            float t = glm::dot(toTarget, direction);
            if (t < 0.0f || t > maxDistance) continue; // Behind ray or too far
            
            glm::vec3 closestPoint = origin + direction * t;
            float distanceToRay = glm::length(transform.position - closestPoint);
            
            if (distanceToRay <= radius && t < closestDistance) {
                closestDistance = t;
                hitEntity = entity;
                hitPoint = closestPoint;
                foundHit = true;
            }
        } catch (...) {
            // Entity doesn't have required components
        }
    }
    
    return foundHit;
}}