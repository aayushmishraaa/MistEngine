#include "ECS/Systems/WeaponSystem.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/WeaponComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/HealthComponent.h"
#include "ECS/Components/ProjectileComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Systems/ProjectileSystem.h"
#include "Camera.h"
#include <iostream>
#include <GLFW/glfw3.h>

extern Coordinator gCoordinator;

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
    
    // Check for left mouse button press
    bool shooting = glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    
    if (shooting) {
        for (auto const& entity : m_Entities) {
            auto& weapon = gCoordinator.GetComponent<WeaponComponent>(entity);
            
            // Use deltaTime-based timing
            weapon.lastShotTime += deltaTime;
            
            // Check if weapon can fire
            if (weapon.lastShotTime < weapon.fireRate) continue;
            if (weapon.isReloading) continue;
            if (weapon.ammo <= 0) {
                // Auto reload when out of ammo
                weapon.isReloading = true;
                weapon.lastReloadTime = 0.0f;
                std::cout << "Out of ammo! Reloading..." << std::endl;
                continue;
            }
            
            // Fire!
            weapon.lastShotTime = 0.0f; // Reset shot timer
            weapon.ammo--;
            
            // Calculate fire origin and direction
            glm::vec3 fireOrigin = m_Camera->Position;
            glm::vec3 fireDirection = glm::normalize(m_Camera->Front);
            
            // Create visible projectile
            FireProjectile(fireOrigin, fireDirection, weapon.damage, entity);
            
            std::cout << "BANG! Player fired projectile. Ammo: " << weapon.ammo << "/" << weapon.maxAmmo << std::endl;
        }
    }
}

void WeaponSystem::ProcessReloading(float deltaTime) {
    // Check for R key press to reload
    static bool rKeyPressed = false;
    bool rKeyDown = glfwGetKey(m_Window, GLFW_KEY_R) == GLFW_PRESS;
    
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

void WeaponSystem::FireProjectile(const glm::vec3& origin, const glm::vec3& direction, float damage, Entity owner) {
    if (m_ProjectileSystem) {
        // Adjust origin to be slightly in front of camera
        glm::vec3 adjustedOrigin = origin + direction * 0.5f;
        float projectileSpeed = 50.0f; // Fast projectiles
        m_ProjectileSystem->CreateProjectile(adjustedOrigin, direction, projectileSpeed, damage, owner);
    }
}

bool WeaponSystem::PerformRaycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, Entity& hitEntity, glm::vec3& hitPoint) {
    // This method is kept for potential future use but we're using projectiles now
    return false;
}