---
okf_version: "0.2"
type: Knowledge Bundle
title: MistEngine — Godot Parity
description: Where MistEngine stands against Godot 4 as a reference engine architecture, and the dependency-ordered roadmap to close the gap.
tags: [parity, godot, architecture, roadmap]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: https://docs.godotengine.org/en/stable/
    title: Godot Engine documentation (stable)
  - resource: https://github.com/GoogleCloudPlatform/open-knowledge-format/blob/main/SPEC.md
    title: Open Knowledge Format v0.2 specification
---

# What this bundle is

A parity model for MistEngine measured against **Godot 4**, chosen as the reference because the
engine has been explicitly imitating it — the commit history is full of "Godot-parity" and
"Godot-shaped" work, but the comparison had never been written down, so "parity" had no definition
and no way to measure progress.

This is the *architecture* axis. Defects live in `CODE_AUDIT.md` (untracked, repo root) and are
referenced here rather than duplicated.

Tree state: commit `04b00b4`. Every `# Evidence` claim below was checked against that tree.

# Headline

**The renderer is ahead of Godot's Compatibility tier. The engine architecture is largely absent.**

**All four roadmap phases have landed.** entity names are a serialized component with a lookup and a Lua binding, and
every render tunable now lives on one serialized `Environment`. The camera is a component, and all six
geometry passes cull, `.mistprefab` gives the engine its first reuse unit, and the six built-but-dead
subsystems are wired. The table below tracks state as of that work; see
[the roadmap](/roadmap/index.md) for what each phase did and did not cover.

On rendering, MistEngine is closer to Godot's **Forward+** than to Compatibility: clustered lighting
for 1024 lights where Compatibility caps at 8 per mesh, PCSS where Compatibility has none, plus TAA,
SSR and compute shaders that Compatibility also lacks. That ambition is real.

What is missing is the spine Godot is built on — a composable scene model, stable entity identity,
a camera that lives in the scene, one place for environment settings, frustum culling, and input
actions. Several of those are *already written and simply never called*.

# Parity at a glance

| Subsystem | State | Detail |
|---|---|---|
| [Scene and composition](/subsystems/scene-and-composition.md) | partial | .mistprefab instancing + overrides; no nesting |
| [Entity identity](/subsystems/entity-identity.md) | partial | names serialized + lookup; no NodePath or groups |
| [Node lifecycle](/subsystems/node-lifecycle.md) | partial | two of Godot's nine callbacks |
| [Resources and assets](/subsystems/resources-and-assets.md) | partial | one loader installed of four |
| [Rendering pipeline](/subsystems/rendering-pipeline.md) | ahead / behind | Forward+-class lighting + culling; no GI, probes or LOD |
| [Environment and camera](/subsystems/environment-and-camera.md) | partial | both serialized; still one viewport |
| [Physics](/subsystems/physics.md) | partial | no layers, areas or character body |
| [Input](/subsystems/input.md) | partial | action system wired + serialized; no event propagation |
| [Scripting](/subsystems/scripting.md) | strong | the most complete subsystem in the engine |
| [Editor tooling](/subsystems/editor-tooling.md) | partial | plugins, shortcuts and play mode wired; no property groups |
| [Animation and audio](/subsystems/animation-and-audio.md) | absent | bind pose only; audio unreachable |

# Where to start

[Roadmap](/roadmap/index.md) — four dependency-ordered phases. Three of them are unbuildable before
their predecessors land, which is the main reason this bundle exists.

Architectural decisions already taken are in [decisions](/decisions/index.md).

# Reading this bundle

Every document carries `status: draft` and `generated.by` an agent, which places the whole bundle in
OKF's **unverified** trust tier. Adding a `verified` entry with a `human:` actor promotes a document
to **human-reviewed** — the bundle tracks its own review state rather than implying authority it has
not earned.
