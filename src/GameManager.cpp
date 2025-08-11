#include "GameManager.h"
#include "UIManager.h"
#include "PhysicsSystem.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/WeaponComponent.h"
#include "ECS/Components/BotComponent.h"
#include "ECS/Components/HealthComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Systems/PlayerSystem.h"
#include "ECS/Systems/WeaponSystem.h"
#include "ECS/Systems/BotSystem.h"
#include "Mesh.h"
#include "ShapeGenerator.h"
#include <iostream>
#include <random>

// GLFW must be included after other headers that might include OpenGL
#include <GLFW/glfw3.h>

extern Coordinator gCoordinator;

GameManager::GameManager() {
}

GameManager::~GameManager() {
}

void GameManager::Initialize(GLFWwindow* window, Renderer* renderer, UIManager* uiManager, PhysicsSystem* physicsSystem) {
    m_Window = window;
    m_Renderer = renderer;
    m_UIManager = uiManager;
    m_PhysicsSystem = physicsSystem;
    
    std::cout << "GameManager initialized!" << std::endl;
    std::cout << "Press F3 to toggle between Editor and Game modes" << std::endl;
}

void GameManager::SetSystems(std::shared_ptr<PlayerSystem> playerSystem, std::shared_ptr<WeaponSystem> weaponSystem, std::shared_ptr<BotSystem> botSystem) {
    m_PlayerSystem = playerSystem;
    m_WeaponSystem = weaponSystem;
    m_BotSystem = botSystem;
    std::cout << "GameManager: Systems set successfully" << std::endl;
}

void GameManager::Update(float deltaTime) {
    HandleInput();
    
    switch (m_GameState) {
        case GameState::MENU:
            // Could show main menu here
            break;
        case GameState::PLAYING:
            UpdateGameplay(deltaTime);
            break;
        case GameState::PAUSED:
            // Game is paused
            break;
        case GameState::GAME_OVER:
            // Handle game over state
            break;
    }
}

void GameManager::HandleInput() {
    // F3 to toggle game mode
    bool f3Down = glfwGetKey(m_Window, GLFW_KEY_F3) == GLFW_PRESS;
    if (f3Down && !m_F3Pressed) {
        ToggleGameMode();
    }
    m_F3Pressed = f3Down;
    
    // ESC to pause/unpause game
    bool escDown = glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (escDown && !m_EscPressed && m_IsGameMode) {
        if (m_GameState == GameState::PLAYING) {
            PauseGame();
        } else if (m_GameState == GameState::PAUSED) {
            StartGame();
        }
    }
    m_EscPressed = escDown;
}

void GameManager::ToggleGameMode() {
    m_IsGameMode = !m_IsGameMode;
    EnableGameMode(m_IsGameMode);
    
    if (m_IsGameMode) {
        StartGame();
        std::cout << "=== GAME MODE ACTIVATED ===" << std::endl;
        std::cout << "Controls: WASD to move, Mouse to look, Left click to shoot, R to reload, ESC to pause" << std::endl;
    } else {
        EndGame();
        std::cout << "=== EDITOR MODE ACTIVATED ===" << std::endl;
        std::cout << "Right-click and drag to look around, WASD/QE to move" << std::endl;
    }
}

void GameManager::StartGame() {
    m_GameState = GameState::PLAYING;
    
    // Create player if not exists
    if (!m_HasPlayer) {
        CreatePlayer();
    }
    
    // Spawn some initial bots
    if (m_CurrentBotCount == 0) {
        SpawnRandomBots(3);
    }
}

void GameManager::PauseGame() {
    m_GameState = GameState::PAUSED;
    std::cout << "Game paused. Press ESC to resume." << std::endl;
}

void GameManager::EndGame() {
    m_GameState = GameState::MENU;
}

void GameManager::RestartGame() {
    // Reset player health
    if (m_HasPlayer) {
        try {
            auto& health = gCoordinator.GetComponent<HealthComponent>(m_PlayerEntity);
            health.Reset();
        } catch (...) {}
    }
    
    StartGame();
}

void GameManager::CreatePlayer() {
    m_PlayerEntity = gCoordinator.CreateEntity();
    
    // Transform - spawn at origin, slightly above ground
    TransformComponent transform;
    transform.position = glm::vec3(0.0f, 2.0f, 5.0f);
    transform.scale = glm::vec3(0.8f, 1.8f, 0.8f); // Human-like proportions
    gCoordinator.AddComponent(m_PlayerEntity, transform);
    
    // Player component
    PlayerComponent player;
    gCoordinator.AddComponent(m_PlayerEntity, player);
    
    // Weapon component
    WeaponComponent weapon;
    gCoordinator.AddComponent(m_PlayerEntity, weapon);
    
    // Health component
    HealthComponent health;
    health.maxHealth = 100.0f;
    health.currentHealth = 100.0f;
    gCoordinator.AddComponent(m_PlayerEntity, health);
    
    // Create visual representation (optional - player is first-person)
    auto playerMesh = CreateMeshEntity(transform.position, glm::vec3(0.0f, 0.0f, 1.0f)); // Blue for player
    
    // Physics (capsule would be better, but use box for now)
    if (m_PhysicsSystem) {
        btRigidBody* body = m_PhysicsSystem->CreateCube(transform.position, 70.0f); // 70kg human
        PhysicsComponent physics;
        physics.rigidBody = body;
        physics.syncTransform = true;
        gCoordinator.AddComponent(m_PlayerEntity, physics);
    }
    
    m_HasPlayer = true;
    std::cout << "Player created with entity ID: " << m_PlayerEntity << std::endl;
}

void GameManager::CreateBot(const glm::vec3& position) {
    Entity botEntity = gCoordinator.CreateEntity();
    
    // Transform
    TransformComponent transform;
    transform.position = position;
    transform.scale = glm::vec3(0.8f, 1.6f, 0.8f); // Slightly shorter than player
    gCoordinator.AddComponent(botEntity, transform);
    
    // Bot component
    BotComponent bot;
    bot.state = BotState::IDLE;
    bot.patrolCenter = position;
    gCoordinator.AddComponent(botEntity, bot);
    
    // Health component
    HealthComponent health;
    health.maxHealth = 75.0f; // Less health than player
    health.currentHealth = 75.0f;
    gCoordinator.AddComponent(botEntity, health);
    
    // Create visual representation
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    generateCubeMesh(vertices, indices);
    std::vector<Texture> textures;
    Mesh* mesh = new Mesh(vertices, indices, textures);
    
    RenderComponent render;
    render.renderable = mesh;
    render.visible = true;
    gCoordinator.AddComponent(botEntity, render);
    
    // Physics
    if (m_PhysicsSystem) {
        btRigidBody* body = m_PhysicsSystem->CreateCube(position, 60.0f); // 60kg bot
        PhysicsComponent physics;
        physics.rigidBody = body;
        physics.syncTransform = true;
        gCoordinator.AddComponent(botEntity, physics);
    }
    
    m_CurrentBotCount++;
    std::cout << "Bot created at (" << position.x << ", " << position.y << ", " << position.z << ") with entity ID: " << botEntity << std::endl;
}

void GameManager::SpawnRandomBots(int count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDis(-15.0f, 15.0f);
    std::uniform_real_distribution<float> heightDis(1.0f, 3.0f);
    
    for (int i = 0; i < count; ++i) {
        glm::vec3 spawnPos(posDis(gen), heightDis(gen), posDis(gen));
        CreateBot(spawnPos);
    }
}

void GameManager::UpdateGameplay(float deltaTime) {
    // Update bot spawning
    UpdateBotSpawning(deltaTime);
    
    // Cleanup dead entities
    CleanupDeadEntities();
    
    // Check player death
    if (m_HasPlayer) {
        try {
            auto& health = gCoordinator.GetComponent<HealthComponent>(m_PlayerEntity);
            if (health.isDead) {
                std::cout << "Player died! Game Over!" << std::endl;
                m_GameState = GameState::GAME_OVER;
                
                // Auto-respawn after 3 seconds
                health.timeSinceDeath += deltaTime;
                if (health.timeSinceDeath >= 3.0f) {
                    RestartGame();
                }
            }
        } catch (...) {}
    }
}

void GameManager::UpdateBotSpawning(float deltaTime) {
    if (m_CurrentBotCount < m_MaxBots) {
        m_BotSpawnTimer += deltaTime;
        if (m_BotSpawnTimer >= m_BotSpawnInterval) {
            // Spawn a new bot at random location
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> posDis(-20.0f, 20.0f);
            
            glm::vec3 spawnPos(posDis(gen), 2.0f, posDis(gen));
            CreateBot(spawnPos);
            
            m_BotSpawnTimer = 0.0f;
        }
    }
}

void GameManager::CleanupDeadEntities() {
    // This is a simplified cleanup - in a real game you'd want a proper entity cleanup system
    // For now, we'll just decrement bot count when bots die
    // (The actual cleanup would happen in the ECS system)
}

Entity GameManager::CreateMeshEntity(const glm::vec3& position, const glm::vec3& color) {
    // This would create a simple colored cube mesh for visualization
    // For now, return 0 (no entity) since it's optional
    return 0;
}

void GameManager::EnableGameMode(bool enable) {
    // Set cursor mode
    if (enable) {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Lock cursor for FPS
    } else {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Free cursor for editor
    }
    
    // Notify systems about mode change
    if (m_PlayerSystem) {
        m_PlayerSystem->SetGameMode(enable);
    }
    if (m_WeaponSystem) {
        m_WeaponSystem->SetGameMode(enable);
    }
    if (m_BotSystem) {
        m_BotSystem->SetGameMode(enable);
    }
}