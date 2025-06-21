
#include "PlaneGameObject.h"
#include <glad/glad.h> // Include glad for OpenGL functions

#include "Shader.h"
PlaneGameObject::PlaneGameObject(unsigned int vao)
    : m_vao(vao)
{
    // Set initial properties for the plane
    position = glm::vec3(0.0f, -0.5f, 0.0f); // Matches the position where the plane vertices were defined
    scale = glm::vec3(1.0f);
    rotation = glm::vec3(0.0f);
}

void PlaneGameObject::render(Shader& shader)
{
    // Use the passed shader
    shader.use();

    // Set model matrix uniform
    glm::mat4 model = getModelMatrix();
    shader.setMat4("model", model);

    // Bind the plane's VAO and draw
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6); // Assuming 6 vertices for a simple plane
    glBindVertexArray(0);
}