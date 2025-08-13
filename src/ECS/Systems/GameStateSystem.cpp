#include "ECS/Systems/GameStateSystem.h"
#include "ECS/Systems/PlayerSystem.h"
#include "ECS/Systems/EnemyAISystem.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/GameStateComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include <iostream>

// Forward declare UIManager to avoid OpenGL header conflicts
class UIManager;

extern Coordinator gCoordinator;

GameStateSystem::GameStateSystem() 
    : m_playerSystem(nullptr)
    , m_enemySystem(nullptr)
    , m_uiManager(nullptr)
    , m_gameStateEntity(static_cast<Entity>(-1))
{
}

void GameStateSystem::Init(PlayerSystem* playerSystem, EnemyAISystem* enemySystem, UIManager* uiManager) {
    m_playerSystem = playerSystem;
    m_enemySystem = enemySystem;
    m_uiManager = uiManager;
}

void GameStateSystem::Update(float deltaTime) {
    if (m_gameStateEntity == static_cast<Entity>(-1)) {
        return; // No game state entity
    }
    
    try {
        auto& gameState = gCoordinator.GetComponent<GameStateComponent>(m_gameStateEntity);
        
        // Update game time
        if (gameState.currentMode == GameMode::PLAYING) {
            gameState.gameTime += deltaTime;
        }
        
        // Update based on current game mode
        switch (gameState.currentMode) {
            case GameMode::PLAYING:
                UpdateGameplay(deltaTime);
                break;
                
            case GameMode::PAUSED:
                // Game is paused, don't update gameplay
                break;
                
            case GameMode::GAME_OVER:
            case GameMode::VICTORY:
                // Handle end game state
                break;
                
            case GameMode::MENU:
            case GameMode::LOADING:
                // Handle menu/loading states
                break;
        }
        
        // Always update UI and stats
        UpdateUI(deltaTime);
        UpdateGameStats(deltaTime);
        UpdatePerformanceStats();
        
    } catch (const std::exception& e) {
        std::cerr << "Error updating game state: " << e.what() << std::endl;
    }
}

Entity GameStateSystem::CreateGameState() {
    Entity gameState = gCoordinator.CreateEntity();
    
    // Add GameState component
    GameStateComponent gameStateComp;
    gameStateComp.currentMode = GameMode::MENU;
    gameStateComp.currentLevel = 1;
    gameStateComp.maxLevel = 5;
    gameStateComp.difficulty = 1.0f;
    gCoordinator.AddComponent(gameState, gameStateComp);
    
    m_gameStateEntity = gameState;
    
    std::cout << "Game state created with ID: " << gameState << std::endl;
    return gameState;
}

void GameStateSystem::StartGame() {
    if (m_gameStateEntity == static_cast<Entity>(-1)) {
        CreateGameState();
    }
    
    try {
        auto& gameState = gCoordinator.GetComponent<GameStateComponent>(m_gameStateEntity);
        
        gameState.previousMode = gameState.currentMode;
        gameState.currentMode = GameMode::PLAYING;
        gameState.gameTime = 0.0f;
        gameState.totalScore = 0;
        gameState.totalKills = 0;
        gameState.totalDeaths = 0;
        gameState.showHUD = true;
        gameState.showPauseMenu = false;
        gameState.showGameOverScreen = false;
        gameState.showVictoryScreen = false;
        
        std::cout << "Game started!" << std::endl;
        
        // Create player if needed
        if (m_playerSystem && !m_playerSystem->HasPlayer()) {
            m_playerSystem->CreatePlayer(glm::vec3(0.0f, 1.0f, 0.0f));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error starting game: " << e.what() << std::endl;
    }
}

void GameStateSystem::PauseGame() {
    if (m_gameStateEntity == static_cast<Entity>(-1)) return;
    
    try {
        auto& gameState = gCoordinator.GetComponent<GameStateComponent>(m_gameStateEntity);
        
        if (gameState.currentMode == GameMode::PLAYING) {
            gameState.previousMode = gameState.currentMode;
            gameState.currentMode = GameMode::PAUSED;
            gameState.showPauseMenu = true;
            
            std::cout << "Game paused" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error pausing game: " << e.what() << std::endl;
    }
}

void GameStateSystem::ResumeGame() {
    if (m_gameStateEntity == static_cast<Entity>(-1)) return;
    
    try {
        auto& gameState = gCoordinator.GetComponent<GameStateComponent>(m_gameStateEntity);
        
        if (gameState.currentMode == GameMode::PAUSED) {
            gameState.currentMode = gameState.previousMode;
            gameState.showPauseMenu = false;
            
            std::cout << "Game resumed" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error resuming game: " << e.what() << std::endl;
    }
}

void GameStateSystem::EndGame(bool victory) {
    if (m_gameStateEntity == static_cast<Entity>(-1)) return;
    
    try {
        auto& gameState = gCoordinator.GetComponent<GameStateComponent>(m_gameStateEntity);
        
        gameState.previousMode = gameState.currentMode;
        gameState.currentMode = victory ? GameMode::VICTORY : GameMode::GAME_OVER;
        gameState.showHUD = false;
        
        if (victory) {
            gameState.showVictoryScreen = true;
            std::cout << "Victory! Game completed in " << gameState.gameTime << " seconds" << std::endl;
            std::cout << "Final Score: " << gameState.totalScore << ", Kills: " << gameState.totalKills << std::endl;
        } else {
            gameState.showGameOverScreen = true;
            gameState.totalDeaths++;
            std::cout << "Game Over! Final Score: " << gameState.totalScore << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error ending game: " << e.what() << std::endl;
    }
}

void GameStateSystem::RestartGame() {
    if (m_gameStateEntity == static_cast<Entity>(-1)) return;
    
    try {
        auto& gameState = gCoordinator.GetComponent<GameStateComponent>(m_gameStateEntity);
        
        // Reset game state
        gameState.currentMode = GameMode::LOADING;
        gameState.gameTime = 0.0f;
        gameState.totalScore = 0;
        gameState.totalKills = 0;
        gameState.levelComplete = false;
        gameState.showHUD = true;
        gameState.showPauseMenu = false;
        gameState.showGameOverScreen = false;
        gameState.showVictoryScreen = false;
        
        std::cout << "Game restarting..." << std::endl;
        
        // TODO: Clean up existing entities and reload level
        
        StartGame();
        
    } catch (const std::exception& e) {
        std::cerr << "Error restarting game: " << e.what() << std::endl;
    }
}

void GameStateSystem::LoadLevel(int levelNumber) {
    if (m_gameStateEntity == static_cast<Entity>(-1)) return;
    
    try {
        auto& gameState = gCoordinator.GetComponent<GameStateComponent>(m_gameStateEntity);
        
        gameState.currentLevel = levelNumber;
        gameState.levelComplete = false;
        
        std::cout << "Loading level " << levelNumber << std::endl;
        
        // TODO: Implement level loading logic
        // This would spawn enemies, setup level geometry, etc.
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading level: " << e.what() << std::endl;
    }
}

void GameStateSystem::CompleteLevel() {
    if (m_gameStateEntity == static_cast<Entity>(-1)) return;
    
    try {
        auto& gameState = gCoordinator.GetComponent<GameStateComponent>(m_gameStateEntity);
        
        gameState.levelComplete = true;
        
        std::cout << "Level " << gameState.currentLevel << " completed!" << std::endl;
        
        // Check if this was the final level
        if (gameState.currentLevel >= gameState.maxLevel) {
            EndGame(true); // Victory
        } else {
            // Proceed to next level
            NextLevel();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error completing level: " << e.what() << std::endl;
    }
}

void GameStateSystem::NextLevel() {
    if (m_gameStateEntity == static_cast<Entity>(-1)) return;
    
    try {
        auto& gameState = gCoordinator.GetComponent<GameStateComponent>(m_gameStateEntity);
        
        if (gameState.currentLevel < gameState.maxLevel) {
            LoadLevel(gameState.currentLevel + 1);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error going to next level: " << e.what() << std::endl;
    }
}

GameMode GameStateSystem::GetCurrentGameMode() const {
    if (m_gameStateEntity == static_cast<Entity>(-1)) return GameMode::MENU;
    
    try {
        auto& gameState = gCoordinator.GetComponent<GameStateComponent>(m_gameStateEntity);
        return gameState.currentMode;
    } catch (...) {
        return GameMode::MENU;
    }
}

bool GameStateSystem::IsGameActive() const {
    GameMode mode = GetCurrentGameMode();
    return mode == GameMode::PLAYING || mode == GameMode::PAUSED;
}

bool GameStateSystem::IsGamePaused() const {
    return GetCurrentGameMode() == GameMode::PAUSED;
}

void GameStateSystem::UpdateGameplay(float deltaTime) {
    // Check win/loss conditions
    CheckWinConditions();
    CheckLossConditions();
    
    // Update player stats from game state
    if (m_playerSystem && m_playerSystem->HasPlayer()) {
        try {
            Entity player = m_playerSystem->GetPlayer();
            auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(player);
            auto& gameState = gCoordinator.GetComponent<GameStateComponent>(m_gameStateEntity);
            
            // Sync player stats with game state
            gameState.totalScore = playerComp.score;
            gameState.totalKills = playerComp.kills;
            
        } catch (...) {
            // Player or game state not found
        }
    }
}

void GameStateSystem::UpdateUI(float deltaTime) {
    // TODO: Update UI elements based on game state
    // This could update HUD elements, score displays, etc.
}

void GameStateSystem::CheckWinConditions() {
    // Win condition: All enemies defeated in current level
    if (m_enemySystem) {
        int aliveEnemies = m_enemySystem->GetAliveEnemyCount();
        if (aliveEnemies == 0) {
            CompleteLevel();
        }
    }
}

void GameStateSystem::CheckLossConditions() {
    // Loss condition: Player is dead
    if (m_playerSystem && m_playerSystem->HasPlayer()) {
        try {
            Entity player = m_playerSystem->GetPlayer();
            auto& playerComp = gCoordinator.GetComponent<PlayerComponent>(player);
            
            if (!playerComp.isAlive) {
                EndGame(false); // Game over
            }
            
        } catch (...) {
            // Player not found
        }
    }
}

void GameStateSystem::UpdateGameStats(float deltaTime) {
    try {
        auto& gameState = gCoordinator.GetComponent<GameStateComponent>(m_gameStateEntity);
        
        // Update frame rate (simple moving average)
        static float frameTimeAccum = 0.0f;
        static int frameCount = 0;
        
        frameTimeAccum += deltaTime;
        frameCount++;
        
        if (frameTimeAccum >= 1.0f) { // Update every second
            gameState.frameRate = frameCount / frameTimeAccum;
            frameTimeAccum = 0.0f;
            frameCount = 0;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error updating game stats: " << e.what() << std::endl;
    }
}

void GameStateSystem::UpdatePerformanceStats() {
    try {
        auto& gameState = gCoordinator.GetComponent<GameStateComponent>(m_gameStateEntity);
        
        // Count active entities
        int entityCount = 0;
        int activeEnemies = 0;
        int activeProjectiles = 0;
        
        // Count entities (simplified)
        if (m_enemySystem) {
            activeEnemies = m_enemySystem->GetAliveEnemyCount();
        }
        
        // TODO: Count projectiles and other entities
        
        gameState.entityCount = entityCount;
        gameState.activeEnemies = activeEnemies;
        gameState.activeProjectiles = activeProjectiles;
        
    } catch (const std::exception& e) {
        std::cerr << "Error updating performance stats: " << e.what() << std::endl;
    }
}