# MistEngine Scripting Manual

> Written against MistEngine 0.5.0-prealpha. The binding surface is
> small by design — this cycle covers 10 functions. Expect more in
> every release; the roadmap at the end flags what's deliberately
> missing so you know what to work around today.

## Contents

1. [Overview](#overview)
2. [Attaching a script](#attaching-a-script)
3. [Lifecycle callbacks](#lifecycle-callbacks)
4. [Bindings reference](#bindings-reference)
5. [Examples](#examples)
6. [Error handling](#error-handling)
7. [Sandbox model](#sandbox-model)
8. [Paths and security](#paths-and-security)
9. [Roadmap](#roadmap)

---

## Overview

MistEngine scripts are written in **Lua 5.4**, embedded via
[sol2](https://github.com/ThePhD/sol2). The scripting subsystem is
gated behind the CMake option `MIST_ENABLE_SCRIPTING` (ON by default);
with it off the engine compiles as a headless PBR renderer with no
scripting overhead.

All scripting entry points live under `Mist::Script` in
`include/Script/` and `src/Script/`. The registry pattern
(`Mist::Script::ScriptRegistry`) lets additional languages slot in
later (JavaScript via QuickJS, a Mist-native DSL, etc.) without
changing every call site that compiles or runs a script.

---

## Attaching a script

There are **three** ways to run Lua code inside the engine. Pick based
on whether you want a per-entity lifecycle or just one-shot execution.

### 1. `bootstrap.lua` — runs once at engine startup

`scripts/bootstrap.lua` executes during engine initialization, *before*
the editor presents its first frame. This is where the default scene
lives: ground plane, cubes, any gameplay entities the editor should
open with. You do not need to register it anywhere — the engine looks
for this exact path.

### 2. `attach_script(entity_id, path)` — per-entity lifecycle

Compiles a Lua file and binds it to an entity as a `ScriptComponent`.
The engine drives that script's `_ready` and `_process` callbacks
every frame — think of it as the Mist equivalent of Godot's
`GDScript.attach()` or Unity's `MonoBehaviour`.

```lua
local e = spawn_cube(0, 1, 0)
attach_script(e, 'res://scripts/spinner.lua')
```

### 3. `run_script(path)` — one-shot execution

Compiles + runs a `.lua` file's top-level body **once**, then drops
it. The script does not get a `ScriptComponent` and does not receive
`_process` ticks. Use this when you want a bootstrap-style helper to
spawn things into the world without lingering afterwards.

```lua
run_script('res://scripts/seed_enemies.lua')
```

---

## Lifecycle callbacks

A script attached via `attach_script` can define these optional
functions. The engine calls them if they exist and silently ignores
them if they don't — missing `_ready` is not an error.

| Callback | Called | By |
|----------|--------|----|
| `_ready()` | Once, after the script's owning entity is fully in the hierarchy | `HierarchySystem` post-order ready walk |
| `_process()` | Every frame | `ScriptSystem::OnUpdate` |

### The ready ordering guarantee

`_ready` fires **post-order** — a child's `_ready` runs before its
parent's. This matches Godot and is the right default: when a parent
wakes up, all its children are already alive and set up. So if a
cube script needs to read its parent's transform in `_ready`, it can.

### Per-frame delta time

`_process()` takes no arguments. Query `get_delta_time()` inside it to
get the frame delta as a float (seconds). The script engine stores
delta in a thread-local so the getter is safe to call from any
binding the engine registers later.

```lua
function _process()
    local dt = get_delta_time()
    -- ... do things scaled by dt
end
```

---

## Bindings reference

All bindings are registered in `LuaScriptLanguage::Init`
(`src/Script/LuaScriptLanguage.cpp`). They form a single flat
namespace — no `mist.` or `engine.` prefix required.

### I/O

#### `print(...)`
Overridden to route Lua output into the engine logger. Prints every
argument joined by tabs, prefixed with `[lua] `, at `LOG_INFO` level.
Lands in the editor's Console dock.

```lua
print("hello", 42, true)
-- → [INFO] [lua] hello  42  true
```

### Entity context

#### `entity_id() → int`
Returns the integer ID of the entity currently being processed. Meaningful
only from inside a `ScriptComponent`'s callbacks (`_ready` / `_process`);
returns `-1` otherwise.

```lua
function _ready()
    print("attached to entity " .. entity_id())
end
```

#### `get_delta_time() → float`
Returns seconds since the previous frame. Set by `ScriptSystem` before
each `_process` tick.

### Transforms

#### `get_transform() → table | nil`
Returns a flat table of the current entity's `TransformComponent`,
or `nil` if there's no such component (or no current entity). Keys:

| Key | Meaning |
|-----|---------|
| `x`, `y`, `z` | Position |
| `rx`, `ry`, `rz` | Euler rotation (degrees) |
| `sx`, `sy`, `sz` | Scale |

#### `set_transform(tbl)`
Writes a flat table back to the current entity's transform. Any key
you omit keeps its current value (it's a partial update). Marks the
transform dirty so `HierarchySystem` rebuilds cached globals next
frame.

```lua
function _process()
    local t = get_transform()
    if not t then return end
    t.ry = t.ry + 45.0 * get_delta_time()
    set_transform(t)
end
```

### ECS / world

#### `spawn_cube(x, y, z) → int`
Creates an entity with `TransformComponent`, `RenderComponent` (bound
to the shared `builtin://cube` mesh), and `HierarchyComponent`.
Returns the new entity ID, or `-1` if the builtin mesh failed to
load. All cubes share **one** `Mesh` — spawning N cubes allocates one
mesh, not N.

#### `spawn_plane(x, y, z, sx?, sy?, sz?) → int`
Same as `spawn_cube` but with the `builtin://plane` mesh and optional
scale. Scale defaults (`20`, `1`, `20`) match the old hardcoded ground
plate — `spawn_plane(0, 0, 0)` gives you the engine's default ground.

#### `destroy_entity(id)`
Removes the entity from every system. Safe to call on an already-dead
ID; safe to call on `-1` (logs a warning and returns cleanly).

### Script management

#### `run_script(path) → bool`
Compiles and runs a `.lua` file's top-level once. Returns `true` on
success, `false` on compile error or missing file. Path must be
`res://`-prefixed and stay inside the project sandbox (see
[Paths and security](#paths-and-security)).

#### `attach_script(id, path) → bool`
Compiles a `.lua` file, wraps it in a `ScriptComponent`, and adds it
to the given entity. Returns `true` on success. From this point the
engine calls `_ready` on the next hierarchy ready walk and `_process`
every frame.

---

## Examples

### `bootstrap.lua` — default scene

```lua
print("bootstrap: assembling default scene")

-- Ground plane, slightly below y=0 so nothing z-fights with it.
spawn_plane(0, -0.01, 0, 20, 1, 20)

-- Delegates to the orbit demo, which spawns the ring of cubes.
run_script('res://scripts/orbits.lua')
```

Runs once at engine startup. Creates the ground and hands off to
`orbits.lua` for the visible demo. This is the canonical "hello world"
for spawn_plane + run_script.

### `orbits.lua` — spawning with scripts attached

```lua
local NUM_CUBES     = 5
local ORBIT_RADIUS  = 3.0
local ORBIT_HEIGHT  = 1.0

for i = 1, NUM_CUBES do
    local theta = ((i - 1) / NUM_CUBES) * (2 * math.pi)
    local x = math.cos(theta) * ORBIT_RADIUS
    local z = math.sin(theta) * ORBIT_RADIUS

    local e = spawn_cube(x, ORBIT_HEIGHT, z)
    if e >= 0 then
        attach_script(e, 'res://scripts/spinner.lua')
    end
end
```

Places five cubes in a ring and attaches `spinner.lua` to each. This
is the pattern to follow when you want "spawn N things and give each
a script."

### `spinner.lua` — a lifecycle script

```lua
local spin_speed = 45.0  -- degrees / second

function _ready()
    print("spinner attached to entity " .. entity_id())
end

function _process()
    local t = get_transform()
    if not t then return end

    local dt = get_delta_time()
    t.ry = t.ry + spin_speed * dt
    set_transform(t)
end
```

Rotates its entity around the Y axis at a constant rate. Showcases:
the two lifecycle callbacks, `entity_id` in `_ready`, `get_transform`
/ `set_transform` roundtrip, and frame-delta-scaled motion.

### Click-to-destroy pattern

Here's a fresh example you can drop into `scripts/` and
`attach_script` to any entity — destroys itself on its first
`_process`. Exercise `destroy_entity`:

```lua
-- self_destruct.lua
function _ready()
    print("entity " .. entity_id() .. " will self-destruct next frame")
end

function _process()
    destroy_entity(entity_id())
end
```

---

## Error handling

### Compile errors

`attach_script` / `run_script` parse the Lua source through sol2's
`safe_script`. On a syntax error:

1. The error is logged at `LOG_ERROR` level with the Lua message and
   the path (e.g. `"Lua compile error: ...unterminated string..."`).
2. The script does **not** attach. `attach_script` returns `false`,
   `run_script` returns `false`.
3. Any partially-constructed entity you spawned before the
   `attach_script` call stays in the scene. Clean that up yourself if
   you care.

### Runtime errors in `_ready` / `_process`

`LuaScriptInstance::CallVoid` uses `sol::protected_function`, so a
Lua runtime error (nil deref, bad arithmetic, etc.) is **caught** —
the rest of the engine keeps running. The error is logged as

```
[ERROR] Lua '_process' runtime error: <message>
```

A throwing `_ready` leaves the entity in a state where `_ready` did
not complete — subsequent `_process` calls still fire. If that's not
what you want, defensively track readiness yourself.

### Missing callbacks

`CallVoid` checks `fn.valid()` before calling. A script without a
`_ready` or `_process` simply doesn't tick those; no error, no log
line.

---

## Sandbox model

The scripting subsystem uses **one `sol::state`** per
`LuaScriptLanguage` instance, but **one `sol::environment`** per
compiled script. This gives:

- **Shared engine surface**: bindings (`print`, `spawn_cube`, etc.)
  are registered on the `sol::state`'s globals. Every environment
  falls back to those globals, so every script sees the same engine
  API.
- **Isolated user state**: globals each script *declares* (e.g.
  `my_local_counter = 0`) live in the environment, not the shared
  state. Two cubes running the same `spinner.lua` have independent
  `spin_speed` values if they set them.

This is why `tests/test_lua_script.cpp` can assert that a global set
in script A isn't readable from script B.

### Why not one state per script?

Per-script states would balloon memory and break binding registration —
we'd have to re-register all 10 bindings on every compile. The
per-environment model is sol2's idiomatic approach and keeps the cost
of a new script near zero.

---

## Paths and security

Script paths **must** use the `res://` prefix. `res://` is MistEngine's
convention for "project-root-relative" — it mirrors Godot's and lets
the engine sandbox file access.

`Mist::PathGuard::resolve_res_path(path)` turns `res://scripts/foo.lua`
into an absolute path under the project root. If the normalized path
escapes the root (e.g. via `..`), `resolve_res_path` returns an empty
string, and the calling binding (`run_script` / `attach_script`) logs

```
[ERROR] attach_script: path escapes sandbox: <input>
```

and refuses the call. You cannot script-read `/etc/passwd`, `/proc/`,
or anything else outside the project.

### Why this matters

Scripts ship inside projects. A malicious project downloaded from the
internet must not be able to read your home directory on first launch.
`PathGuard` is the single chokepoint for that — if you add a new
binding that reads files, route the path through `PathGuard` before
opening.

---

## Roadmap

What's deliberately missing, in rough priority order:

- **Physics bindings** — `apply_force`, `set_velocity`,
  `raycast`. Blocked on deciding whether rigid bodies expose a
  Bullet-ish impulse API or a Godot-ish character-controller
  abstraction. Probably both, with the former first.
- **Hot reload** — editing a `.lua` file while the engine is running
  should rebuild the corresponding `LuaScriptInstance` without a
  restart. Needs a file watcher + the ability to swap
  `ScriptComponent::instance` mid-frame safely.
- **Camera + raycast API** — `get_camera()`,
  `screen_to_world(x, y)`. Needed for any click-to-select,
  pick-to-drag, or first-person aim script.
- **Module system** — `require 'mymod'` for sharing code across
  scripts. Today every script is its own island. This pairs with a
  `res://lua/?.lua` path setup and clear rules about which scripts
  can call which.
- **Coroutine-based timers** — `wait(0.5)` inside a `_process`.
  Lua coroutines make this natural but we don't currently yield the
  script thread through `ScriptSystem`.
- **Event bus bindings** — letting Lua subscribe to `Mist::Signal<T>`
  events (e.g. `on_collision`, `on_death`). The C++ side already has
  the signal primitive; wiring it to sol2 needs a type-erased
  adapter.
- **`sol::usertype<glm::vec3>`** — the transform API uses a flat
  table for now. Upgrading to a proper vec3 userdata gives better
  ergonomics (`t.position + glm.vec3(0,1,0)`) and keeps the binding
  surface stable as we add more components.
- **Inspector binding** — exposing script-declared `export` variables
  to the entity inspector so non-programmers can tweak a
  `spin_speed` via the UI without editing the file.

If you want one of these sooner than the roadmap suggests, open an
issue or a PR. The scripting subsystem is small enough that adding a
binding usually means one function in `LuaScriptLanguage.cpp` and one
test case in `tests/test_lua_script.cpp`.
