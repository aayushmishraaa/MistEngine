#include "SSAORenderer.h"
#include "Core/Logger.h"
#include <random>

SSAORenderer::~SSAORenderer() {
    if (m_NoiseTexture) glDeleteTextures(1, &m_NoiseTexture);
}

void SSAORenderer::Init(int width, int height) {
    m_Width = width;
    m_Height = height;

    m_SSAOShader = Shader("shaders/tonemap.vert", "shaders/ssao.frag");
    m_BlurShader = Shader("shaders/tonemap.vert", "shaders/ssao_blur.frag");

    m_SSAOFBO.Create(width, height, GL_R8, false);
    m_BlurFBO.Create(width, height, GL_R8, false);

    generateKernel();
    generateNoiseTexture();

    LOG_INFO("SSAO initialized: ", width, "x", height, ", ", m_Kernel.size(), " samples");
}

void SSAORenderer::Resize(int width, int height) {
    m_Width = width;
    m_Height = height;
    m_SSAOFBO.Resize(width, height);
    m_BlurFBO.Resize(width, height);
}

void SSAORenderer::generateKernel() {
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    m_Kernel.clear();

    for (unsigned int i = 0; i < 64; ++i) {
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);

        float scale = float(i) / 64.0f;
        scale = 0.1f + scale * scale * 0.9f; // lerp
        sample *= scale;
        m_Kernel.push_back(sample);
    }
}

void SSAORenderer::generateNoiseTexture() {
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    std::vector<glm::vec3> noise;
    for (unsigned int i = 0; i < 16; i++) {
        noise.push_back(glm::vec3(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            0.0f));
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &m_NoiseTexture);
    glTextureStorage2D(m_NoiseTexture, 1, GL_RGB16F, 4, 4);
    glTextureSubImage2D(m_NoiseTexture, 0, 0, 0, 4, 4, GL_RGB, GL_FLOAT, noise.data());
    glTextureParameteri(m_NoiseTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_NoiseTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_NoiseTexture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_NoiseTexture, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void SSAORenderer::Render(GLuint depthTex, const glm::mat4& projection, const glm::mat4& view) {
    if (!enabled) return;

    // SSAO pass
    m_SSAOFBO.Bind();
    glClear(GL_COLOR_BUFFER_BIT);

    m_SSAOShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    m_SSAOShader.setInt("depthTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_NoiseTexture);
    m_SSAOShader.setInt("noiseTexture", 1);

    for (unsigned int i = 0; i < m_Kernel.size(); ++i) {
        m_SSAOShader.setVec3("samples[" + std::to_string(i) + "]", m_Kernel[i]);
    }
    m_SSAOShader.setMat4("projection", projection);
    m_SSAOShader.setMat4("view", view);
    m_SSAOShader.setFloat("radius", radius);
    m_SSAOShader.setFloat("bias", bias);
    m_SSAOShader.setVec2("screenSize", glm::vec2(m_Width, m_Height));

    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Blur pass
    m_BlurFBO.Bind();
    glClear(GL_COLOR_BUFFER_BIT);

    m_BlurShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_SSAOFBO.GetColorTexture());
    m_BlurShader.setInt("ssaoInput", 0);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
