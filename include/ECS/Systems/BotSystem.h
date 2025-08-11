#ifndef BOTSYSTEM_H
#define BOTSYSTEM_H

#include "../System.h"
#include <glm/glm.hpp>
#include <memory>

// Forward declare instead of include to avoid circular dependencies
class ProjectileSystem;

class BotSystem : public System {
public:
    void Update(float deltaTime);
    void SetGameMode(bool isGameMode) { m_IsGameMode = isGameMode; }
    void SetProjectileSystem(std::shared_ptr<ProjectileSystem> projectileSystem) { m_ProjectileSystem = projectileSystem; }

private:
    bool m_IsGameMode = false;
    std::shared_ptr<ProjectileSystem> m_ProjectileSystem;
    
    void UpdateBotAI(float deltaTime);
    void HandleBotState(Entity entity, float deltaTime);
    void HandleIdleState(Entity entity, float deltaTime);
    void HandlePatrolState(Entity entity, float deltaTime);
    void HandleChaseState(Entity entity, float deltaTime);
    void HandleAttackState(Entity entity, float deltaTime);
    void HandleDeadState(Entity entity, float deltaTime);
    
    Entity FindNearestPlayer(const glm::vec3& botPosition, float maxDistance);
    float GetDistanceToPlayer(Entity bot, Entity player);
    glm::vec3 GetRandomPatrolPoint(const glm::vec3& center, float radius);
    void MoveTowards(Entity entity, const glm::vec3& target, float speed, float deltaTime);
    void MarkBotAsDead(Entity entity);
};

#endif // BOTSYSTEM_H