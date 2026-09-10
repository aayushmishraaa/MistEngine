---
type: Architecture Decision
title: 0001 — Prefab asset model
description: Prefabs are a separate .mistprefab asset type rather than Godot's model where every scene is also instantiable.
tags: [decisions, architecture, scene, prefab]
status: stable
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: https://docs.godotengine.org/en/stable/getting_started/step_by_step/instancing.html
    title: Instancing — Godot docs
  - resource: /subsystems/scene-and-composition.md
    title: Scene and composition
---

# Decision

MistEngine's reuse unit will be a **separate `.mistprefab` asset type** holding an entity subtree.
A `.mist` scene stays a scene; it is not itself instantiable.

Status: **decided**. Implementation is [phase 3](/roadmap/phase-3-prefabs.md).

# Alternative rejected

Godot's model, where a `.tscn` scene *is* the prefab: any saved scene can be instanced inside
another, recursively, and edits to the source propagate to every instance.

That model is more powerful and is what the rest of the engine imitates. It was rejected for
this reason: the duality is the expensive part. "A scene is also a prefab" means every scene needs
stable per-node local identity, instance-level property override tracking, and propagation rules
that survive the source scene gaining or losing nodes. MistEngine currently has no stable entity
identity at all — names are not even serialized — so adopting the harder model first would mean
building override semantics on a foundation that does not exist yet.

# Consequences

Accepted:

- Two asset types to document, load, and show in the asset browser instead of one.
- No recursive nesting in the first cut: a prefab containing a prefab instance is a later question,
  not a day-one guarantee.
- Diverges from Godot here, so this bundle's parity framing has one deliberate exception. Worth
  stating plainly rather than describing the result as Godot-equivalent.

Gained:

- A prefab can define its own override surface explicitly, rather than every property of every node
  being implicitly overridable.
- Scene save/load stays a simpler problem — it only became round-trip safe at commit `b542818`, and
  keeping scenes non-instantiable avoids re-destabilising it.

# Revisit when

Nested prefab instances are needed, or instance override propagation turns out to want the same
machinery scenes would need anyway. At that point the two types can converge without breaking
existing assets, since a scene is structurally a superset of a prefab.
