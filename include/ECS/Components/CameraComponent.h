#ifndef CAMERACOMPONENT_H
#define CAMERACOMPONENT_H

#include "Core/Reflection.h"

// A viewpoint that lives in the scene — Godot's Camera3D.
//
// Before this, `Camera camera;` was a private data member of `Renderer`. That
// single fact blocked a surprising amount: one camera ever, never saved, so
// framing was lost on exit; no second viewpoint, so no minimap and no
// render-to-texture; no cutscene camera; and a prefab containing a viewpoint
// was meaningless.
//
// Position and orientation deliberately are NOT here — they come from the
// entity's TransformComponent, the way they do for every other spatial thing
// in the engine. This component carries only what a transform cannot express:
// the projection.
//
// The editor's free-fly camera stays a separate `Camera` object owned by the
// Renderer. Godot has the same split: the editor viewport camera is not a node
// in the scene you are editing, and conflating them means the editor's
// framing gets saved into the level.
struct CameraComponent {
    // Vertical FOV. Matches the `Camera::Zoom` default so a scene that gains a
    // camera does not visibly jump.
    float fovDegrees = 45.0f;

    // Per-camera depth range. These two numbers have to reach the projection
    // matrix, the PBR shader uniforms, the CSM cascade splits and the cluster
    // grid together — see CameraView, which is the single object that carries
    // them so they cannot drift apart.
    //
    // Widening the range is not free: the cluster grid slices log-z between
    // them, so a far plane of 10000 spreads the same 24 slices over a much
    // larger range and coarsens light culling everywhere.
    float nearPlane = 0.1f;
    float farPlane  = 100.0f;

    // The renderer uses the first active camera it finds. More than one active
    // camera is not an error — it just means the winner is iteration order,
    // which is why the editor marks the active one.
    bool active = true;
};

MIST_REFLECT(CameraComponent)
    MIST_FIELD(CameraComponent, fovDegrees, ::Mist::PropertyHint::Range, "1,179")
    MIST_FIELD(CameraComponent, nearPlane,  ::Mist::PropertyHint::Range, "0.001,10")
    MIST_FIELD(CameraComponent, farPlane,   ::Mist::PropertyHint::Range, "1,10000")
    MIST_FIELD(CameraComponent, active,     ::Mist::PropertyHint::None,  "")
MIST_REFLECT_END(CameraComponent)

#endif // CAMERACOMPONENT_H
