---
type: Subsystem Parity
title: Entity identity
description: Godot nodes have names, unique paths and an owner; MistEngine entity names live in an editor-only map and are never written to disk.
tags: [parity, godot, scene, serialization]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: https://docs.godotengine.org/en/stable/classes/class_node.html
    title: Node — Godot class reference
---

# Godot model

Every node has a `name` (a `StringName`, unique among siblings) and therefore a unique `NodePath`
from the scene root. Addressing is first-class:

- `get_node(path)` / `get_node_or_null(path)` / `has_node(path)`
- `get_path()` and `get_path_to(node)`
- `unique_name_in_owner`, which prefixes a node with `%` so it can be fetched without its full path
- `owner`, recording which scene instantiated the node — the mechanism editors and tools rely on to
  know what belongs to a saved scene versus what was added to an instance

Nodes also carry **groups** (`add_to_group`, `SceneTree.get_nodes_in_group`) as a tagging mechanism
orthogonal to the tree.

# MistEngine today

Entities are bare `std::uint32_t` handles. Names exist only as
`std::unordered_map<Entity, std::string> m_EntityNames` **inside `UIManager`** — the editor, not the
engine. The consequences:

- **Names are not serialized.** `include/Scene/SceneSerializer.h:22` documents a `"name": "Ground"`
  field in its own format example; the Save path never writes one. Save a scene, reload it, and every
  entity is "Entity 7".
- **Nothing can address an entity by name.** A Lua script cannot find a sibling; a prefab cannot say
  which child to override; the console cannot refer to an object.
- **No groups or tags.**
- **No `owner` equivalent**, which is precisely the field a prefab system needs to distinguish
  template content from instance-local additions.

Entity ids themselves are now at least sane: `EntityManager::DestroyEntity` gained a liveness guard
at commit `b542818`, so a double-destroy no longer hands the same id to two entities. But an id is
not an identity — it is not stable across a save/load cycle, which is why the scene serializer has to
remap hierarchy links through an old→new table.

# Delta

Identity is the quiet prerequisite under several larger features. Ranked by what it unblocks:

1. **Prefab overrides** need to name the thing being overridden.
2. **Scripting ergonomics** — Lua currently has `entity_id()` for *self* and nothing for anything
   else. There is no `get_node("../Door")`.
3. **Readable scene files** — a `.mist` file is currently numeric ids and component blobs.
4. **Editor usability** — renaming is already in the UI but lost on save, which reads as a bug.

Minimum viable parity is narrow and cheap: move names into a component, serialize them, and add a
name→entity lookup. Full `NodePath` resolution can follow.

# Evidence

- `include/UIManager.h:290` — `m_EntityNames` is a private `UIManager` member.
- `include/Scene/SceneSerializer.h:22` — documents a `"name"` field.
- `src/Scene/SceneSerializer.cpp` Save loop — writes `id`, `transform`, `render`, `physics`,
  `light`, `hierarchy`, `animation`. No `name`.
- `src/Scene/SceneSerializer.cpp` Load — builds an `idRemap` old→new table precisely because saved
  ids are not stable identity (added at commit `b542818`).
- `src/Script/LuaScriptLanguage.cpp` — `entity_id()` returns the current entity only; no lookup
  binding exists.

# Depends on / Blocks

Depends on: nothing. This is the cheapest foundational item in the engine.

Blocks: [scene and composition](/subsystems/scene-and-composition.md),
[scripting](/subsystems/scripting.md) ergonomics, readable scene files.
Implementation: [phase 1](/roadmap/phase-1-identity-and-environment.md).
