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

# Part A — camera as a component

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

# Part B — frustum culling

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
