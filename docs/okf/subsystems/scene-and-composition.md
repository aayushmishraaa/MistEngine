---
type: Subsystem Parity
title: Scene and composition
description: Godot composes everything from nestable node trees that double as prefabs; MistEngine now has .mistprefab instancing with overrides, but no nesting.
tags: [parity, godot, scene, architecture, prefab]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: https://docs.godotengine.org/en/stable/getting_started/step_by_step/instancing.html
    title: Instancing — Godot docs
  - resource: https://docs.godotengine.org/en/stable/classes/class_node.html
    title: Node — Godot class reference
---

# Godot model

A scene is "a collection of nodes organized in a tree structure, with a single node as its root",
saved as `.tscn`. Once saved it is a **blueprint**: `PackedScene.instantiate()` reproduces it
anywhere, and scenes nest arbitrarily — a house scene contains room instances which contain furniture
instances.

Two properties make this the engine's whole composition model rather than a convenience:

- **Propagation.** Editing the source scene updates every instance on save.
- **Overrides.** An instance may override inherited properties; the inspector marks overridden
  properties with a revert button, and the override survives changes to the template.

Godot explicitly positions this as a replacement for abstract architectural patterns — you compose
by nesting scenes, not by layering MVC.

# MistEngine today

**`.mistprefab` is the engine's first reuse unit.** Per
[decision 0001](/decisions/0001-prefab-asset-model.md) it is a separate asset
type holding an entity subtree, not Godot's "every scene is also instantiable"
model.

The format differs from a scene in **addressing, and only addressing**. A scene
stores integer ids and remaps them through an old→new table on load, because
`CreateEntity` does not hand back the ids that were saved. A prefab stores
sibling-unique **name paths** (`"Turret/Barrel"`), so it is position-independent
and ids never leak between assets. The component payload is identical and comes
from `Scene/ComponentBlocks.h`, shared by both serializers so adding a component
updates one writer rather than two.

What works:

- **Propagation.** A scene stores the *reference plus overrides*, never the
  expanded subtree. `SceneSerializer::Save` skips every entity an instance
  spawned and writes a `prefab` block; Load re-instantiates from the source as
  it is on disk now. Edit the prefab, reload the scene, every instance updates.
  Flattening would have made a prefab a one-time copy — which the Hierarchy's
  "Duplicate" already was.
- **Overrides**, keyed on (local name path, component type, field name). The
  reflection layer already gave the last two by name. Stored as a *partial*
  reflected object per (target, component), which is exactly the shape
  `Mist::Reflect::ReadFields` consumes: "apply only the overridden fields" and
  "tolerate fields an older file lacks" are the same operation, so overrides
  reuse it rather than reimplementing the switch.
- **Sibling-unique names enforced at save time.** A duplicate makes an override
  path ambiguous, so saving fails with an error naming the offending path rather
  than producing an asset whose overrides land on whichever child sorts first.
- **`PathGuard` from the first commit**, not retrofitted. Materials and packages
  both had to be fixed after the fact at commit `04b00b4`.
- **Editor surface**: "Save as Prefab..." on a hierarchy subtree, `.mistprefab`
  drag-drop onto the viewport, an Inspector block showing the source path and
  every overridden field with a revert button, and undo for instancing and
  reverting. Deleting any member of an instance deletes the whole instance —
  destroying one child would leave the scene holding a reference whose expansion
  no longer matches its source.
- **`PackageIO`** includes referenced prefabs and the materials inside them in
  its dependency walk.

Failure handling is deliberately non-fatal in two places, because the
alternative makes editing a prefab a scene-breaking operation: an override whose
target no longer exists is dropped with a warning, and a prefab file that fails
to load skips that instance rather than aborting the scene.

# Remaining delta

- **Nested prefabs** are out of scope: a prefab containing an instance of
  another is a later question, decided rather than discovered.
- **Structural overrides** — adding or removing a child on an instance — are out
  of scope. This is where prefab systems genuinely rot, because they force
  propagation rules that survive the source gaining or losing nodes. Overrides
  are scalar and vector *fields* only.
- **Reverting takes effect on reload.** Clearing an override updates the stored
  table, but the live component still holds the overridden value until the
  prefab is re-read. The editor says so rather than letting the button look
  broken.
- The Hierarchy's **"Duplicate"** still copies only Transform and Render,
  silently dropping Physics, Light, Animation and Script, and pushes no undo
  command. Prefabs are the intended answer, but Duplicate remains as it was.

# Evidence

- `include/Assets/PrefabSerializer.h`, `src/Assets/PrefabSerializer.cpp` — the format.
- `include/Assets/PrefabOverrides.h`, `src/Assets/PrefabOverrides.cpp` — apply / record / clear.
- `include/ECS/Components/PrefabComponents.h` — `PrefabInstanceComponent` (serialized reference
  plus override table) and `PrefabMemberComponent` (runtime-only membership marker).
- `include/Scene/ComponentBlocks.h` — the component payload, shared with the scene serializer.
- `src/Scene/SceneSerializer.cpp` — skips expanded instance content, writes the `prefab` block.
- `tests/test_prefab.cpp` — 16 headless cases including propagation, override precedence over
  propagation, and PathGuard escapes.
- `src/Import/SceneImporter.cpp:160` `importNodeTree` — builds a tree, but the result is plain
  entities with no template identity.
- `src/UIManager.cpp` Hierarchy context menu "Duplicate" — copies `TransformComponent` and
  `RenderComponent` only.
- `src/ECS/Systems/HierarchySystem.cpp:41` — parent composition, wired into the render path as of
  commit `ea91d7c`.

# Depends on / Blocks

Both prerequisites landed first, as planned: [entity identity](/subsystems/entity-identity.md) gave
overrides something to address, and [environment and camera](/subsystems/environment-and-camera.md)
made a prefab able to contain a viewpoint.

Implemented in [phase 3](/roadmap/phase-3-prefabs.md).
