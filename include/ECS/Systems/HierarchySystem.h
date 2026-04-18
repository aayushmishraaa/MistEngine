#ifndef HIERARCHYSYSTEM_H
#define HIERARCHYSYSTEM_H

#include "Core/Signal.h"
#include "ECS/Coordinator.h"
#include "ECS/Entity.h"
#include "ECS/System.h"

// Scene-graph management. Walks the HierarchyComponent tree each frame,
// propagates dirty flags, and computes `TransformComponent::cachedGlobal`
// for every entity. Also handles the ordered OnReady callback so a parent
// sees all its children initialized before its own ready fires.
//
// Design: MistEngine's ECS is flat, so hierarchy is another component.
// System's `m_Entities` only picks up entities that have a
// HierarchyComponent — roots iterate children from there. Entities without
// HierarchyComponent still render (RenderSystem reads cachedGlobal; if
// never touched it equals identity and RenderSystem falls back to the
// local GetModelMatrix — see RenderSystem::Update for that path).
class HierarchySystem : public System {
public:
    using System::Update;

    // Compute cachedGlobal for every dirty entity under every root.
    // Called once per frame before RenderSystem.
    void UpdateTransforms(Coordinator& coord);

    // Fire OnReady (post-order) for entities that haven't fired yet.
    void FireReadyCallbacks(Coordinator& coord);

    // Wire parent/child in both components atomically. Marks the child's
    // transform dirty and returns false if either entity lacks a
    // HierarchyComponent (caller forgot to AddComponent).
    static bool Attach(Coordinator& coord, Entity parent, Entity child);

    // Detach breaks the link both ways and re-marks the child as dirty so
    // the next UpdateTransforms recomputes it as a root.
    static bool Detach(Coordinator& coord, Entity child);

    // The OnReady signal fired by FireReadyCallbacks. Consumers (editor,
    // plugins, modules) connect here to run once-per-entity init logic in
    // the right order. See Core/Signal.h for connection semantics.
    static Mist::Signal<Entity>& OnReady();
};

#endif // HIERARCHYSYSTEM_H
