#ifndef PLAYERSYSTEM_H
#define PLAYERSYSTEM_H

#include "../System.h"
#include "../../InputManager.h"
#include <glm/glm.hpp>
#include <memory>

class Camera;

class PlayerSystem : public System {
public:
    PlayerSystem();
    
    void Init(Camera* camera, InputManager* inputManager);
    void Update(float deltaTime) override;
    
    // Player creation
    Entity CreatePlayer(glm::vec3 position = glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Player queries
    Entity GetPlayer() const { return m_playerId; }
    bool HasPlayer() const { return m_playerId != static_cast<Entity>(-1); }
    
private:
    Camera* m_camera;
    InputManager* m_inputManager;
    Entity m_playerId;
    
    // Player input processing
    void ProcessMovementInput(Entity player, float deltaTime);
    void ProcessMouseInput(Entity player, float deltaTime);
    void ProcessWeaponInput(Entity player, float deltaTime);
    
    // Player actions
    void HandleShooting(Entity player);
    void HandleReloading(Entity player);
    void HandleWeaponSwitching(Entity player);
    
    // Health and damage
    void UpdatePlayerHealth(Entity player, float deltaTime);
    void HandlePlayerDeath(Entity player);
    
    // Camera synchronization
    void UpdateCameraFromPlayer(Entity player);
    void UpdatePlayerFromCamera(Entity player);
};

#endif // PLAYERSYSTEM_H