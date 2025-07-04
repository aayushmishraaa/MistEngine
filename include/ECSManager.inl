#include <stdexcept> // For std::runtime_error

inline ECSManager::ECSManager() : entityAlive(MAX_ENTITIES, false), livingEntityCount(0), componentLookup(MAX_ENTITIES) {
    // Initialize component vectors and index vectors for known component types here if needed
    // Or, they will be created on demand when the first component of a type is added
}

inline Entity ECSManager::createEntity() {
    if (livingEntityCount >= MAX_ENTITIES) {
        // Handle error: Maximum number of entities reached
        throw std::runtime_error("Maximum number of entities reached");
    }

    Entity entity = 0;
    // Find a free entity ID
    while (entity < MAX_ENTITIES && entityAlive[entity]) {
        entity++;
    }

    if (entity == MAX_ENTITIES) {
         // This should not happen if livingEntityCount is checked, but as a safeguard
         throw std::runtime_error("Could not find a free entity ID");
    }


    entityAlive[entity] = true;
    livingEntityCount++;

    // Clear component lookup for the new entity
    componentLookup[entity].clear();


    return entity;
}

inline void ECSManager::destroyEntity(Entity entity) {
    if (entity < MAX_ENTITIES && entityAlive[entity]) {
        // Remove all components associated with this entity
        for (auto const& [type, index] : componentLookup[entity]) {
             // This is a simplified approach; a more robust implementation would
             // involve swapping with the last element and popping for efficiency
             // However, for this basic structure, we'll just mark as invalid for now
             // and rely on systems to handle validity checks or a cleanup phase.
             // A proper implementation would need reverse mapping for efficient removal
             // from the component vectors.
        }
        componentLookup[entity].clear();


        entityAlive[entity] = false;
        livingEntityCount--;
    }
}


template<typename T, typename... Args>
inline void ECSManager::addComponent(Entity entity, Args&&... args) {
    if (entity >= MAX_ENTITIES || !entityAlive[entity]) {
        // Handle error: Entity is invalid
        return;
    }

    std::type_index typeIndex = std::type_index(typeid(T));

    // Check if entity already has this component
    if (hasComponent<T>(entity)) {
        // Handle error: Entity already has this component
        return;
    }

    // Get or create the vector for this component type
    if (components.find(typeIndex) == components.end()) {
        components[typeIndex] = std::vector<std::unique_ptr<Component>>(MAX_ENTITIES);
        entityComponentIndex[typeIndex] = std::vector<int>(MAX_ENTITIES, -1);
    }

    // Find the next available slot in the component vector for this type
    // This is a simple linear scan, could be optimized
    int componentIndex = -1;
    for(int i = 0; i < MAX_ENTITIES; ++i) {
        if (components[typeIndex][i] == nullptr) {
            componentIndex = i;
            break;
        }
    }

    if (componentIndex == -1) {
        // Handle error: No more space for this component type
         throw std::runtime_error("No more space for this component type");
    }

    // Create the component and add it to the vector
    components[typeIndex][componentIndex] = std::make_unique<T>(std::forward<Args>(args)...);

    // Update the entity-to-component index mapping
    entityComponentIndex[typeIndex][entity] = componentIndex;

    // Update the component lookup for the entity
    componentLookup[entity][typeIndex] = componentIndex;

}

template<typename T>
inline void ECSManager::removeComponent(Entity entity) {
     if (entity >= MAX_ENTITIES || !entityAlive[entity]) {
        // Handle error: Entity is invalid
        return;
    }

    std::type_index typeIndex = std::type_index(typeid(T));

    // Check if entity has this component
    if (!hasComponent<T>(entity)) {
        // Handle error: Entity does not have this component
        return;
    }

    int componentIndex = componentLookup[entity][typeIndex];

    // Delete the component
    components[typeIndex][componentIndex].reset(); // Use reset to free the unique_ptr

    // Update the entity-to-component index mapping
    entityComponentIndex[typeIndex][entity] = -1;

    // Remove from the component lookup for the entity
    componentLookup[entity].erase(typeIndex);
}


template<typename T>
inline T* ECSManager::getComponent(Entity entity) {
    if (entity >= MAX_ENTITIES || !entityAlive[entity]) {
        // Handle error: Entity is invalid
        return nullptr;
    }

    std::type_index typeIndex = std::type_index(typeid(T));

    // Check if entity has this component and get its index
    auto it = componentLookup[entity].find(typeIndex);
    if (it == componentLookup[entity].end()) {
        return nullptr; // Entity does not have this component
    }

    int componentIndex = it->second;

    // Retrieve the component from the component vector
    if (componentIndex != -1 && componentIndex < MAX_ENTITIES && components[typeIndex][componentIndex] != nullptr) {
        return static_cast<T*>(components[typeIndex][componentIndex].get());
    }

    return nullptr; // Should not happen if componentLookup is consistent, but as a safeguard
}

template<typename T>
inline bool ECSManager::hasComponent(Entity entity) {
    if (entity >= MAX_ENTITIES || !entityAlive[entity]) {
        // Handle error or return false for invalid entity
        return false;
    }

    std::type_index typeIndex = std::type_index(typeid(T));

    // Check if the component type exists in the lookup for this entity
    return componentLookup[entity].count(typeIndex) > 0;
}

template<typename... Components>
inline std::vector<Entity> ECSManager::getEntitiesWith() {
    std::vector<Entity> entities;
    // Iterate through all living entities
    for (Entity entity = 0; entity < MAX_ENTITIES; ++entity) {
        if (entityAlive[entity]) {
            // Check if the entity has all the required components
            bool hasAllComponents = true;
            ([&](auto type){
                if (!hasComponent<decltype(type)>(entity)) {
                    hasAllComponents = false;
                }
            }(Components{}), ...); // Fold expression to check all component types

            if (hasAllComponents) {
                entities.push_back(entity);
            }
        }
    }
    return entities;
}

template<typename T>
inline std::vector<std::unique_ptr<Component>>& ECSManager::getComponentVector() {
    std::type_index typeIndex = std::type_index(typeid(T));
    return components[typeIndex]; // This will create the vector if it doesn't exist
}

template<typename T>
inline std::vector<int>& ECSManager::getEntityComponentIndexVector() {
     std::type_index typeIndex = std::type_index(typeid(T));
     return entityComponentIndex[typeIndex]; // This will create the vector if it doesn't exist
}