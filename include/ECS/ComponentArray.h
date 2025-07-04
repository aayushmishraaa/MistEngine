#ifndef COMPONENTARRAY_H
#define COMPONENTARRAY_H

#include <array>
#include <unordered_map>
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
        m_ComponentArray[newIndex] = component;
        ++m_Size;
    }

    void RemoveData(Entity entity) {
        size_t indexOfRemovedEntity = m_EntityToIndexMap[entity];
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
        return m_ComponentArray[m_EntityToIndexMap[entity]];
    }

    void EntityDestroyed(Entity entity) override {
        if (m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end()) {
            RemoveData(entity);
        }
    }

private:
    std::array<T, MAX_ENTITIES> m_ComponentArray;
    std::unordered_map<Entity, size_t> m_EntityToIndexMap;
    std::unordered_map<size_t, Entity> m_IndexToEntityMap;
    size_t m_Size{};
};

#endif // COMPONENTARRAY_H
