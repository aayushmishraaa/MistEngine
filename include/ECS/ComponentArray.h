#ifndef COMPONENTARRAY_H
#define COMPONENTARRAY_H

#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <functional>
#include "Entity.h"

class IComponentArray {
public:
    virtual ~IComponentArray() = default;
    virtual void EntityDestroyed(Entity entity) = 0;
};

template<typename T>
class ComponentArray : public IComponentArray {
public:
    void InsertData(Entity entity, T component) {
        size_t newIndex = m_Size;
        m_EntityToIndexMap[entity] = newIndex;
        m_IndexToEntityMap[newIndex] = entity;
        if (newIndex >= m_ComponentArray.size()) {
            m_ComponentArray.resize(newIndex + 1);
        }
        m_ComponentArray[newIndex] = component;
        ++m_Size;
    }

    void RemoveData(Entity entity) {
        auto it = m_EntityToIndexMap.find(entity);
        if (it == m_EntityToIndexMap.end()) {
            return;
        }

        size_t indexOfRemovedEntity = it->second;
        size_t indexOfLastElement = m_Size - 1;
        m_ComponentArray[indexOfRemovedEntity] = m_ComponentArray[indexOfLastElement];

        Entity entityOfLastElement = m_IndexToEntityMap[indexOfLastElement];
        m_EntityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
        m_IndexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

        m_EntityToIndexMap.erase(entity);
        m_IndexToEntityMap.erase(indexOfLastElement);

        --m_Size;
    }

    T& GetData(Entity entity) {
        auto it = m_EntityToIndexMap.find(entity);
        if (it == m_EntityToIndexMap.end()) {
            throw std::runtime_error("Entity does not have component");
        }
        return m_ComponentArray[it->second];
    }

    bool HasData(Entity entity) const {
        return m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end();
    }

    void EntityDestroyed(Entity entity) override {
        if (m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end()) {
            RemoveData(entity);
        }
    }

    // Cache-friendly iteration over all active components
    template<typename Fn>
    void ForEach(Fn&& fn) {
        for (size_t i = 0; i < m_Size; i++) {
            fn(m_IndexToEntityMap[i], m_ComponentArray[i]);
        }
    }

    size_t Size() const { return m_Size; }

private:
    std::vector<T> m_ComponentArray;
    std::unordered_map<Entity, size_t> m_EntityToIndexMap;
    std::unordered_map<size_t, Entity> m_IndexToEntityMap;
    size_t m_Size{};
};

#endif // COMPONENTARRAY_H
