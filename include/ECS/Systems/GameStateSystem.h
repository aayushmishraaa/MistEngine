#ifndef GAMESTATESYSTEM_H
#define GAMESTATESYSTEM_H

#include "../System.h"
#include "../Components/GameStateComponent.h"
#include <memory>

class PlayerSystem;
class EnemyAISystem;
class UIManager;

class GameStateSystem : public System {
public:
    GameStateSystem();
    
    void Init(PlayerSystem* playerSystem, EnemyAISystem* enemySystem, UIManager* uiManager);
    void Update(float deltaTime) override;
    
    // Game management
    Entity CreateGameState();
    void StartGame();
    void PauseGame();
    void ResumeGame();
    void EndGame(bool victory = false);
    void RestartGame();
    
    // Level management
    void LoadLevel(int levelNumber);
    void CompleteLevel();
    void NextLevel();
    
    // Game queries
    GameMode GetCurrentGameMode() const;
    bool IsGameActive() const;
    bool IsGamePaused() const;
    
private:
    PlayerSystem* m_playerSystem;
    EnemyAISystem* m_enemySystem;
    UIManager* m_uiManager;
    
    Entity m_gameStateEntity;
    
    // Game state updates
    void UpdateGameplay(float deltaTime);
    void UpdateUI(float deltaTime);
    void CheckWinConditions();
    void CheckLossConditions();
    
    // Statistics
    void UpdateGameStats(float deltaTime);
    void UpdatePerformanceStats();
};

#endif // GAMESTATESYSTEM_H