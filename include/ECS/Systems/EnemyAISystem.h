#ifndef ENEMYAISYSTEM_H
#define ENEMYAISYSTEM_H

#include "../System.h"
#include "../Components/EnemyComponent.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

class PlayerSystem;
class WeaponSystem;
struct Vertex;
class Mesh;

class EnemyAISystem : public System {
public:
    EnemyAISystem();
    
    void Init(PlayerSystem* playerSystem, WeaponSystem* weaponSystem);
    void Update(float deltaTime) override;
    
    // Enemy creation
    Entity CreateEnemy(EnemyType type, glm::vec3 position, const std::vector<glm::vec3>& patrolPoints = {});
    Entity CreateGrunt(glm::vec3 position);
    Entity CreateSoldier(glm::vec3 position);
    
    // Enemy management
    void SetPatrolPoints(Entity enemy, const std::vector<glm::vec3>& points);
    void SetTarget(Entity enemy, Entity target);
    
    // AI State queries
    int GetAliveEnemyCount() const;
    std::vector<Entity> GetEnemiesInRoom(int roomIndex) const;
    
    // Combat interaction methods
    void DamageEnemy(Entity enemy, float damage, Entity attacker);
    void KillEnemy(Entity enemy, Entity killer);
    
private:
    PlayerSystem* m_playerSystem;
    WeaponSystem* m_weaponSystem;
    
    // AI behavior updates
    void UpdateEnemyState(Entity enemy, float deltaTime);
    void UpdatePatrolBehavior(Entity enemy, float deltaTime);
    void UpdateChaseBehavior(Entity enemy, float deltaTime);
    void UpdateAttackBehavior(Entity enemy, float deltaTime);
    void UpdateSearchBehavior(Entity enemy, float deltaTime);
    
    // AI decisions
    bool CanSeePlayer(Entity enemy, Entity player);
    bool IsPlayerInAttackRange(Entity enemy, Entity player);
    float GetDistanceToPlayer(Entity enemy, Entity player);
    glm::vec3 GetNextPatrolPosition(Entity enemy);
    
    // Movement and navigation
    void MoveTowards(Entity enemy, glm::vec3 targetPosition, float deltaTime);
    glm::vec3 GetDirectionToTarget(glm::vec3 from, glm::vec3 to);
    
    // Combat actions
    void AttackPlayer(Entity enemy, Entity player);
    void EquipDefaultWeapon(Entity enemy);
    
    // Enemy configuration
    void ConfigureEnemyStats(Entity enemy, EnemyType type);
    void CreateEnemyPhysics(Entity enemy, glm::vec3 position);
    void CreateEnemyVisuals(Entity enemy, EnemyType type);
};

#endif // ENEMYAISYSTEM_H