#include "ECS/Systems/PlayerSystem.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "Camera.h"
#include "InputManager.h"
#include <iostream>
#include <algorithm>

extern Coordinator gCoordinator;

PlayerSystem::PlayerSystem() 
    : m_camera(nullptr)
    , m_inputManager(nullptr)
    , m_playerId(static_cast<Entity>(-1))
{
}

void PlayerSystem::Init(Camera* camera, InputManager* inputManager) {
    m_camera = camera;
    m_inputManager = inputManager;
}

void PlayerSystem::Update(float deltaTime) {
    if (!HasPlayer()) {
        return;
    }
    
    Entity player = GetPlayer();
    
    // Process input and movement
    ProcessMovementInput(player, deltaTime);
    ProcessMouseInput(player, deltaTime);
    ProcessWeaponInput(player, deltaTime);
    
    // Update player health and state
    UpdatePlayerHealth(player, deltaTime);
    
    // Synchronize camera with player
    UpdateCameraFromPlayer(player);
}

Entity PlayerSystem::CreatePlayer(glm::vec3 position) {
    Entity player = gCoordinator.CreateEntity();
    
    // Add Transform component
    TransformComponent transform;
    transform.position = position;
    transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    transform.scale = glm::vec3(1.0f);
    gCoordinator.AddComponent(player, transform);
    
    // Add Player component
    PlayerComponent playerComp;
    playerComp.health = 100.0f;
    playerComp.maxHealth = 100.0f;
    playerComp.moveSpeed = 5.0f;
    playerComp.mouseSensitivity = 0.1f;
    playerComp.isAlive = true;
    gCoordinator.AddComponent(player, playerComp);
    
    // Set as the main player
    m_playerId = player;
    
    // Set camera position to player position
    if (m_camera) {
        m_camera->Position = position;
        m_camera->Position.y += 1.7f; // Eye level height
    }
    
    std::cout << "Player created with ID: " << player << " at position (" 
              << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
    
    return player;
}

void PlayerSystem::ProcessMovementInput(Entity player, float deltaTime) {
    if (!m_inputManager || !m_camera) return;
    
    try {
        auto& transform = gCoordinator.GetComponent<TransformComponent>(player);
        auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(player);
        
        if (!playerComp.isAlive) return;
        
        glm::vec3 velocity(0.0f);
        bool isMoving = false;
        
        // Get camera direction vectors
        glm::vec3 front = m_camera->Front;
        glm::vec3 right = m_camera->Right;
        
        // Remove Y component for ground movement
        front.y = 0.0f;
        front = glm::normalize(front);
        right.y = 0.0f;
        right = glm::normalize(right);
        
        // WASD movement
        if (m_inputManager->IsKeyPressed(GLFW_KEY_W)) {
            velocity += front;
            isMoving = true;
        }
        if (m_inputManager->IsKeyPressed(GLFW_KEY_S)) {
            velocity -= front;
            isMoving = true;
        }
        if (m_inputManager->IsKeyPressed(GLFW_KEY_A)) {
            velocity -= right;
            isMoving = true;
        }
        if (m_inputManager->IsKeyPressed(GLFW_KEY_D)) {
            velocity += right;
            isMoving = true;
        }
        
        // Vertical movement (Q/E keys)
        if (m_inputManager->IsKeyPressed(GLFW_KEY_Q)) {
            velocity.y -= 1.0f;
            isMoving = true;
        }
        if (m_inputManager->IsKeyPressed(GLFW_KEY_E)) {
            velocity.y += 1.0f;
            isMoving = true;
        }
        
        // Normalize and apply speed
        if (isMoving) {
            if (glm::length(velocity) > 0.0f) {
                velocity = glm::normalize(velocity);
            }
            velocity *= playerComp.moveSpeed * deltaTime;
            
            // Apply movement to transform
            transform.position += velocity;
            
            // Update physics body if present
            try {
                auto& physics = gCoordinator.GetComponent<PhysicsComponent>(player);
                if (physics.rigidBody) {
                    btTransform physicsTransform;
                    physicsTransform.setOrigin(btVector3(transform.position.x, transform.position.y, transform.position.z));
                    physics.rigidBody->setWorldTransform(physicsTransform);
                    physics.rigidBody->activate(true);
                }
            } catch (...) {
                // No physics component, that's okay
            }
        }
        
        playerComp.isMoving = isMoving;
        playerComp.velocity = velocity;
        
    } catch (const std::exception& e) {
        std::cerr << "Error in ProcessMovementInput: " << e.what() << std::endl;
    }
}

void PlayerSystem::ProcessMouseInput(Entity player, float deltaTime) {
    if (!m_inputManager || !m_camera) return;
    
    try {
        auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(player);
        
        if (!playerComp.isAlive) return;
        
        // Get mouse movement from input manager
        glm::vec2 mouseDelta = m_inputManager->GetMouseDelta();
        
        // Apply mouse sensitivity
        mouseDelta *= playerComp.mouseSensitivity;
        
        // Update camera rotation
        m_camera->ProcessMouseMovement(mouseDelta.x, -mouseDelta.y);
        
    } catch (const std::exception& e) {
        std::cerr << "Error in ProcessMouseInput: " << e.what() << std::endl;
    }
}

void PlayerSystem::ProcessWeaponInput(Entity player, float deltaTime) {
    if (!m_inputManager) return;
    
    try {
        auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(player);
        
        if (!playerComp.isAlive) return;
        
        // Shooting input (left mouse button)
        playerComp.wantsToShoot = m_inputManager->IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
        
        // Reload input (R key)
        if (m_inputManager->IsKeyJustPressed(GLFW_KEY_R)) {
            playerComp.wantsToReload = true;
        }
        
        // Weapon switching (number keys)
        for (int i = 0; i < 9; ++i) {
            if (m_inputManager->IsKeyJustPressed(GLFW_KEY_1 + i)) {
                playerComp.wantsToSwitchWeapon = i;
                break;
            }
        }
        
        // Handle weapon actions
        if (playerComp.wantsToShoot) {
            HandleShooting(player);
        }
        if (playerComp.wantsToReload) {
            HandleReloading(player);
            playerComp.wantsToReload = false;
        }
        if (playerComp.wantsToSwitchWeapon >= 0) {
            HandleWeaponSwitching(player);
            playerComp.wantsToSwitchWeapon = -1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error in ProcessWeaponInput: " << e.what() << std::endl;
    }
}

void PlayerSystem::HandleShooting(Entity player) {
    // This will be implemented when WeaponSystem is created
    // For now, just mark that the player wants to shoot
    try {
        auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(player);
        // The WeaponSystem will handle the actual shooting logic
        
    } catch (const std::exception& e) {
        std::cerr << "Error in HandleShooting: " << e.what() << std::endl;
    }
}

void PlayerSystem::HandleReloading(Entity player) {
    // This will be implemented when WeaponSystem is created
    std::cout << "Player wants to reload" << std::endl;
}

void PlayerSystem::HandleWeaponSwitching(Entity player) {
    // This will be implemented when WeaponSystem is created
    try {
        auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(player);
        std::cout << "Player wants to switch to weapon " << playerComp.wantsToSwitchWeapon << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error in HandleWeaponSwitching: " << e.what() << std::endl;
    }
}

void PlayerSystem::UpdatePlayerHealth(Entity player, float deltaTime) {
    try {
        auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(player);
        
        // Clamp health to valid range
        playerComp.health = std::max(0.0f, std::min(playerComp.health, playerComp.maxHealth));
        
        // Check for death
        if (playerComp.health <= 0.0f && playerComp.isAlive) {
            HandlePlayerDeath(player);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error in UpdatePlayerHealth: " << e.what() << std::endl;
    }
}

void PlayerSystem::HandlePlayerDeath(Entity player) {
    try {
        auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(player);
        playerComp.isAlive = false;
        
        std::cout << "Player has died! Final score: " << playerComp.score << std::endl;
        
        // TODO: Trigger game over state
        
    } catch (const std::exception& e) {
        std::cerr << "Error in HandlePlayerDeath: " << e.what() << std::endl;
    }
}

void PlayerSystem::UpdateCameraFromPlayer(Entity player) {
    if (!m_camera) return;
    
    try {
        auto& transform = gCoordinator.GetComponent<TransformComponent>(player);
        
        // Update camera position to follow player
        m_camera->Position = transform.position;
        m_camera->Position.y += 1.7f; // Eye level height
        
    } catch (const std::exception& e) {
        std::cerr << "Error in UpdateCameraFromPlayer: " << e.what() << std::endl;
    }
}

void PlayerSystem::UpdatePlayerFromCamera(Entity player) {
    if (!m_camera) return;
    
    try {
        auto& transform = gCoordinator.GetComponent<TransformComponent>(player);
        
        // Update player position from camera (for editor mode)
        transform.position = m_camera->Position;
        transform.position.y -= 1.7f; // Subtract eye level height
        
    } catch (const std::exception& e) {
        std::cerr << "Error in UpdatePlayerFromCamera: " << e.what() << std::endl;
    }
}