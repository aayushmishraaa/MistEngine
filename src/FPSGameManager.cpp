#include "FPSGameManager.h"
#include "UIManager.h"  // Add this include
#include "ECS/Coordinator.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/PlayerSystem.h"
#include "ECS/Systems/WeaponSystem.h"
#include "ECS/Systems/ProjectileSystem.h"
#include "ECS/Systems/EnemyAISystem.h"
#include "ECS/Systems/GameStateSystem.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/WeaponComponent.h"
#include "ECS/Components/ProjectileComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include "ECS/Components/GameStateComponent.h"
#include "InputManager.h"
#include "Camera.h"
#include "PhysicsSystem.h"
#include <iostream>
#include <random>

// Forward declare UIManager to avoid OpenGL header conflicts
class UIManager;

extern Coordinator gCoordinator;

FPSGameManager::FPSGameManager()
    : m_inputManager(nullptr)
    , m_camera(nullptr)
    , m_uiManager(nullptr)
    , m_physicsSystem(nullptr)
    , m_initialized(false)
    , m_gameStarted(false)
{
}

FPSGameManager::~FPSGameManager() {
    Shutdown();
}

bool FPSGameManager::Initialize(InputManager* inputManager, Camera* camera, UIManager* uiManager, PhysicsSystem* physicsSystem) {
    if (m_initialized) {
        std::cout << "FPSGameManager already initialized" << std::endl;
        return true;
    }
    
    // Store references
    m_inputManager = inputManager;
    m_camera = camera;
    m_uiManager = uiManager;
    m_physicsSystem = physicsSystem;
    
    if (!m_inputManager || !m_camera || !m_uiManager || !m_physicsSystem) {
        std::cerr << "FPSGameManager: Missing required dependencies" << std::endl;
        return false;
    }
    
    // Register FPS components with ECS
    try {
        gCoordinator.RegisterComponent<PlayerComponent>();
        gCoordinator.RegisterComponent<WeaponComponent>();
        gCoordinator.RegisterComponent<ProjectileComponent>();
        gCoordinator.RegisterComponent<EnemyComponent>();
        gCoordinator.RegisterComponent<GameStateComponent>();
        
        std::cout << "FPS components registered successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error registering FPS components: " << e.what() << std::endl;
        return false;
    }
    
    // Register and setup FPS systems
    RegisterFPSSystems();
    SetupSystemDependencies();
    
    m_initialized = true;
    AddConsoleMessage("FPS Game Manager initialized successfully");
    
    return true;
}

void FPSGameManager::Shutdown() {
    if (!m_initialized) return;
    
    m_playerSystem.reset();
    m_weaponSystem.reset();
    m_projectileSystem.reset();
    m_enemySystem.reset();
    m_gameStateSystem.reset();
    
    m_initialized = false;
    m_gameStarted = false;
    
    AddConsoleMessage("FPS Game Manager shutdown complete");
}

void FPSGameManager::RegisterFPSSystems() {
    try {
        // Create systems
        m_playerSystem = gCoordinator.RegisterSystem<PlayerSystem>();
        m_weaponSystem = gCoordinator.RegisterSystem<WeaponSystem>();
        m_projectileSystem = gCoordinator.RegisterSystem<ProjectileSystem>();
        m_enemySystem = gCoordinator.RegisterSystem<EnemyAISystem>();
        m_gameStateSystem = gCoordinator.RegisterSystem<GameStateSystem>();
        
        // Set system signatures
        
        // PlayerSystem signature
        Signature playerSignature;
        playerSignature.set(gCoordinator.GetComponentType<TransformComponent>());
        playerSignature.set(gCoordinator.GetComponentType<PlayerComponent>());
        gCoordinator.SetSystemSignature<PlayerSystem>(playerSignature);
        
        // WeaponSystem signature
        Signature weaponSignature;
        weaponSignature.set(gCoordinator.GetComponentType<TransformComponent>());
        weaponSignature.set(gCoordinator.GetComponentType<WeaponComponent>());
        gCoordinator.SetSystemSignature<WeaponSystem>(weaponSignature);
        
        // ProjectileSystem signature
        Signature projectileSignature;
        projectileSignature.set(gCoordinator.GetComponentType<TransformComponent>());
        projectileSignature.set(gCoordinator.GetComponentType<ProjectileComponent>());
        gCoordinator.SetSystemSignature<ProjectileSystem>(projectileSignature);
        
        // EnemyAISystem signature
        Signature enemySignature;
        enemySignature.set(gCoordinator.GetComponentType<TransformComponent>());
        enemySignature.set(gCoordinator.GetComponentType<EnemyComponent>());
        gCoordinator.SetSystemSignature<EnemyAISystem>(enemySignature);
        
        // GameStateSystem signature
        Signature gameStateSignature;
        gameStateSignature.set(gCoordinator.GetComponentType<GameStateComponent>());
        gCoordinator.SetSystemSignature<GameStateSystem>(gameStateSignature);
        
        AddConsoleMessage("FPS systems registered successfully");
        
    } catch (const std::exception& e) {
        std::cerr << "Error registering FPS systems: " << e.what() << std::endl;
    }
}

void FPSGameManager::SetupSystemDependencies() {
    try {
        // Initialize system dependencies
        m_playerSystem->Init(m_camera, m_inputManager);
        m_weaponSystem->Init(m_playerSystem.get());
        m_enemySystem->Init(m_playerSystem.get(), m_weaponSystem.get());
        m_gameStateSystem->Init(m_playerSystem.get(), m_enemySystem.get(), m_uiManager);
        
        AddConsoleMessage("System dependencies configured successfully");
        
    } catch (const std::exception& e) {
        std::cerr << "Error setting up system dependencies: " << e.what() << std::endl;
    }
}

void FPSGameManager::Update(float deltaTime) {
    if (!m_initialized) return;
    
    // Update all FPS systems (with safety checks)
    try {
        if (m_playerSystem) m_playerSystem->Update(deltaTime);
        if (m_weaponSystem) m_weaponSystem->Update(deltaTime);
        if (m_projectileSystem) m_projectileSystem->Update(deltaTime);
        if (m_enemySystem) m_enemySystem->Update(deltaTime);
        if (m_gameStateSystem) m_gameStateSystem->Update(deltaTime);
    } catch (const std::exception& e) {
        AddConsoleMessage("Error updating FPS systems: " + std::string(e.what()));
    }
    
    // Handle input safely
    if (m_gameStarted && m_inputManager) {
        try {
            // Handle shooting
            if (m_inputManager->IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                HandleShooting();
            }
            
            // Handle pause toggle
            static bool escWasPressed = false;
            bool escIsPressed = m_inputManager->IsKeyPressed(GLFW_KEY_ESCAPE);
            
            if (escIsPressed && !escWasPressed) {
                TogglePause();
            }
            escWasPressed = escIsPressed;
            
        } catch (const std::exception& e) {
            AddConsoleMessage("Error handling input: " + std::string(e.what()));
        }
    }
    
    // Handle game start from SPACE (keeping this as backup)
    if (!m_gameStarted && m_inputManager) {
        try {
            static bool spaceWasPressed = false;
            bool spaceIsPressed = m_inputManager->IsKeyPressed(GLFW_KEY_SPACE);
            
            if (spaceIsPressed && !spaceWasPressed) {
                StartNewGame();
            }
            spaceWasPressed = spaceIsPressed;
        } catch (const std::exception& e) {
            AddConsoleMessage("Error handling space key: " + std::string(e.what()));
        }
    }
}

void FPSGameManager::HandleShooting() {
    if (!m_camera || !m_physicsSystem) return;
    
    try {
        // Create bullet sphere at camera position
        glm::vec3 bulletOrigin = m_camera->Position;
        glm::vec3 bulletDirection = m_camera->Front;
        
        // Create tiny sphere for bullet
        btRigidBody* bulletBody = m_physicsSystem->CreateSphere(
            bulletOrigin, 0.1f, // Very small radius
            0.1f // Light mass
        );
        
        if (bulletBody) {
            // Set bullet velocity
            glm::vec3 bulletVelocity = bulletDirection * 50.0f; // Fast bullet
            bulletBody->setLinearVelocity(btVector3(
                bulletVelocity.x, bulletVelocity.y, bulletVelocity.z));
            
            // Bullet will automatically be rendered as a small sphere
            AddConsoleMessage("Bullet fired!");
            
            // TODO: Create proper projectile entity with ProjectileComponent
            // For now, just creating physics sphere is sufficient for demo
        }
        
    } catch (const std::exception& e) {
        AddConsoleMessage("Error firing bullet: " + std::string(e.what()));
    }
}

void FPSGameManager::StartNewGame() {
    if (!m_initialized) {
        AddConsoleMessage("ERROR: FPS Game Manager not initialized!");
        return;
    }
    
    AddConsoleMessage("=== STARTING SAFE FPS GAME ===");
    
    try {
        // Set game started flag FIRST
        m_gameStarted = true;
        
        // Move camera to safe starting position
        if (m_camera) {
            AddConsoleMessage("Setting camera to safe FPS position...");
            m_camera->Position = glm::vec3(0.0f, 3.0f, 8.0f);  // Further back for safety
            m_camera->Front = glm::vec3(0.0f, 0.0f, -1.0f);     // Looking forward
            AddConsoleMessage("Camera positioned safely");
        }
        
        // Switch to gameplay mode
        if (m_inputManager) {
            AddConsoleMessage("Switching to FPS input mode...");
            m_inputManager->EnableSceneEditorMode(false);
            m_inputManager->SetInputContext(InputContext::GAME_PLAY);
            AddConsoleMessage("Input mode switched successfully");
        }
        
        // Create safe minimal level first
        AddConsoleMessage("Creating safe minimal level...");
        CreateSafeLevel();
        
        // Create player WITHOUT complex systems to avoid crashes
        AddConsoleMessage("Creating simple player...");
        CreateSimplePlayer(glm::vec3(0.0f, 2.0f, 0.0f));
        
        AddConsoleMessage("=== SAFE FPS GAME STARTED ===");
        AddConsoleMessage("Look around with mouse, move with WASD");
        AddConsoleMessage("Left-click to shoot at enemy cubes!");
        
    } catch (const std::exception& e) {
        AddConsoleMessage("CRASH PREVENTED: " + std::string(e.what()));
        m_gameStarted = false;
    } catch (...) {
        AddConsoleMessage("UNKNOWN CRASH PREVENTED in StartNewGame");
        m_gameStarted = false;
    }
}

void FPSGameManager::CreateSafeLevel() {
    try {
        AddConsoleMessage("Creating walls and floor...");
        CreateRoomWalls();
        
        AddConsoleMessage("Creating enemy cubes...");
        CreateSimpleEnemies();
        
        AddConsoleMessage("Safe level created successfully");
        
    } catch (const std::exception& e) {
        AddConsoleMessage("Error creating safe level: " + std::string(e.what()));
    } catch (...) {
        AddConsoleMessage("Unknown error in CreateSafeLevel");
    }
}

void FPSGameManager::CreateRoomWalls() {
    if (!m_uiManager) {
        AddConsoleMessage("No UI Manager for wall entities");
        return;
    }
    
    try {
        AddConsoleMessage("Creating rendered walls using ECS system...");
        
        // Create floor plane
        m_uiManager->CreateGamePlane();
        AddConsoleMessage("Floor plane created and should be visible");
        
        // Create 4 wall cubes that will be positioned around the room
        for (int i = 0; i < 4; i++) {
            m_uiManager->CreateGameCube(); // This creates a full ECS entity with physics and rendering
            AddConsoleMessage("Wall cube " + std::to_string(i+1) + " created using ECS");
        }
        
        AddConsoleMessage("All walls created using ECS - they should be visible!");
        
    } catch (const std::exception& e) {
        AddConsoleMessage("Error creating walls: " + std::string(e.what()));
    }
}

void FPSGameManager::CreateSimpleEnemies() {
    if (!m_uiManager) {
        AddConsoleMessage("No UI Manager for enemy entities");
        return;
    }
    
    try {
        AddConsoleMessage("Creating enemy cubes using ECS system...");
        
        // Create 5 enemy cubes using the proven ECS system
        for (int i = 0; i < 5; i++) {
            m_uiManager->CreateGameCube(); // This creates a full ECS entity that's guaranteed to be visible
            AddConsoleMessage("Enemy cube " + std::to_string(i+1) + " created using ECS");
        }
        
        AddConsoleMessage("All enemy cubes created using ECS - they should be visible!");
        
    } catch (const std::exception& e) {
        AddConsoleMessage("Error creating enemies: " + std::string(e.what()));
    }
}

void FPSGameManager::CreateSimplePlayer(const glm::vec3& spawnPosition) {
    try {
        // Create simple player without complex systems
        if (m_physicsSystem) {
            // Create player physics body
            btRigidBody* playerBody = m_physicsSystem->CreateCube(spawnPosition, 1.0f);
            if (playerBody) {
                AddConsoleMessage("Player physics body created");
            }
        }
        
        AddConsoleMessage("Simple player created successfully");
        
    } catch (const std::exception& e) {
        AddConsoleMessage("Error creating player: " + std::string(e.what()));
    }
}

void FPSGameManager::PauseGame() {
    if (m_gameStateSystem && IsGameActive()) {
        m_gameStateSystem->PauseGame();
        AddConsoleMessage("Game paused");
    }
}

void FPSGameManager::ResumeGame() {
    if (m_gameStateSystem && IsGamePaused()) {
        m_gameStateSystem->ResumeGame();
        AddConsoleMessage("Game resumed");
    }
}

void FPSGameManager::RestartGame() {
    if (m_gameStateSystem) {
        m_gameStateSystem->RestartGame();
        m_gameStarted = false;
        AddConsoleMessage("Game restarted");
    }
}

void FPSGameManager::QuitGame() {
    if (m_gameStateSystem) {
        m_gameStateSystem->EndGame(false);
        AddConsoleMessage("Game quit");
    }
}

bool FPSGameManager::IsGameActive() const {
    // Use the simple game started flag as primary check
    return m_gameStarted && m_initialized;
}

bool FPSGameManager::IsGamePaused() const {
    if (!m_gameStateSystem) {
        return false; // Can't be paused if no game state system
    }
    return m_gameStateSystem->IsGamePaused();
}

void FPSGameManager::TogglePause() {
    if (IsGamePaused()) {
        ResumeGame();
    } else if (IsGameActive()) {
        PauseGame();
    }
}

void FPSGameManager::SpawnTestEnemy(const glm::vec3& position) {
    if (m_enemySystem) {
        Entity enemy = m_enemySystem->CreateGrunt(position);
        AddConsoleMessage("Test enemy spawned at (" + std::to_string(position.x) + ", " + std::to_string(position.y) + ", " + std::to_string(position.z) + ")");
    }
}

void FPSGameManager::GivePlayerWeapon(int weaponType) {
    if (!m_playerSystem || !m_weaponSystem || !m_playerSystem->HasPlayer()) {
        return;
    }
    
    Entity player = m_playerSystem->GetPlayer();
    auto& playerTransform = gCoordinator.GetComponent<TransformComponent>(player);
    
    Entity weapon;
    switch (weaponType) {
        case 0: weapon = m_weaponSystem->CreatePistol(playerTransform.position); break;
        case 1: weapon = m_weaponSystem->CreateRifle(playerTransform.position); break;
        case 2: weapon = m_weaponSystem->CreateWeapon(WeaponType::SHOTGUN, playerTransform.position); break;
        case 3: weapon = m_weaponSystem->CreateWeapon(WeaponType::SNIPER, playerTransform.position); break;
        default: return;
    }
    
    m_weaponSystem->EquipWeapon(weapon, player);
    AddConsoleMessage("Weapon equipped to player");
}

void FPSGameManager::PrintGameStats() {
    if (!m_initialized) return;
    
    AddConsoleMessage("=== GAME STATISTICS ===");
    
    if (m_enemySystem) {
        int aliveEnemies = m_enemySystem->GetAliveEnemyCount();
        AddConsoleMessage("Alive enemies: " + std::to_string(aliveEnemies));
    }
    
    if (m_playerSystem && m_playerSystem->HasPlayer()) {
        try {
            Entity player = m_playerSystem->GetPlayer();
            auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(player);
            AddConsoleMessage("Player Health: " + std::to_string(playerComp.health));
            AddConsoleMessage("Player Score: " + std::to_string(playerComp.score));
            AddConsoleMessage("Player Kills: " + std::to_string(playerComp.kills));
        } catch (...) {
            AddConsoleMessage("Could not retrieve player stats");
        }
    }
    
    AddConsoleMessage("========================");
}

void FPSGameManager::AddConsoleMessage(const std::string& message) {
    std::cout << "[FPS] " << message << std::endl;
    
    // Also add to UI console if available
    if (m_uiManager) {
        // m_uiManager->AddConsoleMessage("[FPS] " + message);
    }
}