---
type: Subsystem Parity
title: Input
description: Godot abstracts input into remappable named actions; MistEngine has a complete action system with zero callers and hardcodes keys instead.
tags: [parity, godot, input]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: https://docs.godotengine.org/en/stable/tutorials/inputs/inputevent.html
    title: InputEvent — Godot docs
---

# Godot model

A typed `InputEvent` hierarchy — `InputEventKey`, `InputEventMouseButton`, `InputEventMouseMotion`,
`InputEventJoypadButton`, `InputEventJoypadMotion`, `InputEventScreenTouch`, `InputEventAction`.

On top sits **`InputMap`**: named actions, defined in Project Settings, each grouping zero or more
events. Godot's own example is `ui_left` binding both the keyboard left arrow and joypad-left. Because
gameplay code asks about *actions*, the same code works across devices and can be remapped at runtime
with no code change. Actions carry a deadzone.

The `Input` singleton answers `is_action_pressed`, `is_action_just_pressed`, `get_axis` and
`get_vector`, and `parse_input_event` lets code synthesise actions.

Events propagate through the tree in **reverse depth-first order** so children consume before
ancestors, through a defined chain: window management → `_input` → `Control._gui_input` →
`_shortcut_input` → `_unhandled_key_input` → `_unhandled_input` → physics picking. A handler stops
propagation with `Viewport.set_input_as_handled()` or `Control.accept_event()`.

# MistEngine today

**`InputSystem` is wired.** Action maps, multiple bindings per action, keyboard /
mouse / gamepad devices with a 0.15 deadzone, a context stack with higher
contexts shadowing lower ones, and `RebindAction` — all of it now has a caller.

- Instantiated in `main()` and polled once per frame.
- `Init()` deliberately installs **no** GLFW callbacks. It used to, which only
  coexists with ImGui's backend in one specific order — and `InputManager`
  documents that callback conflicts with ImGui are exactly why the live path was
  rewritten to pure polling. `Update()` polls `glfwGetKey` /
  `glfwGetMouseButton` instead, so the system is independent of callback
  ordering. Consequence: `GetScrollDelta()` stays 0 unless a host installs
  `ScrollCallback`, since GLFW has no polling equivalent for scroll.
- `InputManager::ProcessCameraMovement` queries named actions instead of
  hardcoded `glfwGetKey` calls, with the raw path kept as a fallback for a host
  that attaches no `InputSystem`.
- **The binding table is serialized.** `input_map.json` is merged over the
  built-in defaults at startup, so a rebind survives a restart and a newly added
  action still gets its default. Guarded under the project root.
- **Lua has input**: `is_action_pressed`, `is_action_just_pressed`, `get_axis`
  and `get_vector`, all degrading to "nothing pressed" when no `InputSystem` is
  live so a headless run cannot crash on them.

**The W/E collision is fixed by the context stack**, which is what it was for.
Viewport navigation is now modal: the camera-fly context is pushed only while the
right mouse button is held, as Godot does. Outside RMB the letters belong to the
gizmo, dispatched through `ShortcutRegistry`. This is a deliberate behaviour
change — WASD no longer flies the camera on its own.

# Remaining delta

- **No event propagation or consumption model.** No tree walk, no
  `set_input_as_handled()`. This matters much less without a `Control` UI tree,
  but input is still read in several places (`InputManager`, `Renderer`'s GLFW
  callbacks, `UIManager`, `main()`).
- **No `_input` / `_unhandled_input` script callbacks** — scripts poll actions
  rather than receiving events. See [node lifecycle](/subsystems/node-lifecycle.md).
- **No rebinding UI.** The table is serializable and `RebindAction` works; there
  is no panel to drive them from.
- Scroll is not available through `InputSystem`, per the polling trade-off above.

# Evidence

- `src/MistEngine.cpp` — instantiation, the saved-map merge, the RMB-gated
  context push, and the per-frame `Update()`.
- `src/Input/InputSystem.cpp` `Init` / `Update` — the no-callbacks decision and
  the polling loop.
- `include/Input/InputMapSerializer.h`, `src/Input/InputMapSerializer.cpp`.
- `src/InputManager.cpp` `ProcessCameraMovement` — action queries with a raw
  fallback.
- `src/Script/LuaScriptLanguage.cpp` — the four action bindings.
- `tests/test_input_system.cpp` — 10 cases; the subsystem had none before.

# Depends on / Blocks

Depended on nothing. Implemented in
[phase 4](/roadmap/phase-4-wire-the-built-but-dead.md) — it was the best parity-per-hour item in
the engine, as predicted.
