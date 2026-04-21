#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H

#include "../System.h"
#include "../Coordinator.h"
#include "../../Shader.h"

#include <glm/glm.hpp>
#include <unordered_map>

extern Coordinator gCoordinator;

class RenderSystem : public System {
public:
    // Hide the base's `Update(float)` explicitly. RenderSystem takes a shader
    // rather than deltaTime because it's driven by the Renderer pass, not by
    // the generic scheduler — `using` import silences GCC's
    // -Woverloaded-virtual warning without changing runtime behaviour.
    using System::Update;
    void Update(Shader& shader);

    // Depth-prepass variant. Iterates the same entity set as Update()
    // but binds only the minimum uniforms the prepass shader needs
    // (model + roughness) — no material texture binds. Main-pass
    // materials / lights / shadows are skipped, which is the point.
    void UpdateDepthOnly(Shader& shader);

    // Velocity-pass variant. Writes per-entity (model, prevModel)
    // uniforms so the velocity fragment shader outputs a screen-space
    // motion vector. `m_PrevModels` keeps a sidecar of last frame's
    // model matrices — kept out of `TransformComponent` so the ECS
    // cache line stays lean (motion tracking is a rendering concern,
    // not a transform one).
    void UpdateVelocity(Shader& shader);

    // Skinned-pass variant. Iterates the same entity set as Update()
    // but only processes entities that carry an AnimationComponent
    // (rigged imports). Advances each entity's Animator by dt,
    // uploads its bone matrices to the bone SSBO (binding 6), then
    // draws through `shader` — which must be a program that binds
    // `skinned_pbr.vert` so the per-vertex bone attributes are
    // consumed. `Update(shader)` skips these entities to avoid
    // double-rendering.
    void UpdateSkinned(Shader& shader, float dt);

private:
    std::unordered_map<Entity, glm::mat4> m_PrevModels;
};

#endif // RENDERSYSTEM_H
