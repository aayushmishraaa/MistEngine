#pragma once
#ifndef MIST_SHADOW_SYSTEM_H
#define MIST_SHADOW_SYSTEM_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <array>
#include "Shader.h"
#include "Renderer/RID.h"

class Camera;

class ShadowSystem {
public:
    static constexpr int NUM_CASCADES = 4;
    static constexpr int SHADOW_MAP_SIZE = 2048;

    ShadowSystem() = default;
    ~ShadowSystem();

    void Init();
    void CalculateCascades(const Camera& camera, const glm::vec3& lightDir, float nearPlane, float farPlane);
    void BeginShadowPass(int cascadeIndex);
    void EndShadowPass();
    void BindCascadeShadowMaps(Shader& shader, int startUnit = 0);

    const glm::mat4& GetLightSpaceMatrix(int cascade) const { return m_LightSpaceMatrices[cascade]; }
    const std::array<float, NUM_CASCADES>& GetCascadeSplits() const { return m_CascadeSplits; }

    // Point light cubemap shadows
    void InitPointLightShadow(int size = 1024);
    void BeginPointLightShadowPass(const glm::vec3& lightPos, float farPlane);
    void EndPointLightShadowPass();
    void BindPointLightShadowMap(Shader& shader, int unit = 9);

    // Debug
    bool showCascadeColors = false;

    Shader csmDepthShader;
    Shader pointDepthShader;

private:
    // CSM cascade texture array — lifetime via RenderingDevice, cached
    // GLuint for sampler bind path. FBO stays raw (no interface for FBOs
    // yet).
    GLuint m_CSMArrayTexture = 0;
    RID    m_CSMArrayRID{};
    GLuint m_CSMFBO = 0;
    std::array<glm::mat4, NUM_CASCADES> m_LightSpaceMatrices;
    std::array<float, NUM_CASCADES> m_CascadeSplits;

    // Point light
    GLuint m_PointShadowCubemap = 0;
    GLuint m_PointShadowFBO = 0;
    int m_PointShadowSize = 1024;
    std::array<glm::mat4, 6> m_PointLightMatrices;
};

#endif
