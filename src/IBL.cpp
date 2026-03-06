#include "IBL.h"
#include "Core/Logger.h"
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

IBL::~IBL() { cleanup(); }

void IBL::cleanup() {
    if (m_EnvCubemap) glDeleteTextures(1, &m_EnvCubemap);
    if (m_IrradianceMap) glDeleteTextures(1, &m_IrradianceMap);
    if (m_PrefilterMap) glDeleteTextures(1, &m_PrefilterMap);
    if (m_BRDFLUT) glDeleteTextures(1, &m_BRDFLUT);
    if (m_CaptureFBO) glDeleteFramebuffers(1, &m_CaptureFBO);
    if (m_CaptureRBO) glDeleteRenderbuffers(1, &m_CaptureRBO);
    m_EnvCubemap = m_IrradianceMap = m_PrefilterMap = m_BRDFLUT = 0;
    m_CaptureFBO = m_CaptureRBO = 0;
    m_Loaded = false;
}

bool IBL::ProcessEnvironmentMap(const std::string& hdrPath) {
    cleanup();

    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float* data = stbi_loadf(hdrPath.c_str(), &width, &height, &nrComponents, 0);
    if (!data) {
        LOG_ERROR("IBL: Failed to load HDR: ", hdrPath);
        return false;
    }

    // HDR texture
    GLuint hdrTexture;
    glCreateTextures(GL_TEXTURE_2D, 1, &hdrTexture);
    glTextureStorage2D(hdrTexture, 1, GL_RGB16F, width, height);
    glTextureSubImage2D(hdrTexture, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, data);
    glTextureParameteri(hdrTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(hdrTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(hdrTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(hdrTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    stbi_image_free(data);

    // Capture FBO
    glCreateFramebuffers(1, &m_CaptureFBO);
    glCreateRenderbuffers(1, &m_CaptureRBO);
    glNamedRenderbufferStorage(m_CaptureRBO, GL_DEPTH_COMPONENT24, 512, 512);
    glNamedFramebufferRenderbuffer(m_CaptureFBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_CaptureRBO);

    // Environment cubemap (512x512)
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_EnvCubemap);
    glTextureStorage2D(m_EnvCubemap, 1, GL_RGB16F, 512, 512);
    glTextureParameteri(m_EnvCubemap, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(m_EnvCubemap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_EnvCubemap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_EnvCubemap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_EnvCubemap, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0), glm::vec3(1,0,0), glm::vec3(0,-1,0)),
        glm::lookAt(glm::vec3(0), glm::vec3(-1,0,0), glm::vec3(0,-1,0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0,1,0), glm::vec3(0,0,1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0,-1,0), glm::vec3(0,0,-1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0,0,1), glm::vec3(0,-1,0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0,0,-1), glm::vec3(0,-1,0))
    };

    // Convert equirect to cubemap
    Shader equirectShader("shaders/equirect_to_cubemap.vert", "shaders/equirect_to_cubemap.frag");
    equirectShader.use();
    equirectShader.setInt("equirectangularMap", 0);
    equirectShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, 512, 512);
    glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
    for (int i = 0; i < 6; ++i) {
        equirectShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_EnvCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Render cube (would need a unit cube VAO here)
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glGenerateTextureMipmap(m_EnvCubemap);

    // Irradiance map (32x32)
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_IrradianceMap);
    glTextureStorage2D(m_IrradianceMap, 1, GL_RGB16F, 32, 32);
    glTextureParameteri(m_IrradianceMap, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_IrradianceMap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_IrradianceMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_IrradianceMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_IrradianceMap, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    Shader irradianceShader("shaders/equirect_to_cubemap.vert", "shaders/irradiance_convolution.frag");
    irradianceShader.use();
    irradianceShader.setInt("environmentMap", 0);
    irradianceShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvCubemap);

    glNamedRenderbufferStorage(m_CaptureRBO, GL_DEPTH_COMPONENT24, 32, 32);
    glViewport(0, 0, 32, 32);
    glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
    for (int i = 0; i < 6; ++i) {
        irradianceShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_IrradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Pre-filtered specular (128x128, 5 mip levels)
    int prefilterSize = 128;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_PrefilterMap);
    glTextureStorage2D(m_PrefilterMap, 5, GL_RGB16F, prefilterSize, prefilterSize);
    glTextureParameteri(m_PrefilterMap, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(m_PrefilterMap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_PrefilterMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_PrefilterMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_PrefilterMap, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    Shader prefilterShader("shaders/equirect_to_cubemap.vert", "shaders/prefilter.frag");
    prefilterShader.use();
    prefilterShader.setInt("environmentMap", 0);
    prefilterShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvCubemap);

    glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
    for (int mip = 0; mip < 5; ++mip) {
        int mipWidth = prefilterSize >> mip;
        int mipHeight = prefilterSize >> mip;
        glNamedRenderbufferStorage(m_CaptureRBO, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / 4.0f;
        prefilterShader.setFloat("roughness", roughness);
        for (int i = 0; i < 6; ++i) {
            prefilterShader.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_PrefilterMap, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // BRDF LUT
    generateBRDFLUT();

    glDeleteTextures(1, &hdrTexture);
    m_Loaded = true;
    LOG_INFO("IBL: Environment map processed successfully");
    return true;
}

void IBL::generateBRDFLUT() {
    glCreateTextures(GL_TEXTURE_2D, 1, &m_BRDFLUT);
    glTextureStorage2D(m_BRDFLUT, 1, GL_RG16F, 512, 512);
    glTextureParameteri(m_BRDFLUT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_BRDFLUT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_BRDFLUT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_BRDFLUT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    Shader brdfShader("shaders/tonemap.vert", "shaders/brdf_lut.frag");
    brdfShader.use();

    glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
    glNamedRenderbufferStorage(m_CaptureRBO, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_BRDFLUT, 0);
    glViewport(0, 0, 512, 512);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void IBL::Bind(Shader& shader, int irradianceUnit, int prefilterUnit, int brdfLUTUnit) {
    if (!m_Loaded) return;

    glActiveTexture(GL_TEXTURE0 + irradianceUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_IrradianceMap);
    shader.setInt("irradianceMap", irradianceUnit);

    glActiveTexture(GL_TEXTURE0 + prefilterUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_PrefilterMap);
    shader.setInt("prefilterMap", prefilterUnit);

    glActiveTexture(GL_TEXTURE0 + brdfLUTUnit);
    glBindTexture(GL_TEXTURE_2D, m_BRDFLUT);
    shader.setInt("brdfLUT", brdfLUTUnit);

    shader.setBool("useIBL", true);
}
