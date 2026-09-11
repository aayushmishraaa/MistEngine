---
type: Roadmap Phase
title: Phase 1 — Entity identity and Environment resource
description: Serialize entity names and add name lookup; collapse 41 scattered render tunables into one reflected, serializable Environment. Both are prerequisites disguised as polish.
tags: [roadmap, phase, identity, environment, serialization]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: /subsystems/entity-identity.md
    title: Entity identity
  - resource: /subsystems/environment-and-camera.md
    title: Environment and camera
---

# Prerequisite

None. This is the entry point.

# Why first

Neither item is glamorous and both are cheap, but [phase 3](/roadmap/phase-3-prefabs.md) cannot start
without them. A prefab whose contents cannot be named cannot express an override, and a scene that
cannot store its own lighting is not a portable description of anything.

# Part A — entity identity — **done**

Move names out of the editor and into the engine.

Landed as described, with two deviations worth recording:

- `SceneImporter::ImportToScene`'s name out-param was **removed** rather than
  retargeted — it already had a `Coordinator&`, so a `bool nameEntities` flag
  replaced the `std::unordered_map<Entity,std::string>*`.
- `SceneSerializer` gained `SaveToString` / `LoadFromString`. The round-trip
  tests this phase calls for need a serializer that touches neither the
  filesystem nor the scene sandbox; play mode's snapshot in
  [phase 4](/roadmap/phase-4-wire-the-built-but-dead.md) needs the same pair,
  so it was pulled forward rather than written twice.

- Add a `NameComponent` (or a `name` field on an existing component) holding a `std::string`, and
  register it in `main()` alongside the others.
- Migrate `UIManager::m_EntityNames` to read and write that component instead of its private map.
  The Inspector's rename field and the Hierarchy labels both already exist; they just need to target
  the component.
- Serialize it. `SceneSerializer`'s Save loop writes `id`/`transform`/`render`/`physics`/`light`/
  `hierarchy`/`animation`; add `name`. The format example at `include/Scene/SceneSerializer.h:22`
  already documents the field, so this makes the header honest.
- Add a name → entity lookup. A `std::unordered_map<std::string, Entity>` maintained alongside, or a
  linear scan to start — correctness before speed.
- Expose it to Lua as `find_entity(name)`. This is the seed of `get_node()`; full `NodePath`
  resolution (`"../Door"`) can wait.

Reuse: `MIST_REFLECT` on the new component gets the Inspector widget and serializer support for free,
as `RenderComponent::meshPath` did at commit `b542818`.

# Part B — Environment resource — *pending*

Collapse the 41 tunables spread across 9 objects into one place.

- Define `struct Environment` with `MIST_REFLECT`, holding the fields that describe *how a scene
  looks*: tonemap operator, exposure, the bloom / SSAO / SSR / SSGI / TAA / DOF / motion-blur enables
  and their parameters, shadow softness and quality, sky mode and atmospheric parameters, ambient
  fallback.
- Keep the sub-renderers as the *implementation*. `PostProcessStack::Execute` reads from the
  `Environment` each frame rather than from its own public fields. This is a plumbing change, not a
  rewrite of any effect.
- Store one `Environment` per scene, serialized in the `.mist` file.
- Replace the View-menu panels with a single reflected Inspector block —
  `DrawReflectedProperties(&env, props)`. `UIManager::DrawPostProcessControls`, `DrawShadowControls`
  and `DrawSkyboxControls` collapse into it, which is a net deletion of hand-written widget code.

Deliberately **not** in scope: Godot's `CameraAttributes` split, the priority chain
(Camera > WorldEnvironment > preview), and fog. One serializable Environment first.

# Files

- New: `include/ECS/Components/NameComponent.h`, `include/Environment.h`
- `src/Scene/SceneSerializer.cpp` — name field; environment block
- `src/UIManager.cpp` — rename target, panel collapse
- `src/PostProcessStack.cpp`, `include/PostProcessStack.h` — read from Environment
- `src/Script/LuaScriptLanguage.cpp` — `find_entity`
- `src/MistEngine.cpp` — register the new component

# How to tell it worked

- Name an entity, save, reload: the name survives. Currently it becomes "Entity 7".
- Change the tonemap to Reinhard and the bloom threshold, save, restart, reload: both persist.
  Currently every setting resets to its compile-time default.
- `find_entity("Ground")` from the console returns a valid id.
- Tests: a serializer round-trip asserting `name` and an `Environment` field survive — extend the
  round-trip cases added at commit `b542818` in `tests/test_scene_serializer.cpp`.
