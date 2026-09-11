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

# Status: **done**

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

# What actually happened

All six wired. Deviations worth recording:

**1. `InputSystem::Init` no longer installs GLFW callbacks.** It used to call `glfwSetKeyCallback` /
`MouseButton` / `Scroll` / `CursorPos`, which only coexists with ImGui's backend in one specific
order — and `src/InputManager.cpp` documents at length that callback conflicts with ImGui are
precisely why the live input path was rewritten to pure polling. `Update()` now polls
`glfwGetKey` / `glfwGetMouseButton`, making the system independent of callback ordering entirely.
Consequence: `GetScrollDelta()` stays 0 unless a host installs `ScrollCallback`; GLFW has no polling
equivalent for scroll, and the editor camera's zoom already comes from `Renderer`'s own callback.

**2. The W/E collision fix is a deliberate behaviour change.** Viewport navigation is now modal: the
camera-fly context is pushed only while the right mouse button is held, as Godot does. **WASD no
longer moves the camera on its own.** Outside RMB the letters belong to the gizmo, dispatched through
`ShortcutRegistry`.

**3. `InputManager` keeps a raw-key fallback.** When no `InputSystem` is attached it still polls
`glfwGetKey` directly, so a host embedding the engine without one gets a working camera.

**4. Play mode snapshots to a string, not a temp file.** An editor feature should not depend on the
scene sandbox or on disk being writable. `SaveToString`/`LoadFromString` landed in phase 1 for the
round-trip tests, so this reused them. Restoring also clears selection and undo history — both hold
entity ids that no longer refer to anything after a world rebuild.

**5. `_ready` is gated too**, not just `_process` and physics. The physics accumulator still drains
while paused, or the first frame after Play would fire a burst of catch-up steps and launch
everything.

**6. One real in-tree plugin** (`src/Editor/Plugins/SceneStatsPlugin.cpp`). The registry's only
`IEditorPlugin` was a test double, so the extension seam could be asserted to compile but not shown
to work.

**7. Extractions that were prerequisites, not polish.** `New Scene`, `Duplicate` and `Save As` were
inlined in menu bodies and had to become methods before a shortcut could call them. Fixing
`New Scene` was not optional: it cleared a UI cache and the selection **without destroying a single
entity**, so the previous scene kept rendering. `Duplicate` also gained the undo command it never
had.

# Wire-or-delete: the four left undecided

Reported rather than decided, as agreed.

| Item | State | Recommendation |
|---|---|---|
| `include/ECS/Event.h` (`EventBus`) | Header-only, **included by nothing at all** — not even `Coordinator`. Does not compile into the library. | **Delete.** `Core/Signal.h` is the better primitive, is now used for four engine signals, and has a test. Two event mechanisms is one too many. |
| `ECS/SystemScheduler` | Topological ordering with cycle detection, no callers, no test. | **Delete for now.** It cannot drive anything: `System` exposes `Update(float)` while every real system has a bespoke signature (`RenderSystem::Update(Shader&)`, `LightSystem::Update(Coordinator&, LightManager&)`). Wiring it means redesigning the system interface first — a real piece of work, not a wiring job, and better restarted than resurrected. |
| `Core/CommandQueue.h` | Header-only, referenced only by its test. | **Keep.** It is shaped for the render thread the async-server model needs, the test documents the contract, and it costs nothing compiled. Delete it only if threaded rendering comes off the roadmap. |
| Audio stack | `MIST_ENABLE_AUDIO` defaults **OFF**; `AudioSourceComponent` is never registered; `AudioSystem` is declared inside its own `.cpp` with a non-ECS signature; `AssetRegistry::audio()` has no loader. `test_audio_clip.cpp` asserts POD defaults only. | **Wire, in its own pass.** This is four connected jobs (register the component, give `AudioSystem` a header and an ECS signature, install an audio loader, default the option ON), not a wiring one-liner. Deleting it would discard a working OpenAL backend. Out of scope here. |

# How to tell it worked

- Rebind "MoveForward" to a different key at runtime and fly the camera with it.
- Press `W` with an entity selected: the gizmo switches to translate and the camera does **not** move.
- Press Ctrl+S: the scene saves. Press F5: play mode starts, physics runs; F8 stops and the scene is
  restored to its pre-play state.
- Edit `pbr_fragment.glsl` while the editor is running: the change appears without a restart.
- `grep -rn "<subsystem>" src | grep -v "own .cpp"` returns a live call site for each item.
