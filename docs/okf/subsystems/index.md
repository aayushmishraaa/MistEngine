---
type: Index
title: Subsystems
description: One parity document per engine subsystem, each holding the Godot model, MistEngine's current state and the delta together.
tags: [parity, index]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
---

# Subsystems

Each document follows the same shape: **Godot model**, **MistEngine today**, **Delta**, **Evidence**,
**Depends on / Blocks**. They are kept in one file per subsystem rather than split across
"Godot" and "MistEngine" trees, so a reader never has to hold two documents open to see a gap.

## Foundational — everything else rests on these

- [Scene and composition](/subsystems/scene-and-composition.md) — the largest single gap
- [Entity identity](/subsystems/entity-identity.md) — names and paths; prerequisite for prefabs
- [Node lifecycle](/subsystems/node-lifecycle.md) — callbacks, process modes, pause
- [Resources and assets](/subsystems/resources-and-assets.md) — loading, caching, serialization

## Rendering

- [Rendering pipeline](/subsystems/rendering-pipeline.md) — the 15-pass frame vs Godot's three renderers
- [Environment and camera](/subsystems/environment-and-camera.md) — scattered settings, camera outside the scene

## Runtime

- [Physics](/subsystems/physics.md)
- [Input](/subsystems/input.md)
- [Scripting](/subsystems/scripting.md)
- [Animation and audio](/subsystems/animation-and-audio.md)

## Tooling

- [Editor tooling](/subsystems/editor-tooling.md)
