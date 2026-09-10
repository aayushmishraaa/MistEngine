#include "ECS/Systems/RenderSystem.h"
#include "ECS/Components/AnimationComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RenderComponent.h"

#include "Assets/MaterialCache.h"
#include "Material.h"
#include "Mesh.h"

extern Coordinator gCoordinator;

void RenderSystem::Update(Shader& shader) {
    for (auto const& entity : m_Entities) {
        auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
        auto& render = gCoordinator.GetComponent<RenderComponent>(entity);

        if (!(render.visible && render.renderable)) continue;

        // Skinned entities are routed through UpdateSkinned(shader)
        // under a different vertex program. Skip them here so they
        // don't draw twice with the wrong vertex layout.
        if (gCoordinator.HasComponent<AnimationComponent>(entity)) continue;

        glm::mat4 model = transform.WorldMatrix();
        shader.setMat4("model", model);

        // Material override. If the component points at a .mistmat
        // path, swap the mesh's inline pbrMaterial for the cached
        // asset for the duration of this Draw, then restore. The
        // swap is safe because RenderSystem is single-threaded and
        // every Draw completes before we touch the pointer again.
        if (!render.materialPath.empty()) {
            if (auto* mesh = dynamic_cast<Mesh*>(render.renderable)) {
                auto overrideMat = Mist::Assets::MaterialCache::Instance()
                    .Get(render.materialPath);
                if (overrideMat) {
                    auto saved = mesh->pbrMaterial;
                    mesh->pbrMaterial = overrideMat;
                    mesh->Draw(shader);
                    mesh->pbrMaterial = saved;
                    continue;
                }
            }
        }

        render.renderable->Draw(shader);
    }
}

void RenderSystem::UpdateSkinned(Shader& shader, float dt) {
    for (auto const& entity : m_Entities) {
        if (!gCoordinator.HasComponent<AnimationComponent>(entity)) continue;

        auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
        auto& render    = gCoordinator.GetComponent<RenderComponent>(entity);
        auto& anim      = gCoordinator.GetComponent<AnimationComponent>(entity);

        if (!(render.visible && render.renderable)) continue;

        // Advance the clip clock.
        //
        // TODO(C5): skinning is NOT functional yet and this is the honest
        // minimum that stops it corrupting the frame. Three separate things are
        // still wrong, tracked together:
        //   1. `AnimationComponent::animator` is default-constructed and
        //      nothing calls Init() on it, so its SSBO handle is 0. The
        //      `BindBoneSSBO(6)` that used to sit here bound buffer name 0 —
        //      unbinding the slot the skinned vertex shader reads — and
        //      Animator::Update's glNamedBufferSubData(0, ...) raised a GL
        //      error every frame for every animated entity. Removed; the
        //      AnimatedModel's own initialised Animator owns binding 6, and
        //      AnimatedModel::Draw binds it.
        //   2. Animator::calculateBoneTransforms writes each bone's LOCAL
        //      transform and never walks the skeleton hierarchy.
        //   3. BoneInfo::offset — the inverse bind matrix, stored in
        //      AnimatedModel.cpp — is never applied, and
        //      AnimatedModel::calculateBoneTransformsHierarchy is declared but
        //      has no definition at all.
        // Net effect today: rigs import, clips populate the Inspector, the
        // clock advances, and the mesh renders in its bind pose. Fixing it
        // needs the hierarchy walk written and a rigged asset to verify
        // against, neither of which belongs in a bug-fix pass.
        anim.Update(dt);

        shader.setMat4("model", transform.WorldMatrix());
        render.renderable->Draw(shader);
    }
}

void RenderSystem::UpdateVelocity(Shader& shader) {
    // Two passes: first draw, using cached prev-model (or current if
    // new entity); then snapshot current models for next frame.
    for (auto const& entity : m_Entities) {
        auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
        auto& render    = gCoordinator.GetComponent<RenderComponent>(entity);
        if (!render.visible || !render.renderable) continue;

        glm::mat4 model = transform.WorldMatrix();
        auto it = m_PrevModels.find(entity);
        // First-seen entities: prevModel == model -> zero velocity on
        // spawn. No smear on a just-created cube.
        glm::mat4 prev = (it != m_PrevModels.end()) ? it->second : model;

        shader.setMat4("model",     model);
        shader.setMat4("prevModel", prev);
        render.renderable->Draw(shader);
    }

    // Snapshot pass — captures this frame's transforms for next
    // frame's velocity computation. Drop entries for entities no
    // longer in m_Entities so the map doesn't grow unbounded when
    // things get destroyed.
    std::unordered_map<Entity, glm::mat4> next;
    next.reserve(m_Entities.size());
    for (auto const& entity : m_Entities) {
        next[entity] = gCoordinator.GetComponent<TransformComponent>(entity).WorldMatrix();
    }
    m_PrevModels = std::move(next);
}

void RenderSystem::UpdateDepthOnly(Shader& shader) {
    // Prepass walk. `render.renderable->Draw(shader)` will call
    // material Bind() paths inside Mesh — but the prepass shader
    // doesn't consume those samplers, so the GL binds are wasted-
    // not-broken. A dedicated "draw geometry only" path on
    // Renderable would avoid them; noted for a follow-up. The
    // per-fragment savings (no PBR math, no shadow sampling) still
    // dominate the redundant texture binds.
    for (auto const& entity : m_Entities) {
        auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
        auto& render    = gCoordinator.GetComponent<RenderComponent>(entity);
        if (render.visible && render.renderable) {
            shader.setMat4("model", transform.WorldMatrix());
            render.renderable->Draw(shader);
        }
    }
}
