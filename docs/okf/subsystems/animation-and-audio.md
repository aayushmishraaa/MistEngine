---
type: Subsystem Parity
title: Animation and audio
description: Godot has an animation graph and a bussed audio server; MistEngine imports rigs and renders a static bind pose, and its audio stack is unreachable.
tags: [parity, godot, animation, audio, skinning]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: https://docs.godotengine.org/en/stable/classes/class_node.html
    title: Node — Godot class reference
  - resource: /subsystems/node-lifecycle.md
    title: Node lifecycle
---

# Godot model

**Animation**: `AnimationPlayer` plays `Animation` resources that can key *any* property, not just
bones. `AnimationTree` layers a state machine and blend trees over that, with blend spaces and
transitions. `Skeleton3D` drives skinning, with IK and blend shapes. Animations are resources, so they
are shareable and serializable.

**Audio**: `AudioServer` with a bus graph — named buses, per-bus effects, sends. `AudioStreamPlayer`,
`AudioStreamPlayer2D` and `AudioStreamPlayer3D` position sound in the tree; `AudioStream` is a
resource.

# MistEngine today

**Animation: imports, then renders a bind pose.**

The import half works. `SceneImporter` detects `HasBones()` and routes the file to `AnimatedModel`,
which parses the skeleton and every `aiAnimation` into engine `Animation` objects, populates
`AnimationComponent::availableClips`, and auto-plays the first clip. The Inspector shows a clip
dropdown with Play/Pause/Stop, speed and loop. `Animator` interpolates position/rotation/scale
keyframes, and `BlendTo` cross-fades.

None of it reaches the screen, for three independent reasons — tracked as `TODO(C5)` in
`RenderSystem::UpdateSkinned`:

1. `AnimationComponent::animator` is default-constructed and `Init()` is never called on it, so its
   bone SSBO handle is 0. Until commit `b542818` this also raised a GL error every frame per animated
   entity and unbound the slot the skinned shader reads.
2. `Animator::calculateBoneTransforms` writes each bone's **local** transform and never walks the
   skeleton hierarchy, so parent bones do not compose.
3. `BoneInfo::offset` — the inverse bind matrix — is stored during import and read nowhere, and
   `AnimatedModel::calculateBoneTransformsHierarchy`, the function that would do the walk, is
   **declared in the header with no definition anywhere**.

So the clock advances, the Inspector looks correct, and the mesh stands still in its bind pose.

**Audio: unreachable.** `AudioEngine`, `AudioClip` and `AudioSourceComponent` exist.
`src/ECS/Systems/AudioSystem.cpp` defines an `AudioSystem` class with **no header and no callers** —
it compiles to nothing referenced. `MIST_ENABLE_AUDIO` defaults to `OFF` and miniaudio is not
vendored. There is no sound in the engine.

# Delta

Animation needs the hierarchy walk written and the inverse bind matrix applied — bounded, well-understood
work, but it is new implementation rather than a fix, and there is no rigged asset in `models/` to
validate against. That is why it was explicitly deferred from the fix pass rather than attempted.

Beyond getting skinning to render at all, the gap to Godot is large: no animation graph or state
machine, no blending beyond a single cross-fade, no IK, no blend shapes, no animating arbitrary
properties (only bone transforms), and animations are not resources.

Audio needs the whole stack connected: a miniaudio dependency, an `AudioSystem` with a header that
something calls, and 3D positioning fed from transforms. No bus graph exists or is designed.

# Evidence

- `src/ECS/Systems/RenderSystem.cpp` — the `TODO(C5)` enumerating all three skinning breaks.
- `src/AnimatedModel.cpp` — `BoneInfo::offset` assigned from `mOffsetMatrix`; never read.
- `grep -n "calculateBoneTransformsHierarchy" src/AnimatedModel.cpp` → no definition;
  `include/AnimatedModel.h:91` declares it.
- `src/Animator.cpp` `calculateBoneTransforms` — writes `GetLocalTransform(time)` per bone, no parent
  composition.
- `src/ECS/Systems/AudioSystem.cpp` — class defined in a `.cpp` with no header; no callers.
- `CMakeLists.txt` — `option(MIST_ENABLE_AUDIO ... OFF)`.

# Depends on / Blocks

Animation blocks nothing else; it is a leaf feature. Audio likewise.
Neither is scheduled — see `CODE_AUDIT.md` for the deferral rationale.
