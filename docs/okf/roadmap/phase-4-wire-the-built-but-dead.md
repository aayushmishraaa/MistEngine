---
type: Roadmap Phase
title: Phase 4 — Wire the built-but-dead
description: Six subsystems are already written, unit-tested and unreachable from main(). The best parity-per-hour in the tree, and independent of every other phase.
tags: [roadmap, phase, input, editor, cleanup]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: /subsystems/input.md
    title: Input
  - resource: /subsystems/editor-tooling.md
    title: Editor tooling
---

# Prerequisite

None. Independent of phases 1–3, which is why it is last — it can be slotted in whenever momentum is
needed, without blocking anything.

# The pattern

This is the dominant structural finding of `CODE_AUDIT.md`: roughly a third of the non-vendored source
is unreachable from `main()`. Several of these subsystems have dedicated unit tests, which is exactly
why nothing flags them — the suite exercises components in isolation and stays green while the
application never calls them.

Each item below is a wiring job, not a design job.

# Items, in descending value

**1. `InputSystem` → InputMap parity.** The highest-value item. Action maps, multiple bindings per
action, gamepad support with deadzone, a context stack, and `RebindAction` are all written
(`include/Input/InputSystem.h`, `InputAction.h`, `InputContext.h`) with prebuilt editor and gameplay
contexts. Zero callers. Wiring it means: instantiate it in `main()`, replace `InputManager`'s
hardcoded `glfwGetKey` polling with action queries, serialize the binding table so it is remappable,
and expose actions to Lua. This also resolves the `W`/`E`/`R` collision where the gizmo keys and the
camera WASD fight each other — the context stack is precisely the mechanism for that.

**2. Play mode.** `EditorState` models Edit / Playing / Paused with snapshot callbacks.
`SetSnapshotCallbacks` is never called and nothing reads `ShouldUpdateGame()`, so the toolbar buttons
mutate an enum and nothing else. Wiring: install snapshot/restore using `SceneSerializer` (round-trip
safe since commit `b542818`), gate physics and script updates in `main()` on `ShouldUpdateGame()`.
Needs process modes to be done properly — see [node lifecycle](/subsystems/node-lifecycle.md) — but a
global gate is a usable first cut.

**3. Shortcut dispatch.** `ShortcutRegistry` already renders correct chord labels into menus, but
`Matches` / `FindByChord` are called only from tests, so Ctrl+S, Ctrl+N, Ctrl+O, Ctrl+D, Delete, F5
and F8 are displayed and not dispatched. Wiring is one dispatch site in `UIManager::NewFrame`.
Small, and removes a standing lie in the UI.

**4. Editor plugin docks.** `EditorPluginRegistry` works — `OnEnable`/`OnDisable`, dock and menu
registration, thread-safe snapshots — and is called only from its test. `UIManager` needs to iterate
`SnapshotDocks()` in `DrawEditorLayout` and `SnapshotMenus()` in `DrawMainMenuBar`. That makes the
editor extensible, which is the point of having written it.

**5. Per-object signals.** `Mist::Signal<T>` is solid and exactly one instance exists engine-wide
(`HierarchySystem::OnReady`). Candidates that currently have no notification mechanism: entity
created/destroyed, component added/removed, selection changed, scene loaded. Low risk, and it is what
lets the editor stop polling.

**6. Shader hot-reload.** `ShaderManager::PollAndReload` runs every 30 frames over a permanently empty
registry, because `Register` has zero callers. One registration call per `Shader` construction in
`Renderer::Init` turns a no-op into a working feature.

# Deletions to pair with it

Wiring should be accompanied by removing the things that will never be wired, because leaving them is
what makes the codebase read as larger than it is:

- `src/Editor/EditorUI.cpp` — 226 lines of `namespace EditorPanels` duplicating `UIManager` members,
  never referenced.
- `UIManager::DrawSceneView` / `DrawAssetBrowser` / `DrawConsole` / `DrawCrosshair` — each appears once,
  as its own definition. Note `DrawSceneView` is the only caller of
  `Renderer::SetFullscreenPresent`, so the entire `Viewport` fullscreen-blit path is dead with it.
- The `#if 0` FPS block in `UIManager.cpp`.
- `src/Core/Engine.cpp` — looks like the composition root, never instantiated; `main()` is the real one.
- `SceneGraph` — but only *after* [phase 2](/roadmap/phase-2-camera-and-culling.md) lifts its frustum
  culling out, which is the one genuinely valuable thing in it.
- Decide explicitly on `EventBus`, `SystemScheduler` and the audio stack: wire or delete, not leave.

# How to tell it worked

- Rebind "MoveForward" to a different key at runtime and fly the camera with it.
- Press `W` with an entity selected: the gizmo switches to translate and the camera does **not** move.
- Press Ctrl+S: the scene saves. Press F5: play mode starts, physics runs; F8 stops and the scene is
  restored to its pre-play state.
- Edit `pbr_fragment.glsl` while the editor is running: the change appears without a restart.
- `grep -rn "<subsystem>" src | grep -v "own .cpp"` returns a live call site for each item.
