---
type: Subsystem Parity
title: Physics
description: Godot exposes four body types, Area triggers, shapes as child nodes and 32 collision layers; MistEngine has one rigid-body component with no layers, no triggers and no character controller.
tags: [parity, godot, physics, bullet]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: https://docs.godotengine.org/en/stable/tutorials/physics/physics_introduction.html
    title: Physics introduction — Godot docs
---

# Godot model

Four collision object types, each a node:

- **`StaticBody3D`** — fixed, collides, does not move in response
- **`RigidBody3D`** — fully simulated; you apply forces and the engine resolves motion
- **`CharacterBody3D`** — collision detection without simulation; movement is yours to write
  (`move_and_slide`)
- **`Area3D`** — detection and influence: overlap signals, and it can override gravity locally

Three design choices matter for parity:

- **Shapes are child nodes** (`CollisionShape3D`), not properties, because a body may hold any number
  of shapes — compound colliders without the parent needing to know about shape types.
- **32 collision layers**, with `collision_layer` ("what I am") separate from `collision_mask`
  ("what I scan for"). This separation is what makes filtering expressive.
- **`PhysicsMaterial`** as a shareable resource for friction and bounce.

Physics code belongs in `_physics_process()`, or `_integrate_forces()` for rigid bodies.

# MistEngine today

One `PhysicsComponent` wrapping a single `btRigidBody`, with four shape kinds
(Box / Sphere / Capsule / StaticPlane) selected by an enum, plus material parameters.

The ownership layer is the best-engineered code in the engine. `Physics/BulletOwners.h` defines
`ScopedRigidBody`, whose destructor removes the body from its world *before* freeing it, and
`PhysicsSystem`'s destructor tears down bodies, then shapes, then the world, in exactly the order
Bullet requires. Someone understood the failure mode and designed against it.

`PhysicsSystem::EnsureBody` implements an upsert keyed on a hash of the shape parameters, so editing
geometry rebuilds the body and editing material parameters does not. As of commit `b542818` mass is
applied live via `setMassProps` and only the static/dynamic transition forces a rebuild — Bullet sorts
those into different broadphase groups, so crossing zero genuinely needs the rebuild.

Lua can apply forces, impulses and velocities, set mass and kinematic flags, and raycast. Bullet
bodies carry their owning entity id in the user-index slot, so a raycast hit resolves back to the
scene.

# Delta

- **No collision layers or masks.** Everything collides with everything. No "bullets hit enemies but
  not each other", no "trigger volumes that ignore the player's own projectiles". This is the single
  most limiting gap, and Bullet already supports it through the group/mask arguments to
  `addRigidBody`.
- **No `Area3D` equivalent** — no overlap detection, no trigger volumes, no enter/exit signals. There
  is no way to build a door, a pickup, a damage zone or a checkpoint.
- **No character controller.** A player is either a full rigid body (which slides and tumbles) or
  kinematic with no collision response written for it. `move_and_slide` has no counterpart.
- **One shape per body**, so compound colliders are impossible.
- **No `PhysicsMaterial` resource** — friction and restitution are per-component scalars, not
  shareable.
- **Parented dynamic bodies are unsupported**, documented at the call site: Bullet owns the world
  transform and the hierarchy would fight it every tick.
- **Scripts cannot run on the physics tick** — see [node lifecycle](/subsystems/node-lifecycle.md).

# Evidence

- `include/ECS/Components/PhysicsComponent.h` — one `btRigidBody*`, four shape kinds, material
  scalars. No layer or mask fields.
- `grep -rin "collision_layer\|collisionmask\|GhostObject\|ContactTest" include src` → no hits.
- `include/Physics/BulletOwners.h` — `ScopedRigidBody::reset()` removes from world before freeing.
- `src/PhysicsSystem.cpp` `ComputeShapeHash` — static/dynamic flag hashed, mass magnitude applied
  live (commit `b542818`).
- `src/ECS/Systems/ECSPhysicsSystem.cpp` — comment documenting parented dynamic bodies as
  unsupported.
- `src/Script/LuaScriptLanguage.cpp` — `apply_force`, `apply_impulse`, `set_velocity`, `set_mass`,
  `set_kinematic`, `raycast`.

# Depends on / Blocks

Layers and areas depend on nothing and are cheap — Bullet supports both natively.
Blocks: essentially all gameplay logic. Not currently scheduled; the roadmap prioritises the scene
model, but layers + areas would be the highest-value unscheduled item.
