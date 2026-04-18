#include "LightManager.h"
#include "Core/Logger.h"

LightManager::~LightManager() {
    if (m_LightSSBO) glDeleteBuffers(1, &m_LightSSBO);
    if (m_ClusterAABBSSBO) glDeleteBuffers(1, &m_ClusterAABBSSBO);
    if (m_LightIndexSSBO) glDeleteBuffers(1, &m_LightIndexSSBO);
    if (m_LightGridSSBO) glDeleteBuffers(1, &m_LightGridSSBO);
}

void LightManager::Init() {
    m_ClusterBuildShader = Shader("shaders/cluster_build.comp");
    m_ClusterCullShader = Shader("shaders/cluster_cull.comp");

    // Light SSBO (binding 2)
    glCreateBuffers(1, &m_LightSSBO);
    glNamedBufferStorage(m_LightSSBO, MAX_LIGHTS * sizeof(Light), nullptr, GL_DYNAMIC_STORAGE_BIT);

    // Cluster AABB SSBO (binding 5)
    int totalClusters = CLUSTER_X * CLUSTER_Y * CLUSTER_Z;
    glCreateBuffers(1, &m_ClusterAABBSSBO);
    glNamedBufferStorage(m_ClusterAABBSSBO, totalClusters * 2 * sizeof(glm::vec4), nullptr, GL_DYNAMIC_STORAGE_BIT);

    // Light index SSBO (binding 3)
    glCreateBuffers(1, &m_LightIndexSSBO);
    glNamedBufferStorage(m_LightIndexSSBO, totalClusters * MAX_LIGHTS_PER_CLUSTER * sizeof(int), nullptr, GL_DYNAMIC_STORAGE_BIT);

    // Light grid SSBO (binding 4) - ivec2 per cluster (offset, count)
    glCreateBuffers(1, &m_LightGridSSBO);
    glNamedBufferStorage(m_LightGridSSBO, totalClusters * sizeof(glm::ivec2), nullptr, GL_DYNAMIC_STORAGE_BIT);

    m_Initialized = true;
    LOG_INFO("LightManager initialized: ", totalClusters, " clusters, max ", MAX_LIGHTS, " lights");
}

int LightManager::AddLight(const Light& light) {
    if ((int)m_Lights.size() >= MAX_LIGHTS) {
        LOG_WARN("LightManager: Max lights reached (", MAX_LIGHTS, ")");
        return -1;
    }
    m_Lights.push_back(light);
    m_LightsDirty = true;
    return (int)m_Lights.size() - 1;
}

void LightManager::RemoveLight(int index) {
    if (index >= 0 && index < (int)m_Lights.size()) {
        m_Lights.erase(m_Lights.begin() + index);
        m_LightsDirty = true;
    }
}

void LightManager::UploadToGPU() {
    if (!m_Initialized || m_Lights.empty()) return;
    if (!m_LightsDirty) return; // Skip upload when light set is unchanged.
    glNamedBufferSubData(m_LightSSBO, 0, m_Lights.size() * sizeof(Light), m_Lights.data());
    m_LightsDirty = false;
}

void LightManager::BuildClusters(float nearPlane, float farPlane, int screenW, int screenH) {
    if (!m_Initialized || !m_ClusterBuildShader.isValid()) return;

    m_ClusterBuildShader.use();
    m_ClusterBuildShader.setFloat("nearPlane", nearPlane);
    m_ClusterBuildShader.setFloat("farPlane", farPlane);
    m_ClusterBuildShader.setVec2("screenSize", glm::vec2(screenW, screenH));

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_ClusterAABBSSBO);
    glDispatchCompute(CLUSTER_X, CLUSTER_Y, CLUSTER_Z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void LightManager::CullLights(const glm::mat4& view, const glm::mat4& projection) {
    if (!m_Initialized || !m_ClusterCullShader.isValid() || m_Lights.empty()) return;

    m_ClusterCullShader.use();
    m_ClusterCullShader.setMat4("viewMatrix", view);
    m_ClusterCullShader.setMat4("projectionMatrix", projection);
    m_ClusterCullShader.setInt("lightCount", (int)m_Lights.size());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_LightSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_LightIndexSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_LightGridSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_ClusterAABBSSBO);

    int totalClusters = CLUSTER_X * CLUSTER_Y * CLUSTER_Z;
    int workGroupSize = 256;
    int numGroups = (totalClusters + workGroupSize - 1) / workGroupSize;
    glDispatchCompute(numGroups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void LightManager::BindForRendering() {
    if (!m_Initialized) return;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_LightSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_LightIndexSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_LightGridSSBO);
}
