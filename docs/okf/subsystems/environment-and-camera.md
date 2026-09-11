---
type: Subsystem Parity
title: Environment and camera
description: Godot puts render settings in one shareable Environment resource and the camera in the scene; MistEngine now has the Environment, but the camera is still a Renderer member.
tags: [parity, godot, rendering, camera, serialization]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: https://docs.godotengine.org/en/stable/tutorials/3d/environment_and_post_processing.html
    title: Environment and post-processing — Godot docs
---

# Godot model

**One `Environment` resource** holds background/sky, ambient and reflected light, fog and volumetric
fog, tonemap operator, glow, SSR/SSAO/SSIL, SDFGI, and colour correction. Being a resource, it is
saveable, shareable between scenes, and editable in the inspector for free.

It resolves by priority: a `Camera3D`'s environment overrides a `WorldEnvironment` node (one per
scene tree), which overrides the editor's preview environment.

Exposure and depth of field were deliberately **split out** into a separate `CameraAttributes`
resource in Godot 4, with `CameraAttributesPractical` (arbitrary units) and
`CameraAttributesPhysical` (millimetres, EV100) variants, plus auto-exposure.

And the camera is a **node**: `Camera3D` sits in the tree, so a scene stores its own viewpoint, a
scene can hold several, and render-to-texture is just another viewport.

# MistEngine today

## Environment — landed

`struct Environment` (`include/Environment.h`) is a single reflected,
serialized object owning every render tunable: tonemap operator and exposure,
ambient fallback, the bloom / SSAO / SSR / SSGI / TAA / FXAA / DOF /
motion-blur enables and their parameters, shadow softness and quality, the
atmospheric sky parameters, and the `usePBR` / `showEditorGrid` /
`showPhysicsDebug` toggles. 34 reflected fields, one owner.

The nine objects that used to carry these as public fields —
`PostProcessStack`, `Renderer`, `SkyboxRenderer`, `SSRRenderer`,
`BloomRenderer`, `SSAORenderer`, `SSGIRenderer`, `ShadowSystem`,
`TAARenderer` — no longer carry any of them. `PostProcessStack::Execute` takes
a `const Environment&`, and each sub-renderer's `Render` takes it too, so the
settings are read where they are used rather than mirrored. There is one copy
of each value in the process.

Consequences:

- **A scene stores how it looks.** `SceneSerializer` writes an `environment`
  block and reads it back. The `Environment*` parameter is nullable, so a
  headless caller round-trips entities without one, and a scene file written
  before this feature leaves the caller's settings untouched instead of
  snapping them to defaults.
- **The View menu collapsed.** `DrawPostProcessControls`, `DrawShadowControls`
  and `DrawSkyboxControls` — roughly 150 lines of hand-written ImGui
  duplicating field names already declared in `Environment.h` — became one
  `DrawReflectedProperties(&env, props)` call. A new tunable now costs one
  `MIST_FIELD` line and appears in the Inspector and the scene file with no
  further edits.
- **The sun still tracks the scene.** `Renderer::RenderWithECSAndUI` continues
  to derive the sun from the first directional `LightComponent`, but now
  writes it into `Environment::sunDirection`, which is also what gets
  serialized. A scene with a directional light stores the sun that light
  implies; a scene without one keeps the authored fallback, where previously
  it rendered against a hardcoded vector.

Not modelled, deliberately: Godot's `CameraAttributes` split (exposure and DOF
as a separate resource, with auto-exposure), the
Camera3D > WorldEnvironment > editor-preview priority chain, and fog. Sky
*mode* also stays off the Environment — it selects which shader
`SkyboxRenderer` runs, which is implementation rather than scene description.

One known ergonomic gap: the Inspector has no property groups or categories,
so 34 fields render as one flat list. Godot's `@export_group` is the fix and is
not scheduled.

## Camera — still a Renderer member

`Camera camera;` remains a private `Renderer` data member
(`include/Renderer.h`). So: one camera, ever; not in the scene; not saved; no
second viewpoint; no render-to-texture. There is no `CameraComponent`.

`Renderer::kNearPlane` / `kFarPlane` are still shared constants, because the
projection matrix, the PBR shader uniforms, the CSM cascade splits and the
cluster grid all have to agree on them.

# Evidence

- `include/Environment.h` — the struct and its 34-field `MIST_REFLECT` block.
- `include/Renderer.h` — `Environment m_Environment` and `GetEnvironment()`; `m_Exposure`,
  `m_UsePBR`, `m_ShowEditorGrid` and `m_ShowPhysicsDebug` are gone.
- `include/PostProcessStack.h` — `Execute(const Environment&, ...)`; no tunable fields remain.
- `src/Scene/SceneSerializer.cpp` — the `environment` block in both Save and Load.
- `src/UIManager.cpp` `DrawEnvironmentPanel` — one reflected panel; the three it replaced are gone.
- `tests/test_scene_serializer.cpp` — Environment round-trip including the `TonemapOperator` enum,
  a field-coverage guard, and the absent-block case.
- `include/Renderer.h` — `Camera camera;` is still a private member; no `CameraComponent` exists.
- `src/Renderer.cpp` — the directional-light scan that derives the sun.
- `ls include/ECS/Components/` — no `CameraComponent.h`.

# Depends on / Blocks

Environment depends on nothing; it is a reflected struct plus serializer wiring.
Camera-as-component blocks [prefabs](/subsystems/scene-and-composition.md) containing viewpoints and
any multi-viewport work.
Implementation: [phase 1](/roadmap/phase-1-identity-and-environment.md) for Environment,
[phase 2](/roadmap/phase-2-camera-and-culling.md) for the camera.
