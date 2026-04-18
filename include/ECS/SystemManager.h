#ifndef SYSTEMMANAGER_H
#define SYSTEMMANAGER_H

#include "System.h"
#include "TypeID.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

class SystemManager {
  public:
    template <typename T> std::shared_ptr<T> RegisterSystem() {
        const std::uint32_t typeId = Mist::ecs::type_id<T>();
        auto system = std::make_shared<T>();
        m_Systems.insert({typeId, system});
        return system;
    }

    template <typename T> void SetSignature(Signature signature) {
        m_Signatures.insert({Mist::ecs::type_id<T>(), signature});
    }

    void EntityDestroyed(Entity entity) {
        for (auto const& pair : m_Systems) {
            pair.second->m_Entities.erase(entity);
        }
    }

    void EntitySignatureChanged(Entity entity, Signature entitySignature) {
        for (auto const& pair : m_Systems) {
            auto const typeId = pair.first;
            auto const& system = pair.second;
            auto const& systemSignature = m_Signatures[typeId];

            if ((entitySignature & systemSignature) == systemSignature) {
                system->m_Entities.insert(entity);
            } else {
                system->m_Entities.erase(entity);
            }
        }
    }

  private:
    // Integer-keyed maps; same rationale as ComponentManager — avoids the
    // per-call typeid().name() string hash.
    std::unordered_map<std::uint32_t, Signature> m_Signatures{};
    std::unordered_map<std::uint32_t, std::shared_ptr<System>> m_Systems{};
};

#endif // SYSTEMMANAGER_H
