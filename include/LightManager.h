#pragma once
#ifndef MIST_LIGHT_MANAGER_H
#define MIST_LIGHT_MANAGER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Light.h"
#include "Shader.h"

class LightManager {
public:
    static constexpr int MAX_LIGHTS = 1024;
    static constexpr int CLUSTER_X = 16;
    static constexpr int CLUSTER_Y = 9;
    static constexpr int CLUSTER_Z = 24;
    static constexpr int MAX_LIGHTS_PER_CLUSTER = 128;

    LightManager() = default;
    ~LightManager();

    void Init();

    // Light management
    int AddLight(const Light& light);
    void RemoveLight(int index);
    Light& GetLight(int index) { return m_Lights[index]; }
    const std::vector<Light>& GetLights() const { return m_Lights; }
    int GetLightCount() const { return (int)m_Lights.size(); }

    // Per-frame operations
    void UploadToGPU();
    void BuildClusters(float nearPlane, float farPlane, int screenW, int screenH);
    void CullLights(const glm::mat4& view, const glm::mat4& projection);

    // Bind SSBOs for shaders
    void BindForRendering();

private:
    std::vector<Light> m_Lights;

    // SSBOs
    GLuint m_LightSSBO = 0;        // binding 2
    GLuint m_ClusterAABBSSBO = 0;  // binding 5
    GLuint m_LightIndexSSBO = 0;   // binding 3
    GLuint m_LightGridSSBO = 0;    // binding 4

    // Compute shaders
    Shader m_ClusterBuildShader;
    Shader m_ClusterCullShader;

    bool m_Initialized = false;
};

#endif
