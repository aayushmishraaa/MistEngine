---
type: Subsystem Parity
title: Scripting
description: The most complete subsystem in MistEngine — a sandboxed Lua backend behind a language-agnostic interface, genuinely comparable to Godot's ScriptLanguage abstraction.
tags: [parity, godot, scripting, lua]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: https://docs.godotengine.org/en/stable/tutorials/scripting/resources.html
    title: Resources — Godot docs
  - resource: /subsystems/node-lifecycle.md
    title: Node lifecycle
---

# Godot model

Multiple languages behind one abstraction: GDScript (first-party, tightly integrated), C# (via .NET),
and GDExtension for native languages. A `Script` is a `Resource`, so it is path-cached, refcounted and
serializable like any other asset.

Scripts attach to nodes and receive the full [lifecycle](/subsystems/node-lifecycle.md). They declare
inspector-visible properties with `@export` and its variants, which map onto the engine's
`PropertyHint` system — `@export_range`, `@export_enum`, `@export_flags`, `@export_file`,
`@export_multiline`, `@export_group`, and `@export_custom` for direct hint control. Scripts can
define signals, and the editor can connect them visually.

# MistEngine today

This is the subsystem that is genuinely close to parity, and the only headline feature that was fully
wired before this year's fix pass.

`Script/ScriptLanguage.h` defines `IScriptLanguage` (name, extension, `Compile`, `Init`, `Shutdown`)
and `IScriptInstance` (`CallVoid`, `GetString`, `SetString`) — language-agnostic by design, with
`ScriptRegistry` dispatching on file extension. That is the same shape as Godot's `ScriptLanguage`
registry.

The Lua backend is careful work:

- **sol2 hidden behind PIMPLs** so its heavy templates do not bleed into every translation unit.
- **A per-script `sol::environment`**, falling back to globals, so two scripts cannot trample each
  other's top-level declarations. This mirrors Godot's per-script scope.
- **Thread-local current-entity and delta-time bridges**, set around each callback with exception
  safety so the TLS never leaks past a throwing callback.
- **Protected calls** — every `CallVoid` goes through `sol::protected_function` and logs errors
  rather than unwinding into the engine.
- **`PathGuard` on every script path**, so `run_script` and `attach_script` cannot escape the project
  root.

Bindings cover spawn/destroy, transforms, lighting (including Kelvin and photometric units), physics
forces/velocity/mass/kinematic, animation playback, and raycast. The editor console doubles as a REPL
by routing unrecognised commands to `Compile`.

# Delta

- **Two lifecycle callbacks, not nine** — `_ready` and `_process` only. See
  [node lifecycle](/subsystems/node-lifecycle.md).
- **No `@export` equivalent.** A script cannot declare inspector-visible properties; the Inspector
  shows only the script path and whether an instance loaded. The reflection system that would back
  this already exists and drives component inspectors — the missing piece is letting a script
  register properties into it.
- **No script-defined signals.** `Mist::Signal<T>` is solid but scripts cannot declare or connect to
  one, and engine-wide exactly one signal instance exists (`HierarchySystem::OnReady`).
- **Scripts are not resources** — no caching by path, no hot reload. Editing a `.lua` requires a
  restart.
- **No entity addressing.** `entity_id()` returns self; there is no `get_node("../Door")`. Blocked by
  [entity identity](/subsystems/entity-identity.md).
- **Sandbox caveat:** `sol::lib::base` exposes `load`, `loadfile` and `dofile`, which read outside
  `PathGuard`'s reach. Low severity while the only script sources are project files and the user's own
  console input, but scene and package files carry script paths, so it is an untrusted-input surface.
- **`_ready` can be missed.** It fires only through `HierarchySystem::OnReady`, which fires once per
  entity; a script attached after that point never receives it.

# Evidence

- `include/Script/ScriptLanguage.h` — the language-agnostic interface pair.
- `src/Script/LuaScriptLanguage.cpp` — PIMPLs, per-instance `sol::environment`, thread-local bridges,
  `sol::protected_function`, `PathGuard::resolve_res_path` on script paths.
- `src/Script/LuaScriptLanguage.cpp` `Init` — `open_libraries(base, string, math, table)`; `base`
  carries `load`/`loadfile`/`dofile`.
- `src/ECS/Systems/ScriptSystem.cpp` — `_ready` via signal, `_process` per frame, nothing else.
- `grep -rn "Mist::Signal<" src include | grep -v Core/Signal.h` → `HierarchySystem` only.

# Depends on / Blocks

Property export depends on the reflection system, which already exists.
Entity addressing depends on [entity identity](/subsystems/entity-identity.md)
([phase 1](/roadmap/phase-1-identity-and-environment.md)).
