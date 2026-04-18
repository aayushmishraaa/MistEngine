# MistEngine Editor — Controls Reference

This document lists every keyboard and mouse binding available in the
MistEngine editor viewport. If something here doesn't match what the
editor actually does, that's a bug — please open an issue.

All bindings live in `src/InputManager.cpp`. The editor starts in
**Scene Editor mode** with the cursor unlocked so you can interact with
ImGui panels normally. Camera control keys are polled every frame —
they do not require focus on the Scene View panel.

---

## Camera movement (keyboard)

| Key | Action |
|-----|--------|
| **W** | Move camera forward (along view direction) |
| **S** | Move camera backward |
| **A** | Strafe left |
| **D** | Strafe right |
| **Q** | Move camera down (world −Y) |
| **E** | Move camera up (world +Y) |

Movement speed is controlled by `Camera::MovementSpeed` and scales
with frame delta time — behaviour is frame-rate independent.

---

## Camera look (mouse)

| Button | Action |
|--------|--------|
| **Right-click + drag** | FPS-style mouse look (yaw + pitch) |
| **Middle-click + drag** | Orbit around the focal point |
| **Middle-click + Shift + drag** | Pan (translate focal point) |
| **Scroll wheel** | Zoom toward focal point |

Mouse look respects ImGui focus — if the cursor is over a panel that
wants input (`io.WantCaptureMouse`), the camera is not dragged. This
is why right-clicking on a property sheet doesn't accidentally swivel
the view.

---

## Numpad view presets (Godot convention)

| Key | Action |
|-----|--------|
| **Numpad 1** | Front view (camera looking down −Z) |
| **Numpad 3** | Right view (camera looking down −X) |
| **Numpad 7** | Top view (camera looking straight down) |
| **Numpad 5** | Toggle orbit / free camera mode |
| **Numpad 0** | Reset to the default editor vantage (yaw −90°, pitch −20°, focal point at origin, orbit distance 10) |

Presets are edge-triggered: hold the key and the view only snaps once.
Each preset resets the Euler angles and calls
`Camera::updateCameraVectors()` so `Front` / `Right` / `Up` are
recomputed consistently — matches Godot's numpad behaviour.

---

## Mode toggles

| Key | Action |
|-----|--------|
| **F3** | Toggle Scene Editor ↔ Gameplay mode |

Gameplay mode locks the cursor into the window and swaps camera
bindings to immersive (always-on mouse look). Scene Editor mode is the
default; press F3 to return.

---

## Notes on input architecture

- Input is **pure polling** via `glfwGetKey` / `glfwGetMouseButton`, not
  GLFW callbacks. This lets ImGui's own callbacks run untouched —
  earlier iterations fought ImGui for callback ownership and would
  eat text-field input.
- `InputManager::Update(deltaTime)` is called once per frame from
  `MistEngine.cpp`. It polls, diffs against `m_PrevKeyStates` for
  edge-triggered actions (numpad presets, F3), and dispatches.
- All camera logic lives in `Camera.cpp`. `InputManager` is a thin
  translator — if you want to change the feel of movement, touch
  `Camera::ProcessKeyboard` / `ProcessMouseMovement`.

---

## Adding a new binding

1. **Poll the key** inside `UpdateKeyStatesFromPolling` so
   `m_KeyStates[GLFW_KEY_...]` is current for that frame.
2. **Handle it** inside `HandleKeyboardPolling`. Use
   `m_KeyStates[k] && !m_PrevKeyStates[k]` for edge-triggered
   (one-shot) actions; use plain `m_KeyStates[k]` for held-down
   movement.
3. **Document it here.** Drift between code and this file is what
   produced the confusion that prompted this document in the first
   place.
