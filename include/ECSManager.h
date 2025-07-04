
#ifndef ECS_MANAGER_H
#define ECS_MANAGER_H

#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <algorithm>

// Basic Entity type
using Entity = unsigned int;
const Entity MAX_ENTITIES = 10000; // Define a maximum number of entities

// Basic Component base class
struct Component {
    virtual ~Component() = default;
};

// ECS Manager class
class ECSManager {
public:
    ECSManager();

    Entity createEntity();
    void destroyEntity(Entity entity);

    template<typename T, typename... Args>
    void addComponent(Entity entity, Args&&... args);

    template<typename T>
    void removeComponent(Entity entity);

    template<typename T>
    T* getComponent(Entity entity);

    template<typename T>
    bool hasComponent(Entity entity);

    // Method to get entities with a specific set of components (for systems)
    template<typename... Components>
    std::vector<Entity> getEntitiesWith();

private:
    std::vector<bool> entityAlive;
    unsigned int livingEntityCount;

    // Stores vectors of components, indexed by component type
    std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>> components;

    // Stores mapping from entity ID to component index within each component vector
    std::unordered_map<std::type_index, std::vector<int>> entityComponentIndex;

    // Stores mapping from entity ID to a map of component type to index
    std::vector<std::unordered_map<std::type_index, int>> componentLookup;

    // Helper to get component vector for a given type
    template<typename T>
    std::vector<std::unique_ptr<Component>>& getComponentVector();

    // Helper to get entity component index vector for a given type
    template<typename T>
    std::vector<int>& getEntityComponentIndexVector();
};

#include "ECSManager.inl" // Include inline implementations for templates

#endif // ECS_MANAGER_H
