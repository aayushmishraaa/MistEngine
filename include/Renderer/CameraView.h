#pragma once
#ifndef MIST_CAMERA_VIEW_H
#define MIST_CAMERA_VIEW_H

// The resolved viewpoint for one frame.
//
// Problem this solves: `Renderer::kNearPlane` / `kFarPlane` were shared
// constants precisely because four subsystems have to agree on the depth
// range — the projection matrix, the PBR shader's nearPlane/farPlane uniforms,
// the CSM cascade split computation, and the cluster grid's log-z slicing.
// When those were eight separate literals, editing one silently
// desynchronised the clustered-light lookup from the grid it indexes into,
// which is the bug fixed at commit ea91d7c.
//
// Making the depth range per-camera reintroduces exactly that hazard, only
// worse: the values now vary at runtime. So instead of each site reading
// `camera` plus the constants, the frame resolves ONE CameraView at the top of
// RenderWithECSAndUI and every consumer takes it. There is one place where the
// active camera is chosen and one place where near/far are decided.
//
// `aspect` is carried for the same reason. ShadowSystem::CalculateCascades
// used to hardcode 1.6 while the projection matrix used the real viewport
// ratio, so the cascade frusta it fitted were not the frustum being rendered —
// wrong-sized cascades at any aspect other than 16:10.

#include <glm/glm.hpp>

struct CameraView {
    glm::mat4 view       {1.0f};
    glm::mat4 projection {1.0f};  // unjittered; TAA applies its own offset
    glm::vec3 position   {0.0f};

    float fovDegrees = 45.0f;
    float aspect     = 16.0f / 9.0f;
    float nearPlane  = 0.1f;
    float farPlane   = 100.0f;

    // True when this view came from a CameraComponent in the scene rather than
    // from the editor's free-fly camera. Consumers that need to behave
    // differently in the editor (gizmos, the grid) can ask.
    bool fromScene = false;
};

#endif // MIST_CAMERA_VIEW_H
