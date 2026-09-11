#pragma once
#ifndef MIST_ENTITY_NAME_H
#define MIST_ENTITY_NAME_H

// Name lookup helpers over NameComponent.
//
// Kept out of NameComponent.h so the component header stays dependency-free
// (Coordinator.h already pulls in several component headers; putting these
// there would close the cycle).
//
// FindEntityByName is a linear scan. That is deliberate for now: a name index
// has to be invalidated on create, destroy and rename, and getting that wrong
// hands out stale entity ids — which is exactly the class of bug the liveness
// guard at commit b542818 had to fix. Correctness first; if this shows up in a
// profile, the index goes next to the Coordinator, not here.

#include "ECS/Components/NameComponent.h"
#include "ECS/Coordinator.h"

#include <string>
#include <string_view>

namespace Mist {

// Name of `entity`, or a "Entity <id>" placeholder when it has no
// NameComponent. The placeholder matches what the Hierarchy panel used to
// synthesise, so untitled entities read the same as before.
inline std::string EntityName(Coordinator& coord, Entity entity) {
    if (coord.HasComponent<NameComponent>(entity)) {
        const auto& n = coord.GetComponent<NameComponent>(entity).name;
        if (!n.empty()) return n;
    }
    return "Entity " + std::to_string(entity);
}

// True when the entity carries an explicit name, as opposed to falling back
// to the placeholder. Callers that must not persist a placeholder use this.
inline bool HasEntityName(Coordinator& coord, Entity entity) {
    return coord.HasComponent<NameComponent>(entity)
        && !coord.GetComponent<NameComponent>(entity).name.empty();
}

inline void SetEntityName(Coordinator& coord, Entity entity, std::string name) {
    if (coord.HasComponent<NameComponent>(entity)) {
        coord.GetComponent<NameComponent>(entity).name = std::move(name);
    } else {
        coord.AddComponent(entity, NameComponent{std::move(name)});
    }
}

inline void ClearEntityName(Coordinator& coord, Entity entity) {
    if (coord.HasComponent<NameComponent>(entity)) {
        coord.RemoveComponent<NameComponent>(entity);
    }
}

// First living entity with this exact name, or kInvalidEntity.
// Names are not unique engine-wide, so "first" is iteration order — callers
// that need determinism should be addressing by hierarchy path instead.
inline constexpr Entity kInvalidEntity = static_cast<Entity>(-1);

inline Entity FindEntityByName(Coordinator& coord, std::string_view name) {
    for (Entity e : coord.GetLivingEntities()) {
        if (!coord.HasComponent<NameComponent>(e)) continue;
        if (coord.GetComponent<NameComponent>(e).name == name) return e;
    }
    return kInvalidEntity;
}

} // namespace Mist

#endif // MIST_ENTITY_NAME_H
