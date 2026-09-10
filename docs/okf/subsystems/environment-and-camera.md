---
type: Subsystem Parity
title: Environment and camera
description: Godot puts render settings in one shareable Environment resource and the camera in the scene; MistEngine scatters 41 tunables across 9 objects and none of them are saved.
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

Render settings are spread across **nine objects, 41 public tunables, none serialized**:

| Object | Tunables |
|---|---|
| `PostProcessStack` | 15 |
| `Renderer` | 6 — `lightDir`, `lightColor`, `m_Exposure`, `m_UsePBR`, `m_ShowEditorGrid`, `m_ShowPhysicsDebug` |
| `SkyboxRenderer` | 4 — sun direction, Rayleigh, Mie, turbidity |
| `SSRRenderer` | 4 |
| `BloomRenderer` | 3 |
| `SSAORenderer` | 3 |
| `SSGIRenderer` | 3 |
| `ShadowSystem` | 2 |
| `TAARenderer` | 1 |

Every one is reachable from the View menu and every one resets to its compile-time default on
restart. A lighting look cannot be saved, shared, or version-controlled.

There is a partial substitute for ambient/sky coupling: `Renderer::RenderWithECSAndUI` scans the ECS
for the first directional `LightComponent` and adopts its direction and colour as the sun, which also
drives the skybox. That is a real piece of Godot-like behaviour, built by hand for one property.

The camera is `Camera camera;` — a `Renderer` data member. So: one camera, ever; not in the scene;
not saved; no second viewpoint; no render-to-texture. There is no `CameraComponent`.

# Delta

- **No serializable environment**, so a scene is not a complete description of how it looks. Reopen
  it and you get the defaults.
- **Settings have no owner.** Adding a post-process knob means adding a public field to whichever
  renderer happens to hold the pass, then a View-menu widget. There is no single place that means
  "how this scene renders".
- **Camera cannot be authored.** Framing is lost on exit. A cutscene camera, a minimap, a reflection
  probe and a second editor viewport are all blocked by the same fact.
- **No exposure/DOF split**, and DOF's focus distance is a raw number with no auto-exposure.

The fix has unusually good leverage: `PBRMaterial` already proves the pattern — one reflected struct
that the inspector renders and the serializer round-trips for free. An `Environment` struct with
`MIST_REFLECT` would collapse all 41 tunables into one serializable object and make the inspector
panel fall out automatically.

# Evidence

- Tunable counts: `awk '/^public:/,/^private:/' include/<X>.h | grep -cE "^\s+(bool|float|int) "`.
- `include/Renderer.h:104` — `Camera camera;` as a private `Renderer` member.
- `include/Renderer.h:114-115,145-148` — `lightDir`, `lightColor`, `m_Exposure`, `m_UsePBR`,
  `m_ShowEditorGrid`, `m_ShowPhysicsDebug`.
- `src/Scene/SceneSerializer.cpp` Save — writes transform/render/physics/light/hierarchy/animation.
  No environment, no camera.
- `src/Renderer.cpp` — the directional-light scan that derives the sun.
- `ls include/ECS/Components/` — no `CameraComponent.h`.

# Depends on / Blocks

Environment depends on nothing; it is a reflected struct plus serializer wiring.
Camera-as-component blocks [prefabs](/subsystems/scene-and-composition.md) containing viewpoints and
any multi-viewport work.
Implementation: [phase 1](/roadmap/phase-1-identity-and-environment.md) for Environment,
[phase 2](/roadmap/phase-2-camera-and-culling.md) for the camera.
