#ifndef HIERARCHYCOMPONENT_H
#define HIERARCHYCOMPONENT_H

#include "Core/Reflection.h"
#include "ECS/Entity.h"

#include <cstdint>
#include <vector>

// Parent/child relationship component. An entity without this component is
// treated as a root (no parent, no tracked children). The parent/children
// fields are kept in sync by HierarchySystem::Attach/Detach — callers
// should not touch them directly.
//
// Using a sentinel uint32_t(-1) for "no parent" instead of std::optional so
// the component stays trivially copyable; ECS component storage will evolve
// to support non-trivial types later but the hierarchy doesn't need it.
struct HierarchyComponent {
    static constexpr Entity kNoParent = static_cast<Entity>(-1);

    Entity              parent     = kNoParent;
    std::vector<Entity> children;
    bool                readyFired = false;  // see HierarchySystem::FireReadyCallbacks
};

// Reflection intentionally omits `parent` and `children` from the inspector
// — hierarchy is edited graphically via the Scene panel, not by typing
// entity IDs. `readyFired` is implementation-detail and stays hidden too.
MIST_REFLECT(HierarchyComponent)
MIST_REFLECT_END(HierarchyComponent)

#endif // HIERARCHYCOMPONENT_H
