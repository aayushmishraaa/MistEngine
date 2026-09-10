#ifndef ENTITYMANAGER_H
#define ENTITYMANAGER_H

#include <queue>
#include <unordered_set>
#include <vector>
#include <bitset>
#include <cassert>
#include "Entity.h"
#include "Component.h"

using Signature = std::bitset<MAX_COMPONENTS>;

class EntityManager {
public:
    EntityManager() {
        m_Signatures.resize(MAX_ENTITIES);
        for (Entity entity = 0; entity < MAX_ENTITIES; ++entity) {
            m_AvailableEntities.push(entity);
        }
    }

    Entity CreateEntity() {
        assert(m_LivingEntityCount < MAX_ENTITIES && "Too many entities");
        Entity id = m_AvailableEntities.front();
        m_AvailableEntities.pop();
        ++m_LivingEntityCount;
        m_LivingEntities.insert(id);
        return id;
    }

    void DestroyEntity(Entity entity) {
        assert(entity < MAX_ENTITIES && "Entity out of range");

        // Ignore a destroy of something that isn't alive. Without this guard a
        // double-destroy pushed the same id onto the free queue twice, so two
        // subsequent CreateEntity calls handed out the SAME id — two logical
        // entities sharing one set of components. It also drove
        // m_LivingEntityCount negative (it's unsigned, so it wrapped).
        //
        // Reachable in practice: the Lua `destroy_entity` binding is documented
        // as tolerant of stale ids, and the editor's delete-undo-redo path can
        // re-issue a destroy for an id it already released.
        if (m_LivingEntities.erase(entity) == 0) {
            return;
        }

        m_Signatures[entity].reset();
        m_AvailableEntities.push(entity);
        --m_LivingEntityCount;
    }

    void SetSignature(Entity entity, Signature signature) {
        assert(entity < MAX_ENTITIES && "Entity out of range");
        m_Signatures[entity] = signature;
    }

    Signature GetSignature(Entity entity) {
        assert(entity < MAX_ENTITIES && "Entity out of range");
        return m_Signatures[entity];
    }

    uint32_t GetLivingEntityCount() const { return m_LivingEntityCount; }

    // Authoritative set of currently-allocated entity IDs. UI and
    // serialization iterate this rather than guessing from a max-seen-ID
    // counter — the latter silently skips entities created by non-UI
    // paths (Lua, plugins) and was the root cause of the "Entity 0"
    // hierarchy bug.
    const std::unordered_set<Entity>& GetLivingEntities() const { return m_LivingEntities; }

private:
    std::queue<Entity> m_AvailableEntities{};
    std::unordered_set<Entity> m_LivingEntities{};
    std::vector<Signature> m_Signatures{};
    uint32_t m_LivingEntityCount{};
};

#endif // ENTITYMANAGER_H
