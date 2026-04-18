#ifndef COMPONENTMANAGER_H
#define COMPONENTMANAGER_H

#include "Component.h"
#include "ComponentArray.h"
#include "Entity.h"
#include "TypeID.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

class ComponentManager {
  public:
    template <typename T> void RegisterComponent() {
        const std::uint32_t typeId = Mist::ecs::type_id<T>();
        m_ComponentTypes.insert({typeId, m_NextComponentType});
        m_ComponentArrays.insert({typeId, std::make_shared<ComponentArray<T>>()});
        ++m_NextComponentType;
    }

    template <typename T> ComponentType GetComponentType() {
        return m_ComponentTypes[Mist::ecs::type_id<T>()];
    }

    template <typename T> void AddComponent(Entity entity, T component) {
        GetComponentArray<T>()->InsertData(entity, component);
    }

    template <typename T> void RemoveComponent(Entity entity) {
        GetComponentArray<T>()->RemoveData(entity);
    }

    template <typename T> T& GetComponent(Entity entity) {
        return GetComponentArray<T>()->GetData(entity);
    }

    template <typename T> bool HasComponent(Entity entity) {
        return GetComponentArray<T>()->HasData(entity);
    }

    void EntityDestroyed(Entity entity) {
        for (auto const& pair : m_ComponentArrays) {
            pair.second->EntityDestroyed(entity);
        }
    }

  private:
    // Keyed by the integer type-id from Mist::ecs::type_id<T>() — unordered_map
    // on std::uint32_t is a direct hash (identity) rather than a string compare.
    std::unordered_map<std::uint32_t, ComponentType> m_ComponentTypes{};
    std::unordered_map<std::uint32_t, std::shared_ptr<IComponentArray>> m_ComponentArrays{};
    ComponentType m_NextComponentType{};

    template <typename T> std::shared_ptr<ComponentArray<T>> GetComponentArray() {
        return std::static_pointer_cast<ComponentArray<T>>(
            m_ComponentArrays[Mist::ecs::type_id<T>()]);
    }
};

#endif // COMPONENTMANAGER_H
