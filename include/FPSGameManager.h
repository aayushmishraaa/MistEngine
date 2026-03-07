#ifndef FPSGAMEMANAGER_H
#define FPSGAMEMANAGER_H

#include <memory>
#include <vector>
#include <string>
#include <unordered_set>
#include <glm/glm.hpp>
#include "ECS/Entity.h"
#include "ECS/Components/EnemyComponent.h"

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

enum class PickupType {
    HEALTH,
    AMMO,
    WEAPON_SHOTGUN,
    WEAPON_RIFLE,
    WEAPON_SNIPER
};

enum class RoomState {
    LOCKED,
    ACTIVE,
    CLEARED
};

struct Room {
    glm::vec3 center;
    float width, height, depth;
    std::vector<std::pair<glm::vec3, int>> doors;
    RoomState state = RoomState::LOCKED;
    std::vector<Entity> spawnedEnemyIds;
};

struct LevelData {
    std::vector<Room> rooms;
    glm::vec3 playerSpawnPosition{0.0f, 0.0f, 3.0f};
};

struct HUDData {
    float health = 100, maxHealth = 100;
    int ammo = 0, maxAmmo = 0, reserveAmmo = 0;
    int score = 0, kills = 0;
    std::string weaponName = "None";
    int aliveEnemies = 0;
    int currentRoom = 0;
    bool isReloading = false;
    float reloadProgress = 0.0f;
};

class FPSGameManager {
public:
    FPSGameManager();
    ~FPSGameManager();

    bool Initialize(InputManager* inputManager, Camera* camera, UIManager* uiManager, PhysicsSystem* physicsSystem);
    void Shutdown();
    void Update(float deltaTime);

    void StartNewGame();
    void StopGame();
    void PauseGame();
    void ResumeGame();
    void RestartGame();
    void QuitGame();

    bool IsGameActive() const;
    bool IsGamePaused() const;
    void TogglePause();

    HUDData GetHUDData() const;

    void SpawnTestEnemy(const glm::vec3& position);
    void GivePlayerWeapon(int weaponType);
    void PrintGameStats();

    std::shared_ptr<EnemyAISystem> m_enemySystem;

private:
    std::shared_ptr<PlayerSystem> m_playerSystem;
    std::shared_ptr<WeaponSystem> m_weaponSystem;
    std::shared_ptr<ProjectileSystem> m_projectileSystem;
    std::shared_ptr<GameStateSystem> m_gameStateSystem;

    InputManager* m_inputManager;
    Camera* m_camera;
    UIManager* m_uiManager;
    PhysicsSystem* m_physicsSystem;

    bool m_initialized;
    bool m_gameStarted;

    std::vector<Entity> m_gameEntities;
    std::vector<std::pair<Entity, PickupType>> m_pickupEntities;

    glm::vec3 m_savedCameraPos{0.0f};
    float m_savedCameraYaw = -90.0f;
    float m_savedCameraPitch = 0.0f;
    bool m_savedOrbitMode = false;

    LevelData m_levelData;

    void RegisterFPSSystems();
    void SetupSystemDependencies();
    void AddConsoleMessage(const std::string& message);

    Entity CreateVisibleCube(const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& color);
    Entity CreateVisibleSphere(const glm::vec3& pos, float radius, const glm::vec3& color);
    Entity CreateVisiblePlane(const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& color);
    void TrackEntity(Entity e);
    void CleanupGameEntities();

    void GenerateDoomLevel();
    void BuildRoom(Room& room, int roomIndex);
    void BuildCorridor(const Room& from, const Room& to, float width = 3.0f, float height = 3.5f);
    void BuildWallWithGap(const glm::vec3& wallStart, const glm::vec3& wallEnd,
                          float wallY, float wallHeight, float thickness,
                          float gapCenter, float gapWidth, const glm::vec3& color, bool alongX);
    void BuildSolidWall(const glm::vec3& start, const glm::vec3& end,
                        float wallY, float wallHeight, float thickness, const glm::vec3& color, bool alongX);

    Entity CreateUnifiedEnemy(EnemyType type, const glm::vec3& position, int roomIndex);
    void SpawnRoomEnemies(int roomIndex);

    Entity CreatePickup(PickupType type, const glm::vec3& position);
    void CheckPickupCollisions();
    void CheckRoomTransitions();
    void HandleShooting();
};

#endif // FPSGAMEMANAGER_H
