#include "SkyboxRenderer.h"
#include "Core/Logger.h"
#include <glm/gtc/matrix_transform.hpp>

SkyboxRenderer::~SkyboxRenderer() {
    if (m_SkyboxVAO) glDeleteVertexArrays(1, &m_SkyboxVAO);
    if (m_SkyboxVBO) glDeleteBuffers(1, &m_SkyboxVBO);
}

void SkyboxRenderer::Init() {
    m_ProceduralShader = Shader("shaders/skybox_vertex.glsl", "shaders/skybox_fragment.glsl");
    m_CubemapShader = Shader("shaders/skybox_cubemap.vert", "shaders/skybox_cubemap.frag");
    m_AtmosphericShader = Shader("shaders/skybox_atmosphere.vert", "shaders/skybox_atmosphere.frag");
    setupCube();
    LOG_INFO("SkyboxRenderer initialized");
}

void SkyboxRenderer::setupCube() {
    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };
    glGenVertexArrays(1, &m_SkyboxVAO);
    glGenBuffers(1, &m_SkyboxVBO);
    glBindVertexArray(m_SkyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_SkyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void SkyboxRenderer::Render(const glm::mat4& view, const glm::mat4& projection) {
    glDepthFunc(GL_LEQUAL);
    glm::mat4 skyView = glm::mat4(glm::mat3(view));

    Shader* shader = nullptr;
    switch (m_Mode) {
        case SkyboxMode::Procedural:
            shader = &m_ProceduralShader;
            shader->use();
            shader->setMat4("view", skyView);
            shader->setMat4("projection", projection);
            break;

        case SkyboxMode::HDRCubemap:
            shader = &m_CubemapShader;
            shader->use();
            shader->setMat4("view", skyView);
            shader->setMat4("projection", projection);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_CubemapTexture);
            shader->setInt("environmentMap", 0);
            break;

        case SkyboxMode::Atmospheric:
            shader = &m_AtmosphericShader;
            shader->use();
            shader->setMat4("view", skyView);
            shader->setMat4("projection", projection);
            shader->setVec3("sunDirection", sunDirection);
            shader->setFloat("rayleighStrength", rayleighStrength);
            shader->setFloat("mieStrength", mieStrength);
            shader->setFloat("turbidity", turbidity);
            break;
    }

    glBindVertexArray(m_SkyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}
