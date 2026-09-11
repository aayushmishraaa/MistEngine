#pragma once
#ifndef MIST_TEST_WORLD_H
#define MIST_TEST_WORLD_H

// Shared setup for test cases that drive the global `gCoordinator`.
//
// Why this exists: `Coordinator::Init()` replaces all three managers
// outright, so calling it twice silently discards every component
// registration made before the second call. Several test files need a live
// global coordinator, Catch2 gives no ordering guarantee between them, and
// a per-file `static` latch therefore does exactly the wrong thing — whichever
// file's latch runs second wipes the first file's registrations, and the suite
// passes or fails depending on link order.
//
// One reset used by every such case, registering the superset of components
// the tests need and re-registering HierarchySystem with its signature. Cases
// that want their own isolated world construct a local `Coordinator` instead
// (see tests/test_hierarchy.cpp) and should not use this.

#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/HierarchyComponent.h"
#include "ECS/Components/LightComponent.h"
#include "ECS/Components/NameComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/PrefabComponents.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Coordinator.h"
#include "ECS/Systems/HierarchySystem.h"

#include <vector>

extern Coordinator gCoordinator;

namespace MistTest {

// Rebuilds the global world from scratch: fresh managers, the full component
// set, and HierarchySystem with its signature. Call it at the top of every
// case that touches gCoordinator.
//
// Returns true so callers can wrap it in a REQUIRE and make the dependency
// visible at the top of the case.
inline bool ResetGlobalWorld() {
    gCoordinator.Init();
    gCoordinator.RegisterComponent<NameComponent>();
    gCoordinator.RegisterComponent<CameraComponent>();
    gCoordinator.RegisterComponent<TransformComponent>();
    gCoordinator.RegisterComponent<PhysicsComponent>();
    gCoordinator.RegisterComponent<LightComponent>();
    gCoordinator.RegisterComponent<PrefabInstanceComponent>();
    gCoordinator.RegisterComponent<PrefabMemberComponent>();
    gCoordinator.RegisterComponent<HierarchyComponent>();
    gCoordinator.RegisterSystem<HierarchySystem>();

    Signature sig;
    sig.set(gCoordinator.GetComponentType<TransformComponent>());
    sig.set(gCoordinator.GetComponentType<HierarchyComponent>());
    gCoordinator.SetSystemSignature<HierarchySystem>(sig);
    return true;
}

} // namespace MistTest

#endif // MIST_TEST_WORLD_H
