---
type: Roadmap Phase
title: Phase 3 — .mistprefab instancing
description: The first reuse unit in the engine. A separate asset type holding an entity subtree, instanced by reference with per-instance overrides.
tags: [roadmap, phase, prefab, scene, serialization]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: /decisions/0001-prefab-asset-model.md
    title: Decision 0001 — prefab asset model
  - resource: /subsystems/scene-and-composition.md
    title: Scene and composition
---

# Prerequisite

[Phase 1](/roadmap/phase-1-identity-and-environment.md) and
[phase 2](/roadmap/phase-2-camera-and-culling.md). Names are needed to address override targets;
the camera needs to be a component for a prefab to meaningfully contain one.

# Status: **done**

# Scope

Per [decision 0001](/decisions/0001-prefab-asset-model.md): `.mistprefab` is a **separate asset type**,
not an instantiable scene. Godot's scene/prefab duality was considered and rejected.

A prefab is an entity subtree — a root plus descendants, with their components — serialized with
**prefab-local names** rather than entity ids, so it is position-independent and ids never leak
between assets.

# Design

**Format.** Reuse the scene serializer's machinery: JSON, reflection-driven component blocks, the same
`writeReflectedFields` / `readReflectedFields` helpers. The difference is that children are addressed
by prefab-local name paths (`"Turret/Barrel"`) rather than by remapped ids. That keeps one serializer
implementation rather than two.

**Instancing.** A scene entity carries a `PrefabInstanceComponent` with the source path and an
override table. On load: read the prefab, spawn the subtree, apply overrides. The entities are normal
entities afterwards — no special-casing in the render or physics paths.

**Overrides.** Keyed on `(local name path, component type, field name)`, which the existing reflection
`PropertyInfo` already gives by name. Scope the first cut deliberately narrowly: scalar and vector
fields only. Structural overrides — adding or removing a child on an instance — are where prefab
systems get genuinely hard, and are explicitly out of scope for the first cut.

**Propagation.** Because the scene stores a *reference* plus overrides rather than a flattened copy,
editing the prefab and reloading the scene picks up the change for free. That is the whole payoff of
storing it by reference.

# Editor surface

- Hierarchy context menu: "Save as prefab" on a selected subtree.
- Asset browser: drag a `.mistprefab` onto the viewport to instance it. The drop plumbing already
  exists — `UIManager::HandleAssetDrop` routes by extension and has `.mistpkg`, `.mist`, `.obj`,
  `.fbx`, `.gltf`, `.glb` cases to extend.
- Inspector: mark overridden fields and offer a revert, as Godot does. This is what makes overrides
  comprehensible rather than mysterious.
- Undo: instancing and reverting both push commands. `Mist::Editor::UndoStack` merge semantics already
  handle the drag case.

# Files

- New: `include/Assets/PrefabSerializer.h`, `src/Assets/PrefabSerializer.cpp`
- New: `include/ECS/Components/PrefabInstanceComponent.h`
- `src/Scene/SceneSerializer.cpp` — serialize the instance reference, not the expanded subtree
- `src/UIManager.cpp` — save-as-prefab, drop handling, override markers
- `src/Assets/PackageIO.cpp` — include referenced prefabs in the dependency walk

# Risks

- **Override semantics are where these systems rot.** Keeping the first cut to scalar field overrides
  on named children is the main mitigation.
- **Name collisions** inside a prefab break addressing. Enforce sibling-unique names on save, as Godot
  does.
- **Nested prefabs** are out of scope; decide deliberately rather than discovering the limitation.
- **`PathGuard`** must cover prefab paths from the start. Materials and packages both had to be
  retrofitted at commit `04b00b4`; do not repeat that.

# What actually happened

Landed as designed. Worth recording:

- The component payload was hoisted into `include/Scene/ComponentBlocks.h` and is now shared with the
  scene serializer. Writing a second copy would have guaranteed drift — add a component, update two
  writers and two readers, and a prefab silently loses what a scene keeps.
- Overrides are stored as a **partial reflected object** per (target, component) rather than one
  entry per field. That shape is exactly what `Mist::Reflect::ReadFields` already consumes, so
  applying overrides and tolerating version skew turned out to be the same operation.
- Override targets resolve through `PrefabMemberComponent::localPath`, not by re-walking the live
  hierarchy by name. Re-deriving the path would break the moment a user renames an instanced child:
  an override should follow the slot it was authored against, not the label.
- Two failures are deliberately non-fatal, because the alternative makes editing a prefab a
  scene-breaking operation: a missing override target is dropped with a warning, and a prefab file
  that fails to load skips that instance rather than aborting the whole scene.
- Correction to the risk list above: the claim that
  "the pattern from `tests/test_scene_serializer.cpp` applies directly" was based on that file having
  no round-trip tests. It has had them since `b542818`.

# How to tell it worked

- Save a three-entity subtree as a prefab, instance it four times, move one: the other three are
  unaffected.
- Edit the prefab's source, reload the scene: all four instances pick up the change, except fields
  that were overridden.
- Save and reload a scene containing instances: instance count, overrides and transforms all survive.
- Tests: prefab round-trip and override application are pure serialization, so headless — the pattern
  from `tests/test_scene_serializer.cpp` applies directly.
