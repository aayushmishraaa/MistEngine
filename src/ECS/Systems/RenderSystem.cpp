#include "ECS/Systems/RenderSystem.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RenderComponent.h"

void RenderSystem::Update(Shader& shader) {
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
