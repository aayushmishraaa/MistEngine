#include "/home/runner/MistEngine/include/BackpackGameObject.h"

#include "/home/runner/MistEngine/include/Shader.h"

BackpackGameObject::BackpackGameObject(const std::string& modelPath)
    : m_model(modelPath)
{
    // Set initial properties for the backpack
    position = glm::vec3(0.0f, 5.0f, 0.0f); // Initial position above the plane
    scale = glm::vec3(1.0f); // Adjust scale as needed
    rotation = glm::vec3(0.0f);
}

void BackpackGameObject::render(Shader& shader)
{
    // Use the passed shader
    shader.use();

    // Set model matrix uniform
    glm::mat4 model = getModelMatrix();
    shader.setMat4("model", model);

    // TODO: Set view and projection matrix uniforms here or ideally before calling Scene::render()
    // Also, set any other necessary uniforms like material properties, light properties, etc.

    // Render the model
    m_model.Draw(shader); // Assuming Model::Draw takes a Shader object or reference
}
