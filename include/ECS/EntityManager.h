#ifndef ENTITYMANAGER_H
#define ENTITYMANAGER_H

#include <queue>
#include <array>
#include <bitset>
#include "Entity.h"
#include "Component.h"

using Signature = std::bitset<MAX_COMPONENTS>;

class EntityManager {
public:
    EntityManager() {
        for (Entity entity = 0; entity < MAX_ENTITIES; ++entity) {
            m_AvailableEntities.push(entity);
        }
    }

    Entity CreateEntity() {
        Entity id = m_AvailableEntities.front();
        m_AvailableEntities.pop();
        ++m_LivingEntityCount;
        return id;
    }

    void DestroyEntity(Entity entity) {
        m_Signatures[entity].reset();
        m_AvailableEntities.push(entity);
        --m_LivingEntityCount;
    }

    void SetSignature(Entity entity, Signature signature) {
        m_Signatures[entity] = signature;
    }

    Signature GetSignature(Entity entity) {
        return m_Signatures[entity];
    }

private:
    std::queue<Entity> m_AvailableEntities{};
    std::array<Signature, MAX_ENTITIES> m_Signatures{};
    uint32_t m_LivingEntityCount{};
};

#endif // ENTITYMANAGER_H
