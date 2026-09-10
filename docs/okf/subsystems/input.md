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

**Two input systems exist. The better one is dead.**

`Input/InputSystem.h` + `InputAction.h` + `InputContext.h` implement most of `InputMap`: named
actions, multiple bindings per action, keyboard / mouse / gamepad-button / gamepad-axis devices,
per-binding scale for inverted axes, a gamepad deadzone of 0.15, `IsActionPressed` /
`IsActionJustPressed` / `IsActionJustReleased` / `GetAxisValue` / `GetAxis2D`, a context *stack* with
higher contexts shadowing lower ones, and `RebindAction`. Prebuilt `CreateEditorContext()` and
`CreateGameplayContext()` factories ship with it.

It has **zero callers**. Nothing instantiates it.

What actually runs is `InputManager`: pure polling, hardcoded keys, `glfwGetKey(m_Window, GLFW_KEY_W)`
and friends. It handles camera fly, orbit/pan on middle mouse, right-drag look, numpad view presets,
and an editor/gameplay context enum. Remapping is impossible. Gamepads are unsupported in the live
path.

There is no event propagation model at all — no tree walk, no consumption. Input is read wherever it
is needed: `InputManager`, `Renderer`'s GLFW callbacks, `UIManager::NewFrame` for Ctrl+Z and the gizmo
keys, and `main()` for F1 and F.

That scattering causes real collisions. `W`/`E`/`R` set the gizmo mode in `UIManager` *and* move the
camera in `InputManager` — pressing `W` to translate also flies forward.

# Delta

Parity here is mostly a **wiring** problem, not a design problem. `InputSystem` is close to
`InputMap` already; what it lacks is:

- Any caller.
- A serialized binding table (Godot's lives in Project Settings; MistEngine's is hardcoded in
  `CreateEditorContext()`).
- `get_vector` (it has `GetAxis2D`, which is the same idea).
- Event propagation and consumption, which matters much less without a `Control` UI tree.

The gizmo/camera key collision is a symptom of having no single input owner, and wiring `InputSystem`
with its context stack is exactly the mechanism that resolves it.

# Evidence

- `grep -rn "InputSystem" src tests | grep -v src/Input/InputSystem.cpp` → no results.
- `include/Input/InputContext.h` — `CreateEditorContext()` / `CreateGameplayContext()` with full
  binding tables.
- `src/Input/InputSystem.cpp` — `GetGamepadAxis` with a 0.15 deadzone; `IsActionJustPressed` over the
  context stack.
- `src/InputManager.cpp` `UpdateKeyStatesFromPolling` — hardcoded `glfwGetKey` calls.
- `src/UIManager.cpp` `NewFrame` — `ImGuiKey_W/E/R` set gizmo mode, colliding with camera WASD.

# Depends on / Blocks

Depends on nothing. Purely additive.
Implementation: [phase 4](/roadmap/phase-4-wire-the-built-but-dead.md) — the single best
parity-per-hour item in the engine.
