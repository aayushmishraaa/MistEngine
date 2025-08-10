#include "ECS/Systems/RenderSystem.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RenderComponent.h"

extern Coordinator gCoordinator;

void RenderSystem::Update(Shader& shader) {
    // Silent operation - no per-frame logging spam
    
    if (m_Entities.size() == 0) {
        return;
    }
    
    for (auto const& entity : m_Entities) {
        auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
        auto& render = gCoordinator.GetComponent<RenderComponent>(entity);
        
        if (render.visible && render.renderable) {
            glm::mat4 model = transform.GetModelMatrix();
            shader.setMat4("model", model);
            render.renderable->Draw(shader);
        }
    }
}
