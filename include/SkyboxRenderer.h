#pragma once
#ifndef MIST_SKYBOX_RENDERER_H
#define MIST_SKYBOX_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

enum class SkyboxMode {
    Procedural,
    HDRCubemap,
    Atmospheric
};

class SkyboxRenderer {
public:
    SkyboxRenderer() = default;
    ~SkyboxRenderer();

    void Init();
    void Render(const glm::mat4& view, const glm::mat4& projection);
    void SetMode(SkyboxMode mode) { m_Mode = mode; }
    SkyboxMode GetMode() const { return m_Mode; }
    void SetCubemapTexture(GLuint tex) { m_CubemapTexture = tex; }

    // Atmospheric parameters
    glm::vec3 sunDirection = glm::normalize(glm::vec3(0.5f, 0.3f, 0.8f));
    float rayleighStrength = 1.0f;
    float mieStrength = 0.005f;
    float turbidity = 2.0f;

private:
    GLuint m_SkyboxVAO = 0;
    GLuint m_SkyboxVBO = 0;
    GLuint m_CubemapTexture = 0;

    Shader m_ProceduralShader;
    Shader m_CubemapShader;
    Shader m_AtmosphericShader;
    SkyboxMode m_Mode = SkyboxMode::Procedural;

    void setupCube();
};

#endif
