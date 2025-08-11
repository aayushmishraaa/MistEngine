#include "ECS/Systems/PlayerSystem.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "Camera.h"
#include <iostream>
#include <GLFW/glfw3.h>

extern Coordinator gCoordinator;

void PlayerSystem::Init(GLFWwindow* window, Camera* camera) {
    m_Window = window;
    m_Camera = camera;
    
    // Get initial cursor position
    glfwGetCursorPos(m_Window, &m_LastX, &m_LastY);
}

void PlayerSystem::Update(float deltaTime) {
    if (!m_IsGameMode) return;
    
    HandleInput(deltaTime);
    UpdateCamera();
}

void PlayerSystem::HandleInput(float deltaTime) {
    if (!m_Window || !m_Camera) return;
    
    ProcessMovement(deltaTime);
    ProcessMouseLook();
    ProcessJumping(deltaTime);
}

void PlayerSystem::ProcessMovement(float deltaTime) {
    for (auto const& entity : m_Entities) {
        auto& player = gCoordinator.GetComponent<PlayerComponent>(entity);
        auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
        
        glm::vec3 movement(0.0f);
        
        // Get camera forward and right vectors for movement
        glm::vec3 forward = m_Camera->Front;
        forward.y = 0.0f; // Remove Y component for ground movement
        forward = glm::normalize(forward);
        
        glm::vec3 right = m_Camera->Right;
        right.y = 0.0f; // Remove Y component for ground movement  
        right = glm::normalize(right);
        
        // WASD movement
        if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS)
            movement += forward;
        if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS)
            movement -= forward;
        if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS)
            movement -= right;
        if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS)
            movement += right;
        
        // Apply movement
        if (glm::length(movement) > 0.0f) {
            movement = glm::normalize(movement);
            
            // Apply to physics body if it exists (preferred)
            try {
                auto& physics = gCoordinator.GetComponent<PhysicsComponent>(entity);
                if (physics.rigidBody) {
                    btVector3 btMovement(movement.x * player.movementSpeed, 0.0f, movement.z * player.movementSpeed);
                    btVector3 currentVelocity = physics.rigidBody->getLinearVelocity();
                    btMovement.setY(currentVelocity.getY()); // Preserve Y velocity (gravity)
                    
                    physics.rigidBody->setLinearVelocity(btMovement);
                    physics.rigidBody->activate(true);
                    
                    // Update ground check for jumping
                    if (abs(currentVelocity.getY()) < 0.1f) {
                        player.isGrounded = true;
                    } else {
                        player.isGrounded = false;
                    }
                    return;
                }
            } catch (...) {
                // No physics component, apply to transform directly
            }
            
            // Fallback: direct transform movement
            glm::vec3 velocity = movement * player.movementSpeed * deltaTime;
            transform.position += velocity;
        }
    }
}

void PlayerSystem::ProcessMouseLook() {
    double xpos, ypos;
    glfwGetCursorPos(m_Window, &xpos, &ypos);
    
    if (m_FirstMouse) {
        m_LastX = xpos;
        m_LastY = ypos;
        m_FirstMouse = false;
    }
    
    double xoffset = xpos - m_LastX;
    double yoffset = m_LastY - ypos; // Reversed since y-coordinates go from bottom to top
    
    m_LastX = xpos;
    m_LastY = ypos;
    
    for (auto const& entity : m_Entities) {
        auto& player = gCoordinator.GetComponent<PlayerComponent>(entity);
        
        // Apply mouse sensitivity
        xoffset *= player.mouseSensitivity;
        yoffset *= player.mouseSensitivity;
        
        // Update camera with mouse input
        m_Camera->ProcessMouseMovement(static_cast<float>(xoffset), static_cast<float>(yoffset));
    }
}

void PlayerSystem::ProcessJumping(float deltaTime) {
    if (glfwGetKey(m_Window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        for (auto const& entity : m_Entities) {
            auto& player = gCoordinator.GetComponent<PlayerComponent>(entity);
            
            if (player.isGrounded && player.canJump) {
                try {
                    auto& physics = gCoordinator.GetComponent<PhysicsComponent>(entity);
                    if (physics.rigidBody) {
                        btVector3 jumpImpulse(0.0f, player.jumpForce, 0.0f);
                        physics.rigidBody->applyCentralImpulse(jumpImpulse);
                        player.isGrounded = false;
                        player.canJump = false;
                    }
                } catch (...) {
                    // No physics component
                }
            }
        }
    } else {
        // Reset jump ability when space is released
        for (auto const& entity : m_Entities) {
            auto& player = gCoordinator.GetComponent<PlayerComponent>(entity);
            player.canJump = true;
        }
    }
}

void PlayerSystem::UpdateCamera() {
    // Position camera at player position + offset
    for (auto const& entity : m_Entities) {
        auto& player = gCoordinator.GetComponent<PlayerComponent>(entity);
        auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
        
        glm::vec3 cameraPos = transform.position + player.cameraOffset;
        m_Camera->Position = cameraPos;
    }
}