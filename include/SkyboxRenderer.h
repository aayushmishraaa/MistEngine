#pragma once
#ifndef MIST_SKYBOX_RENDERER_H
#define MIST_SKYBOX_RENDERER_H

#include "Environment.h"
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
    void Render(const glm::mat4& view, const glm::mat4& projection,
                const Environment& env);
    void SetMode(SkyboxMode mode) { m_Mode = mode; }
    SkyboxMode GetMode() const { return m_Mode; }
    void SetCubemapTexture(GLuint tex) { m_CubemapTexture = tex; }

    // Atmospheric parameters

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
