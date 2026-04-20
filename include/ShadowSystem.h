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

    // Point light cubemap shadows — legacy single-light path, kept
    // as fallback. New multi-light path below.
    void InitPointLightShadow(int size = 1024);
    void BeginPointLightShadowPass(const glm::vec3& lightPos, float farPlane);
    void EndPointLightShadowPass();
    void BindPointLightShadowMap(Shader& shader, int unit = 9);

    // --- Multi-omni shadow atlas (Phase H) ---
    //
    // Allocates one GL_TEXTURE_CUBE_MAP_ARRAY with MAX_OMNI_SHADOWS
    // cubemap layers. Each frame, Renderer picks up to that many
    // shadow-enabled omni lights and renders 6 faces per layer.
    // PBR shader samples this array by the light's `shadow_idx`.
    static constexpr int MAX_OMNI_SHADOWS = 4;
    static constexpr int OMNI_SHADOW_SIZE = 512;

    void InitOmniShadowAtlas();
    // Set up the shadow pass for a single omni light's cubemap layer.
    // Caller then loops 6 faces, calling BindOmniShadowFace per face
    // and rendering scene geometry.
    void BeginOmniShadowPass(int layer, const glm::vec3& lightPos, float farPlane);
    // Attach face `f` of the current layer to the FBO, clear it, and
    // set the viewProj uniform on omniDepthShader to that face's VP.
    void BindOmniShadowFace(int face);
    void EndOmniShadowPass();
    void BindOmniShadowAtlas(Shader& shader, int unit = 8);

    int GetCurrentOmniLayer() const { return m_CurrentOmniLayer; }
    Shader omniDepthShader;

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

    // Omni shadow atlas — cubemap array + one FBO reused across all
    // faces via glFramebufferTextureLayer. Each layer = one light.
    GLuint m_OmniShadowArray = 0;
    GLuint m_OmniShadowFBO   = 0;
    int    m_CurrentOmniLayer = 0;
    std::array<glm::mat4, 6> m_OmniFaceMatrices;
    glm::vec3 m_OmniShadowLightPos{0.0f};
    float     m_OmniShadowFarPlane = 25.0f;
};

#endif
