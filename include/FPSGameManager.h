#ifndef FPSGAMEMANAGER_H
#define FPSGAMEMANAGER_H

#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>

// Forward declarations
class PlayerSystem;
class WeaponSystem;
class ProjectileSystem;
class EnemyAISystem;
class GameStateSystem;
class InputManager;
class Camera;
class UIManager;
class PhysicsSystem;
class Coordinator;

class FPSGameManager {
public:
    FPSGameManager();
    ~FPSGameManager();
    
    // Initialization
    bool Initialize(InputManager* inputManager, Camera* camera, UIManager* uiManager, PhysicsSystem* physicsSystem);
    void Shutdown();
    
    // Game loop
    void Update(float deltaTime);
    
    // Game management
    void StartNewGame();
    void PauseGame();
    void ResumeGame();
    void RestartGame();
    void QuitGame();
    
    // Level management
    void LoadTestLevel();
    void SpawnEnemiesInRoom(const glm::vec3& roomCenter, int enemyCount);
    void CreatePlayerWithWeapons(const glm::vec3& spawnPosition);
    
    // Utility functions
    bool IsGameActive() const;
    bool IsGamePaused() const;
    void TogglePause();
    
    // Debug functions
    void SpawnTestEnemy(const glm::vec3& position);
    void GivePlayerWeapon(int weaponType);
    void PrintGameStats();
    
    // PUBLIC: System access for global interaction
    std::shared_ptr<EnemyAISystem> m_enemySystem;
    
private:
    // Systems
    std::shared_ptr<PlayerSystem> m_playerSystem;
    std::shared_ptr<WeaponSystem> m_weaponSystem;
    std::shared_ptr<ProjectileSystem> m_projectileSystem;
    std::shared_ptr<GameStateSystem> m_gameStateSystem;
    
    // External references
    InputManager* m_inputManager;
    Camera* m_camera;
    UIManager* m_uiManager;
    PhysicsSystem* m_physicsSystem;
    
    // Game state
    bool m_initialized;
    bool m_gameStarted;
    
    // Helper functions
    void RegisterFPSSystems();
    void SetupSystemDependencies();
    void CreateTestLevel();
    void SpawnPlayerAtPosition(const glm::vec3& position);
    void AddConsoleMessage(const std::string& message);
    
    // NEW: Safe level creation methods
    void CreateSafeLevel();
    void CreateRoomWalls();
    void CreateSimpleEnemies();
    void CreateSimplePlayer(const glm::vec3& spawnPosition);
    void HandleShooting();
};

#endif // FPSGAMEMANAGER_H