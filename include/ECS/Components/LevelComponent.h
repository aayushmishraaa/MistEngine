#ifndef LEVELCOMPONENT_H
#define LEVELCOMPONENT_H

#include <glm/glm.hpp>
#include <vector>
#include <string>

enum class RoomState {
    LOCKED,     // Cannot enter, enemies not spawned
    ACTIVE,     // Player can enter, enemies active
    CLEARED,    // All enemies defeated
    COMPLETED   // Player has exited room
};

struct Room {
    std::string name;
    RoomState state = RoomState::LOCKED;
    
    // Geometry bounds
    glm::vec3 center{0.0f};
    glm::vec3 size{10.0f, 3.0f, 10.0f}; // Width, Height, Depth
    
    // Enemy spawning
    std::vector<glm::vec3> enemySpawnPoints;
    std::vector<int> spawnedEnemyIds; // Entity IDs of enemies in this room
    int totalEnemies = 0;
    int remainingEnemies = 0;
    
    // Connections
    std::vector<int> connectedRooms; // Indices of connected rooms
    std::vector<glm::vec3> doorPositions; // Positions of doors/triggers
    
    // Completion rewards
    int scoreReward = 100;
    bool hasPickups = false;
};

struct LevelComponent {
    std::string levelName = "Level 1";
    int currentRoom = 0;
    std::vector<Room> rooms;
    
    // Level progression
    bool isCompleted = false;
    int totalScore = 0;
    int roomsCleared = 0;
    
    // Player spawn
    glm::vec3 playerSpawnPosition{0.0f, 1.0f, 0.0f};
    glm::vec3 playerSpawnRotation{0.0f};
    
    // Level bounds (for world limits)
    glm::vec3 worldMin{-50.0f, -10.0f, -50.0f};
    glm::vec3 worldMax{50.0f, 10.0f, 50.0f};
};

#endif // LEVELCOMPONENT_H