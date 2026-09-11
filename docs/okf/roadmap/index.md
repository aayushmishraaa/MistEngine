---
type: Index
title: Roadmap
description: Four dependency-ordered phases toward Godot-shaped engine architecture, three of which are unbuildable before their predecessors.
tags: [roadmap, index, planning]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
---

# Roadmap

Ordered by dependency, not by value. That ordering is the main reason this bundle exists: the
highest-value item (prefabs) cannot be built until two cheaper things land first, and the cheapest
item (wiring dead subsystems) is independent of everything and therefore goes last.

| Phase | Theme | Prerequisite |
|---|---|---|
| [1](/roadmap/phase-1-identity-and-environment.md) — **done** | Entity identity + Environment resource | none |
| [2](/roadmap/phase-2-camera-and-culling.md) | Camera as a component + frustum culling | phase 1 |
| [3](/roadmap/phase-3-prefabs.md) | `.mistprefab` instancing | phases 1 and 2 |
| [4](/roadmap/phase-4-wire-the-built-but-dead.md) | Wire six already-written subsystems | none — independent |

# Why this order

**Phase 1 first** because a prefab whose contents cannot be named is unreferenceable, and a scene
that cannot store its own lighting is not portable. Both are prerequisites disguised as polish.

**Phase 2 before 3** because a prefab containing a camera is meaningless while the camera is a
`Renderer` member, and because culling wants the entity-subtree bounds that prefabs will also need.

**Phase 4 last despite being cheapest.** Six subsystems are already written, unit-tested, and
unreachable from `main()`. It is the best parity-per-hour in the tree, but it is independent of
phases 1–3, so it can be slotted in whenever momentum is needed without blocking anything.
