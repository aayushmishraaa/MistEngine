#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <glm/glm.hpp>
#include "ECS/Entity.h"
#include <memory>

// Forward declarations to avoid header conflicts
struct GLFWwindow;
class Renderer;
class UIManager;
class PhysicsSystem;
class PlayerSystem;
class WeaponSystem;
class BotSystem;

enum class GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER
};

class GameManager {
public:
    GameManager();
    ~GameManager();
    
    void Initialize(GLFWwindow* window, Renderer* renderer, UIManager* uiManager, PhysicsSystem* physicsSystem);
    void SetSystems(std::shared_ptr<PlayerSystem> playerSystem, std::shared_ptr<WeaponSystem> weaponSystem, std::shared_ptr<BotSystem> botSystem);
    void Update(float deltaTime);
    void HandleInput();
    
    // Game state management
    void StartGame();
    void PauseGame();
    void EndGame();
    void RestartGame();
    
    // Game mode
    void ToggleGameMode();
    bool IsGameMode() const { return m_IsGameMode; }
    
    // Entity management
    void CreatePlayer();
    void CreateBot(const glm::vec3& position);
    void SpawnRandomBots(int count);
    void CleanupDeadEntities();
    
    GameState GetGameState() const { return m_GameState; }

private:
    GLFWwindow* m_Window = nullptr;
    Renderer* m_Renderer = nullptr;
    UIManager* m_UIManager = nullptr;
    PhysicsSystem* m_PhysicsSystem = nullptr;
    
    // FPS Game systems
    std::shared_ptr<PlayerSystem> m_PlayerSystem;
    std::shared_ptr<WeaponSystem> m_WeaponSystem;
    std::shared_ptr<BotSystem> m_BotSystem;
    
    GameState m_GameState = GameState::MENU;
    bool m_IsGameMode = false;
    
    // Game entities
    Entity m_PlayerEntity = 0;
    bool m_HasPlayer = false;
    
    // Bot management
    float m_BotSpawnTimer = 0.0f;
    float m_BotSpawnInterval = 10.0f; // Spawn a bot every 10 seconds
    int m_MaxBots = 5;
    int m_CurrentBotCount = 0;
    
    // Key press tracking
    bool m_F3Pressed = false;
    bool m_EscPressed = false;
    
    void UpdateGameplay(float deltaTime);
    void UpdateBotSpawning(float deltaTime);
    Entity CreateMeshEntity(const glm::vec3& position, const glm::vec3& color = glm::vec3(1.0f));
    void EnableGameMode(bool enable);
};

#endif // GAMEMANAGER_H