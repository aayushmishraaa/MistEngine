#ifndef SCRIPTSYSTEM_H
#define SCRIPTSYSTEM_H

#include "ECS/Coordinator.h"
#include "ECS/System.h"

// Script lifecycle driver. Calls `_ready()` on each attached script
// exactly once — piggy-backs on HierarchySystem::OnReady so callbacks
// fire in deterministic post-order (children before parents) — and
// `_process(dt)` once per frame.
class ScriptSystem : public System {
public:
    using System::Update;

    void Update(Coordinator& coord, float dt);

    // Hook ScriptSystem into HierarchySystem::OnReady. Call once during
    // engine bring-up after both systems exist.
    void WireReadyCallback(Coordinator& coord);

    // Explicit reset for tests so the signal connection doesn't linger
    // across Catch2 test-case boundaries.
    void UnwireReadyCallback();

private:
    size_t m_ReadyConnection = 0;
};

#endif // SCRIPTSYSTEM_H
