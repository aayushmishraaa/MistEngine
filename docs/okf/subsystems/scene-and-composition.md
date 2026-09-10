---
type: Subsystem Parity
title: Scene and composition
description: Godot composes everything from nestable node trees that double as prefabs; MistEngine has one flat entity list and no reuse unit at all.
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

One flat list of entities in a single global `Coordinator`. There is no reuse unit of any kind: no
prefab, no template, no sub-scene, no way to say "another one of those".

Parent/child relationships exist via `HierarchyComponent`, and since commit `ea91d7c` they actually
affect rendering — before that `cachedGlobal` was computed and read by nothing. `SceneImporter` does
build a real entity tree from an Assimp node hierarchy. So the *tree* exists; the *instancing* does
not.

Duplicating content means one of: re-importing the source model, re-running a Lua spawn function, or
the Hierarchy panel's "Duplicate" context item — which copies only Transform and Render, silently
dropping Physics, Light, Animation and Script, and pushes no undo command.

# Delta

The gap is not "scenes cannot nest" — it is that **nothing can be reused**. Consequences that show
up immediately in practice:

- A level must be authored entirely in Lua (`scripts/showcase.lua`) or re-imported from a DCC tool.
  There is no third option.
- A fix to a repeated object must be applied by hand to every copy.
- The asset browser can drop a model onto the viewport, but the result is an anonymous entity tree
  with no link back to anything reusable.

This is the largest single architectural gap in the engine, and it is also the one with the most
prerequisites — see [decision 0001](/decisions/0001-prefab-asset-model.md) for the chosen model and
why Godot's exact approach was rejected.

# Evidence

- `include/ECS/Coordinator.h` — flat entity storage; no scene or sub-scene concept.
- `grep -rin "prefab\|packedscene\|instantiate" include src` → only
  `Script/ScriptLanguage.h:41`, an unrelated use of the word in a comment about compiling scripts.
- `src/Import/SceneImporter.cpp:160` `importNodeTree` — builds a tree, but the result is plain
  entities with no template identity.
- `src/UIManager.cpp` Hierarchy context menu "Duplicate" — copies `TransformComponent` and
  `RenderComponent` only.
- `src/ECS/Systems/HierarchySystem.cpp:41` — parent composition, wired into the render path as of
  commit `ea91d7c`.

# Depends on / Blocks

Blocked by [entity identity](/subsystems/entity-identity.md) — a prefab whose contents cannot be
named or addressed cannot express an override.

Blocked by [environment and camera](/subsystems/environment-and-camera.md) for the camera half: a
prefab containing a viewpoint is meaningless while `Camera` is a `Renderer` member.

Blocks: practically all content authoring.
Implementation: [phase 3](/roadmap/phase-3-prefabs.md).
