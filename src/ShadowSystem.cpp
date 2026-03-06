#include "ShadowSystem.h"
#include "Camera.h"
#include "Core/Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

ShadowSystem::~ShadowSystem() {
    if (m_CSMArrayTexture) glDeleteTextures(1, &m_CSMArrayTexture);
    if (m_CSMFBO) glDeleteFramebuffers(1, &m_CSMFBO);
    if (m_PointShadowCubemap) glDeleteTextures(1, &m_PointShadowCubemap);
    if (m_PointShadowFBO) glDeleteFramebuffers(1, &m_PointShadowFBO);
}

void ShadowSystem::Init() {
    csmDepthShader = Shader("shaders/csm_depth.vert", "shaders/csm_depth.frag");

    // Create texture array for cascades
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_CSMArrayTexture);
    glTextureStorage3D(m_CSMArrayTexture, 1, GL_DEPTH_COMPONENT32F, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, NUM_CASCADES);
    glTextureParameteri(m_CSMArrayTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_CSMArrayTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_CSMArrayTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(m_CSMArrayTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTextureParameterfv(m_CSMArrayTexture, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTextureParameteri(m_CSMArrayTexture, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTextureParameteri(m_CSMArrayTexture, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    // Create FBO
    glCreateFramebuffers(1, &m_CSMFBO);
    glNamedFramebufferDrawBuffer(m_CSMFBO, GL_NONE);
    glNamedFramebufferReadBuffer(m_CSMFBO, GL_NONE);

    LOG_INFO("ShadowSystem initialized: ", NUM_CASCADES, " cascades, ", SHADOW_MAP_SIZE, "x", SHADOW_MAP_SIZE);
}

void ShadowSystem::CalculateCascades(const Camera& camera, const glm::vec3& lightDir, float nearPlane, float farPlane) {
    float lambda = 0.75f; // logarithmic/uniform blend factor

    for (int i = 0; i < NUM_CASCADES; i++) {
        float p = (float)(i + 1) / (float)NUM_CASCADES;
        float logSplit = nearPlane * std::pow(farPlane / nearPlane, p);
        float uniformSplit = nearPlane + (farPlane - nearPlane) * p;
        m_CascadeSplits[i] = lambda * logSplit + (1.0f - lambda) * uniformSplit;
    }

    float lastSplitDist = nearPlane;
    glm::mat4 viewMatrix = camera.GetViewMatrix();
    glm::mat4 projMatrix = glm::perspective(glm::radians(camera.Zoom), 1.6f, nearPlane, farPlane);
    glm::mat4 invCam = glm::inverse(projMatrix * viewMatrix);

    for (int cascade = 0; cascade < NUM_CASCADES; cascade++) {
        float splitDist = m_CascadeSplits[cascade];

        // Calculate frustum corners for this cascade
        glm::vec3 frustumCorners[8] = {
            {-1, 1, -1}, {1, 1, -1}, {1, -1, -1}, {-1, -1, -1},
            {-1, 1,  1}, {1, 1,  1}, {1, -1,  1}, {-1, -1,  1}
        };

        glm::mat4 cascadeProj = glm::perspective(glm::radians(camera.Zoom), 1.6f, lastSplitDist, splitDist);
        glm::mat4 invCascade = glm::inverse(cascadeProj * viewMatrix);

        for (auto& corner : frustumCorners) {
            glm::vec4 pt = invCascade * glm::vec4(corner, 1.0f);
            corner = glm::vec3(pt) / pt.w;
        }

        // Calculate center
        glm::vec3 center(0.0f);
        for (const auto& c : frustumCorners) center += c;
        center /= 8.0f;

        // Calculate radius for stable shadows
        float radius = 0.0f;
        for (const auto& c : frustumCorners) {
            radius = std::max(radius, glm::length(c - center));
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        glm::vec3 maxExtents(radius);
        glm::vec3 minExtents = -maxExtents;

        glm::mat4 lightView = glm::lookAt(center - glm::normalize(lightDir) * (-minExtents.z + 10.0f), center, glm::vec3(0, 1, 0));
        glm::mat4 lightProj = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z + 20.0f);

        // Texel grid snapping to prevent shadow edge swimming
        glm::mat4 shadowMatrix = lightProj * lightView;
        glm::vec4 shadowOrigin = shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        shadowOrigin *= (float)SHADOW_MAP_SIZE / 2.0f;
        glm::vec4 roundedOrigin = glm::round(shadowOrigin);
        glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
        roundOffset *= 2.0f / (float)SHADOW_MAP_SIZE;
        roundOffset.z = 0.0f;
        roundOffset.w = 0.0f;
        lightProj[3] += roundOffset;

        m_LightSpaceMatrices[cascade] = lightProj * lightView;
        lastSplitDist = splitDist;
    }
}

void ShadowSystem::BeginShadowPass(int cascadeIndex) {
    glNamedFramebufferTextureLayer(m_CSMFBO, GL_DEPTH_ATTACHMENT, m_CSMArrayTexture, 0, cascadeIndex);
    glBindFramebuffer(GL_FRAMEBUFFER, m_CSMFBO);
    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    csmDepthShader.use();
    csmDepthShader.setMat4("lightSpaceMatrix", m_LightSpaceMatrices[cascadeIndex]);
}

void ShadowSystem::EndShadowPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowSystem::BindCascadeShadowMaps(Shader& shader, int startUnit) {
    glActiveTexture(GL_TEXTURE0 + startUnit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_CSMArrayTexture);
    shader.setInt("cascadeShadowMap", startUnit);

    for (int i = 0; i < NUM_CASCADES; i++) {
        shader.setMat4("lightSpaceMatrices[" + std::to_string(i) + "]", m_LightSpaceMatrices[i]);
        shader.setFloat("cascadeSplits[" + std::to_string(i) + "]", m_CascadeSplits[i]);
    }
    shader.setBool("showCascadeColors", showCascadeColors);
}

void ShadowSystem::InitPointLightShadow(int size) {
    m_PointShadowSize = size;

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_PointShadowCubemap);
    glTextureStorage2D(m_PointShadowCubemap, 1, GL_DEPTH_COMPONENT32F, size, size);
    glTextureParameteri(m_PointShadowCubemap, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_PointShadowCubemap, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_PointShadowCubemap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_PointShadowCubemap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_PointShadowCubemap, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glCreateFramebuffers(1, &m_PointShadowFBO);
    glNamedFramebufferDrawBuffer(m_PointShadowFBO, GL_NONE);
    glNamedFramebufferReadBuffer(m_PointShadowFBO, GL_NONE);
}

void ShadowSystem::BeginPointLightShadowPass(const glm::vec3& lightPos, float farPlane) {
    float aspect = 1.0f;
    float near = 0.1f;
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, farPlane);

    m_PointLightMatrices[0] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1,0,0), glm::vec3(0,-1,0));
    m_PointLightMatrices[1] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1,0,0), glm::vec3(0,-1,0));
    m_PointLightMatrices[2] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0,1,0), glm::vec3(0,0,1));
    m_PointLightMatrices[3] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0,-1,0), glm::vec3(0,0,-1));
    m_PointLightMatrices[4] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0,0,1), glm::vec3(0,-1,0));
    m_PointLightMatrices[5] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0,0,-1), glm::vec3(0,-1,0));

    glBindFramebuffer(GL_FRAMEBUFFER, m_PointShadowFBO);
    glViewport(0, 0, m_PointShadowSize, m_PointShadowSize);
}

void ShadowSystem::EndPointLightShadowPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowSystem::BindPointLightShadowMap(Shader& shader, int unit) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_PointShadowCubemap);
    shader.setInt("pointShadowMap", unit);
}
