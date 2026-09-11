---
type: Roadmap Phase
title: Phase 2 — Camera as a component and frustum culling
description: Move the camera out of Renderer into the scene, then connect the frustum culling that is already written but unreachable.
tags: [roadmap, phase, camera, culling, performance]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: /subsystems/environment-and-camera.md
    title: Environment and camera
  - resource: /subsystems/rendering-pipeline.md
    title: Rendering pipeline
---

# Prerequisite

[Phase 1](/roadmap/phase-1-identity-and-environment.md) — the camera needs to serialize, which means
the scene format must already carry component data the camera can sit beside.

# Why these two together

They share a dependency: both need per-entity world bounds and a camera that something other than
`Renderer` can reach. Doing them in one pass avoids building that twice.

# Part A — camera as a component — **done**

`Camera camera;` is a private `Renderer` member (`include/Renderer.h:104`), which means one camera,
never saved, never in the scene.

- Add `CameraComponent` carrying the projection parameters — fov, near, far, and whether this is the
  active camera. Position and orientation come from `TransformComponent`, as they should.
- Keep the existing `Camera` class as the editor's *viewport* camera. The editor needs a free-fly
  camera that is not part of the scene; Godot has the same split.
- `Renderer` picks the active `CameraComponent` for the game view and falls back to the editor camera
  when none exists.
- Serialize it, so a scene stores its own framing.

This is what unblocks a second viewport, render-to-texture, cutscene cameras, and a prefab that
contains a viewpoint.

Watch for: `Renderer::kNearPlane` / `kFarPlane` were introduced at commit `ea91d7c` precisely because
those two numbers must agree across the projection matrix, the PBR shader uniforms, the CSM splits and
the cluster grid. A per-camera near/far has to thread through all four, or the cluster lookup
desynchronises from the grid it indexes — which is exactly the class of bug C2 was.

**How that was handled:** rather than threading two floats to four places, the frame resolves one
`CameraView` (`include/Renderer/CameraView.h`) at the top of `RenderWithECSAndUI` and every consumer
takes it. One place picks the camera; one place decides near/far. Two latent bugs surfaced while
doing it and were fixed: the cluster-grid cache key did not include near/far, and
`ShadowSystem::CalculateCascades` hardcoded a 1.6 aspect ratio while the projection used the real
viewport ratio.

# Part B — frustum culling — **done**

The code is written. `include/Scene/Frustum.h` has `ExtractFromVP` and `Intersects(AABB)`;
`src/Scene/SceneGraph.cpp:103` does a real cull test. `SceneGraph` is unreachable from `main()`.

- Give each renderable entity a world-space AABB. `Mesh` has vertex data, so a local AABB is cheap to
  compute once at load; `AABB::Transform(mat4)` already exists in `include/Scene/AABB.h`.
- Cache the world AABB next to `cachedGlobal` and invalidate on the same `dirty` flag.
- Cull in `RenderSystem`. All six geometry passes currently submit every entity unconditionally —
  CSM ×4 cascades, omni shadows up to 4 lights × 6 faces, depth prepass, velocity, main, skinned.
  Shadow passes cull against the light's frustum, not the camera's.
- Add a profiler counter for submitted vs culled, so the win is measurable rather than assumed.

Reuse `Scene/Frustum.h` and `Scene/AABB.h` as-is. Do not write new intersection code; the existing
implementation is correct and already has the plane-extraction maths right.

**It was correct** — but that was verified, not assumed. `tests/test_frustum.cpp` was written
*before* the code was wired into any pass, precisely because the implementation had never actually
run: its only caller was the unreachable `SceneGraph`. Plane extraction is also where a
column-major/row-major mix-up produces planes that are wrong but not obviously wrong.

Deviations from the plan above:

- Bounds live on `RenderComponent`, not beside `cachedGlobal` on `TransformComponent` — the transform
  has no idea what mesh an entity holds.
- The recompute is **unconditional**, not invalidated on the `dirty` flag. The flag was unreliable:
  the Inspector's Position field never set it. That is fixed here too, since it also meant dragging a
  parent in the Inspector left its children behind.
- `AnimatedModel` bounds are the bind pose inflated by half their extent, because GPU skinning leaves
  the CPU no cheap way to know where an animated mesh actually is.
- `Profiler` gained `SetCullStats` rather than a generic named-counter API; it has no such API, and
  adding one for two integers was not warranted.

# Files

- New: `include/ECS/Components/CameraComponent.h`
- `include/Renderer.h`, `src/Renderer.cpp` — active-camera selection; per-camera near/far
- `src/ECS/Systems/RenderSystem.cpp` — cull before submit
- `include/ECS/Components/TransformComponent.h` — cached world AABB beside `cachedGlobal`
- `src/Scene/SceneSerializer.cpp` — camera block

# How to tell it worked

- A saved scene reopens with the camera where it was left.
- Profiler shows submitted-vs-culled counts changing as the camera turns; total draw calls drop on the
  `showcase.lua` scene.
- Nothing disappears that should be visible — the failure mode of a bad cull is popping at frustum
  edges, so check against `m_ShowEditorGrid` and the pillar row.
- Tests: frustum-vs-AABB cases are pure maths and headless, so they belong in the suite.
