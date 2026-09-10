---
type: Subsystem Parity
title: Rendering pipeline
description: MistEngine's lighting and post-process stack is Forward+-class and ahead of Godot's Compatibility renderer, but it has none of Godot's scene-level rendering features and does no culling.
tags: [parity, godot, rendering, performance]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: https://docs.godotengine.org/en/stable/tutorials/rendering/renderers.html
    title: Renderers — Godot docs
  - resource: https://docs.godotengine.org/en/stable/tutorials/performance/using_servers.html
    title: Optimization using Servers — Godot docs
---

# Godot model

Three renderers, selected per project:

| | Compatibility | Mobile | Forward+ |
|---|---|---|---|
| API | OpenGL | Vulkan / D3D12 / Metal | Vulkan / D3D12 / Metal |
| Lights | 8 per mesh | 8 per mesh, 256 per view | clustered, 512 per cluster |
| PCSS | no | omni + spot only | all light types |
| Compute shaders | no | penalised | yes |
| VoxelGI / SDFGI / SSIL | no | no | yes |
| SSR / volumetric fog | no | no | yes |
| SSAO | yes | no | yes |
| TAA / FSR2 | no | no | yes |
| Colour / depth | RGBA8 / 24-bit | RGB10A2 / 24-bit reverse-Z | RGBA16F / 32-bit reverse-Z |

Behind all three sits the **server architecture**: `RenderingServer` owns GPU resources, addressed by
opaque `RID` handles that are allocated and freed manually. Nodes are thin wrappers holding RIDs —
`VisualInstance3D.get_instance()`, `CanvasItem.get_canvas_item()`. The scene system is explicitly
*optional*: "while it is not possible to compile it out, it can be completely bypassed." Servers run
asynchronously, and any call returning a value stalls them.

# MistEngine today

A 15-pass forward frame at GL 4.6. On the lighting and post axes it sits **above Compatibility**:

- Clustered lighting, 16×9×24 grid, 1024-light SSBO, compute-shader culling
- CSM, 4 cascades at 2048², with PCSS and a Vogel-disk blocker search
- Omni shadows in a cubemap array, 4 shadow-casting point lights
- Depth prepass writing octahedral normals + roughness; Hi-Z pyramid
- TAA (Halton jitter, variance clipping), SSR over Hi-Z, bloom, SSAO, bokeh DOF, motion blur, FXAA
- AgX / ACES / Reinhard tonemapping into an LDR present target

Much of that was only actually *reaching the screen* after commit `ea91d7c`: the cluster grid was
never dispatched, SSAO multiplied ambient by an unbound sampler, and TAA's jitter never reached the
geometry passes.

The `RenderingDevice` / `RID` abstraction exists and mirrors Godot's shape — but every consumer
`static_cast`s the abstract pointer to `GLRenderingDevice` to reach `GetGLHandle()`, so it is not yet
a seam a second backend could use.

# Delta

**No culling at all.** This is the most consequential gap. All six geometry-submitting passes iterate
every entity unconditionally — CSM ×4 cascades, omni shadows up to 4 lights × 6 faces, depth prepass,
velocity, main, skinned. A frustum implementation with plane extraction and AABB intersection
*exists* and is correct, in `Scene/Frustum.h`, used only by `SceneGraph` — which is itself
unreachable from `main()`. So the culling code is written and simply not connected.

Missing scene-level rendering features, all of which Godot exposes as nodes or resources:

- Global illumination in any form (VoxelGI, SDFGI, SSIL, lightmaps)
- Reflection probes, decals, volumetric fog
- LOD / visibility ranges, occlusion culling
- MSAA
- `ShaderMaterial` — there is exactly one fixed `PBRMaterial`, so a custom shader per object is
  impossible
- An [Environment resource](/subsystems/environment-and-camera.md)

Also absent: the asynchronous server model. Everything is synchronous on the main thread.
`Core/CommandQueue.h` exists and is shaped for a render thread — a mutex-guarded push/flush queue whose
header documents the intent — but it is referenced only by `tests/test_command_queue.cpp`. Nothing in
the engine pushes to it.

# Evidence

- `grep -c "renderSystem->Update" src/Renderer.cpp` → 6 geometry-submitting passes.
- `include/Scene/Frustum.h:20,60` — `ExtractFromVP` and `Intersects(AABB)`.
- `src/Scene/SceneGraph.cpp:103` — the only real cull test in the tree; `SceneGraph` is not
  referenced by `MistEngine.cpp`, `Renderer.cpp` or `UIManager.cpp`.
- `src/Shader.cpp` — `static_cast<GLRenderingDevice*>(Mist::GPU::Device())`, the cast that defeats
  the backend abstraction.
- `grep -rin "lod\|occlusion\|reflectionprobe\|decal" include src` → no engine hits.
- `grep -rln CommandQueue src include tests` → the header and `tests/test_command_queue.cpp` only;
  no engine call site.

# Depends on / Blocks

Culling depends on per-entity world bounds, which is also what prefab instancing will want.
Implementation: [phase 2](/roadmap/phase-2-camera-and-culling.md).
