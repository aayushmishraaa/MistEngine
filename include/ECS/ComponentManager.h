#ifndef COMPONENTMANAGER_H
#define COMPONENTMANAGER_H

#include "Component.h"
#include "ComponentArray.h"
#include "Entity.h"
#include "TypeID.h"

#include <cassert>
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
        // Must tolerate an UNREGISTERED T and answer "no".
        //
        // GetComponentArray below indexes m_ComponentArrays with operator[],
        // which default-inserts a null shared_ptr for a type that was never
        // registered — so the old `GetComponentArray<T>()->HasData(entity)`
        // dereferenced null. Any caller that probes for a component a given
        // Coordinator doesn't know about (tests that register a subset,
        // Coordinator::UnlinkFromHierarchy asking about HierarchyComponent)
        // would crash rather than get `false`.
        auto arr = TryGetComponentArray<T>();
        return arr && arr->HasData(entity);
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

    // Non-inserting lookup. Returns nullptr when T was never registered.
    template <typename T> std::shared_ptr<ComponentArray<T>> TryGetComponentArray() {
        auto it = m_ComponentArrays.find(Mist::ecs::type_id<T>());
        if (it == m_ComponentArrays.end()) return nullptr;
        return std::static_pointer_cast<ComponentArray<T>>(it->second);
    }

    // Registered-type accessor for the add/get/remove paths, which are a
    // programming error on an unregistered type rather than a query.
    template <typename T> std::shared_ptr<ComponentArray<T>> GetComponentArray() {
        auto arr = TryGetComponentArray<T>();
        assert(arr && "Component type used before RegisterComponent<T>()");
        return arr;
    }
};

#endif // COMPONENTMANAGER_H
