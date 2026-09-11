---
type: Subsystem Parity
title: Entity identity
description: Godot nodes have names, unique paths and an owner; MistEngine now has a serialized NameComponent and name lookup, but no NodePath, groups or owner.
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

Entities are bare `std::uint32_t` handles, but names are now a first-class
engine concept rather than an editor detail.

`NameComponent` (`include/ECS/Components/NameComponent.h`) holds a
`std::string`, is registered in `main()` alongside the other components, and is
reflected — so the Inspector widget and serializer support came for free, the
same way `RenderComponent::meshPath` did at commit `b542818`.

What that changed:

- **Names serialize.** `SceneSerializer` writes a `"name"` field and reads it
  back, which makes the format example in `include/Scene/SceneSerializer.h` —
  which had documented that field since v0.5 without ever writing it — honest.
  Untitled entities emit nothing rather than an empty string.
- **`UIManager::m_EntityNames` is gone.** The editor's private
  `std::unordered_map<Entity,std::string>` has no remaining references. The
  Hierarchy labels, the Inspector rename field, duplicate-with-" (copy)", and
  the undo snapshot all read and write the component. Deleting an entity no
  longer needs a paired `erase` — the name dies with the entity, which removed
  five leak-shaped call sites.
- **`SceneImporter` writes names into the ECS.** Its
  `std::unordered_map<Entity,std::string>* outNames` out-param is replaced by a
  `bool nameEntities`, so imported aiNode/aiMesh names survive a save.
- **Lua can address other entities.** `find_entity(name)`, `entity_name(id)`
  and `set_entity_name(id, name)` sit beside `entity_id()`. This is the seed of
  `get_node()`.

Lookup is `Mist::FindEntityByName` (`include/ECS/EntityName.h`), a linear scan.
That is deliberate: a name index has to be invalidated on create, destroy and
rename, and getting that wrong hands out stale entity ids — the same class of
bug the liveness guard at `b542818` had to fix.

Entity ids themselves remain non-identity: they are not stable across a
save/load cycle, which is why the scene serializer still remaps hierarchy links
through an old→new table. Names are now what a test — or a prefab override —
addresses across that boundary.

# Remaining delta

Against Godot, still missing:

- **No `NodePath` resolution.** `get_node("../Door")` needs a hierarchy walk;
  only flat name lookup exists.
- **No sibling-uniqueness rule.** Godot guarantees names are unique among
  siblings. MistEngine does not, so `FindEntityByName` returns "first match in
  iteration order". Uniqueness will be enforced where it actually matters —
  inside a prefab, where a duplicate name makes an override ambiguous.
- **No groups or tags.**
- **No `owner` equivalent**, the field distinguishing template content from
  instance-local additions. [Prefabs](/roadmap/phase-3-prefabs.md) introduce
  this as a runtime membership marker rather than a general `owner`.
- **No `unique_name_in_owner` (`%Node`) shorthand.**

# Evidence

- `include/ECS/Components/NameComponent.h` — the component and its
  `MIST_REFLECT` block.
- `include/ECS/EntityName.h` — `EntityName`, `HasEntityName`, `SetEntityName`,
  `FindEntityByName`, `kInvalidEntity`.
- `src/MistEngine.cpp` — `RegisterComponent<NameComponent>()`.
- `src/Scene/SceneSerializer.cpp` — the `name` block in both Save and Load.
- `grep -rn "m_EntityNames" src include` → no results.
- `tests/test_scene_serializer.cpp` — four identity round-trip cases, headless
  via the new `SaveToString` / `LoadFromString` overloads.
- `tests/test_lua_script.cpp` — `find_entity` / `set_entity_name` coverage.

# Depends on / Blocks

Depends on: nothing. This was the cheapest foundational item in the engine.

Unblocked by this landing: [scene and composition](/subsystems/scene-and-composition.md)
can now express a prefab override target, [scripting](/subsystems/scripting.md)
can address a sibling, and scene files are readable.
Implemented in [phase 1](/roadmap/phase-1-identity-and-environment.md), part A.
