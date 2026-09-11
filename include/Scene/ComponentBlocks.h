#pragma once
#ifndef MIST_COMPONENT_BLOCKS_H
#define MIST_COMPONENT_BLOCKS_H

// Per-entity component JSON, shared by every serializer that stores entities.
//
// Problem this solves: SceneSerializer's Save/Load carried the only copy of
// "what a serialized entity looks like" — the transform / render / physics /
// light / camera / animation blocks, plus the mesh-ref encoding that lets a
// RenderComponent recover what its non-owning Renderable* pointed at. The
// prefab serializer stores the same entities in a different *addressing*
// scheme (name paths instead of remapped ids), but the component payload is
// identical.
//
// Copying it would have guaranteed drift: add a component, remember to update
// two writers and two readers, and a prefab silently loses the field a scene
// keeps. That is the same trap Core/ReflectionJson.h was hoisted to avoid, one
// level up.
//
// What is deliberately NOT here: names and hierarchy. Those are *addressing*,
// and addressing is exactly what differs between the two formats — scenes use
// integer ids remapped through a table, prefabs use sibling-unique name paths
// so a prefab is position-independent and ids never leak between assets.

#include "Core/ReflectionJson.h"
#include "ECS/Components/AnimationComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/LightComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Coordinator.h"

#include <nlohmann/json.hpp>

namespace Mist::Scene {

// Mesh reference encoding. `{"builtin":"cube"}` or `{"ext":"path/to.fbx"}`,
// derived from RenderComponent::meshPath.
nlohmann::json MeshRefFor(const RenderComponent& r);

// Resolves a mesh ref back to a Renderable*, or nullptr. On-disk models are a
// known gap: they are re-imported through SceneImporter, which spawns its own
// entity tree, so a serialized reference cannot restore one from here.
Renderable* ResolveMeshRef(const nlohmann::json& meshJson);

// Writes transform / camera / render / physics / light / animation blocks for
// `entity` into `out`. Only components the entity actually has are emitted.
void WriteComponentBlocks(nlohmann::json& out, Entity entity, Coordinator& coord);

// The inverse. Adds each component present in `in` to `entity`. Components
// absent from the JSON are simply not added, so an entity round-trips with the
// same component set it started with.
void ReadComponentBlocks(const nlohmann::json& in, Entity entity, Coordinator& coord);

} // namespace Mist::Scene

#endif // MIST_COMPONENT_BLOCKS_H
