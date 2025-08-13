#ifndef GAMESTATECOMPONENT_H
#define GAMESTATECOMPONENT_H

#include <string>

enum class GameMode {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER,
    VICTORY,
    LOADING
};

struct GameStateComponent {
    GameMode currentMode = GameMode::MENU;
    GameMode previousMode = GameMode::MENU;
    
    // Game session data
    float gameTime = 0.0f;
    int totalScore = 0;
    int totalKills = 0;
    int totalDeaths = 0;
    
    // Level progression
    int currentLevel = 1;
    int maxLevel = 5;
    bool levelComplete = false;
    
    // UI state
    bool showHUD = true;
    bool showPauseMenu = false;
    bool showGameOverScreen = false;
    bool showVictoryScreen = false;
    
    // Game settings
    float difficulty = 1.0f; // Multiplier for enemy health/damage
    bool godMode = false; // For testing
    bool infiniteAmmo = false; // For testing
    
    // Performance stats
    float frameRate = 60.0f;
    int entityCount = 0;
    int activeEnemies = 0;
    int activeProjectiles = 0;
};

#endif // GAMESTATECOMPONENT_H