---
type: Subsystem Parity
title: Node lifecycle
description: Godot gives every node nine ordered callbacks plus process modes and pause; MistEngine has two, available only to Lua scripts.
tags: [parity, godot, lifecycle, ecs, scripting]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: https://docs.godotengine.org/en/stable/classes/class_node.html
    title: Node — Godot class reference
---

# Godot model

Ordered callbacks, with guarantees:

| Callback | Ordering guarantee |
|---|---|
| `_enter_tree()` | parent before children |
| `_ready()` | **reverse** — children before parents |
| `_process(delta)` | every rendered frame |
| `_physics_process(delta)` | fixed rate, ~60 Hz |
| `_input(event)` | before GUI |
| `_shortcut_input` / `_unhandled_key_input` / `_unhandled_input` | after GUI, in that order |
| `_exit_tree()` | leaving the tree |

Underneath sits a notification system (`NOTIFICATION_ENTER_TREE` = 10, `NOTIFICATION_READY` = 13,
`NOTIFICATION_PAUSED` = 14, `NOTIFICATION_PARENTED` = 18, …) that C++ and scripts both observe.

Two controls layer on top:

- **Process modes** — `PROCESS_MODE_INHERIT` / `PAUSABLE` / `WHEN_PAUSED` / `ALWAYS` / `DISABLED`,
  which is how `SceneTree.paused` yields a working pause menu without per-node bookkeeping.
- **Process thread groups** — `MAIN_THREAD` or `SUB_THREAD`, opting a subtree into threaded
  processing.

Per-node toggles (`set_process`, `set_physics_process`) and lifecycle signals (`tree_entered`,
`tree_exiting`, `child_entered_tree`, `renamed`) complete the surface.

# MistEngine today

Two callbacks, and only for Lua `ScriptComponent`s:

- `_ready()` — fired once per entity via `HierarchySystem::OnReady`, genuinely in Godot's reverse
  post-order, which is a nice piece of fidelity.
- `_process()` — every frame, from `ScriptSystem::Update`.

Absent: `_enter_tree` / `_exit_tree`, `_physics_process` (scripts cannot run at the fixed 60 Hz tick
even though the engine has one), any input callback, the notification system, process modes, pause,
and thread groups.

C++ systems have no lifecycle at all — `System` exposes a single `Update(float)` and most subsystems
bypass even that with bespoke signatures (`RenderSystem::Update(Shader&)`,
`LightSystem::Update(Coordinator&, LightManager&)`), so `SystemScheduler` — which does implement
dependency-ordered execution with cycle detection — cannot drive any of them. It is unreachable from
`main()`.

One ordering subtlety was fixed at commit `ea91d7c`: `_process` now runs *before*
`UpdateTransforms`, so script-driven movement composes in the same frame rather than a frame late.

# Delta

- **No pause.** There is no `SceneTree.paused` equivalent and no process modes, so a pause menu
  cannot be built without every system growing its own flag. This is also why the editor's
  Play/Pause/Stop buttons do nothing — see [editor tooling](/subsystems/editor-tooling.md).
- **No `_physics_process`.** Gameplay code that needs the fixed tick cannot have it, despite
  `src/MistEngine.cpp` running a correct fixed-step accumulator.
- **No enter/exit tree.** Scripts cannot react to being added or removed, so cleanup has no hook.
- **No input callbacks**, so all input handling lives in the engine rather than in scripts.

# Evidence

- `src/ECS/Systems/ScriptSystem.cpp` — `_ready` via the `OnReady` signal, `_process` per frame.
  Nothing else.
- `src/ECS/Systems/HierarchySystem.cpp:55` `ReadyPostOrder` — children-before-parents ordering.
- `include/ECS/System.h` — the base class is `virtual void Update(float)` and a `std::set<Entity>`.
- `include/ECS/SystemScheduler.h`, `src/ECS/SystemScheduler.cpp` — topological ordering with cycle
  detection, no callers outside itself.
- `src/MistEngine.cpp` — fixed 60 Hz accumulator clamped at 0.25 s, used only by the two physics
  systems.

# Depends on / Blocks

Blocks: pause, play-mode semantics, script-side input.
Partially addressed by [phase 4](/roadmap/phase-4-wire-the-built-but-dead.md), which wires play mode;
full lifecycle parity is not scheduled.
