#include "BloomRenderer.h"
#include "Core/Logger.h"

BloomRenderer::~BloomRenderer() {
    cleanup();
}

void BloomRenderer::cleanup() {
    for (auto& mip : m_MipChain) {
        if (mip.texture) glDeleteTextures(1, &mip.texture);
    }
    m_MipChain.clear();
    if (m_FBO) { glDeleteFramebuffers(1, &m_FBO); m_FBO = 0; }
}

void BloomRenderer::Init(int width, int height, int mipLevels) {
    m_MipLevels = mipLevels;
    m_SrcWidth = width;
    m_SrcHeight = height;

    m_DownsampleShader = Shader("shaders/tonemap.vert", "shaders/bloom_downsample.frag");
    m_UpsampleShader = Shader("shaders/tonemap.vert", "shaders/bloom_upsample.frag");

    glCreateFramebuffers(1, &m_FBO);
    createMipChain(width, height);

    LOG_INFO("BloomRenderer initialized: ", mipLevels, " mip levels");
}

void BloomRenderer::createMipChain(int width, int height) {
    for (auto& mip : m_MipChain) {
        if (mip.texture) glDeleteTextures(1, &mip.texture);
    }
    m_MipChain.clear();

    glm::vec2 mipSize((float)width, (float)height);
    for (int i = 0; i < m_MipLevels; i++) {
        mipSize *= 0.5f;
        mipSize = glm::max(mipSize, glm::vec2(1.0f));

        BloomMip mip;
        mip.size = mipSize;
        glCreateTextures(GL_TEXTURE_2D, 1, &mip.texture);
        glTextureStorage2D(mip.texture, 1, GL_R11F_G11F_B10F, (int)mipSize.x, (int)mipSize.y);
        glTextureParameteri(mip.texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(mip.texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(mip.texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(mip.texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        m_MipChain.push_back(mip);
    }
}

void BloomRenderer::Resize(int width, int height) {
    if (width == m_SrcWidth && height == m_SrcHeight) return;
    m_SrcWidth = width;
    m_SrcHeight = height;
    createMipChain(width, height);
}

void BloomRenderer::RenderBloom(GLuint srcTexture, float thresh, float inten) {
    if (!enabled || m_MipChain.empty()) return;

    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    // Downsample pass
    m_DownsampleShader.use();
    m_DownsampleShader.setVec2("srcResolution", glm::vec2(m_SrcWidth, m_SrcHeight));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srcTexture);
    m_DownsampleShader.setInt("srcTexture", 0);

    for (int i = 0; i < (int)m_MipChain.size(); i++) {
        auto& mip = m_MipChain[i];
        glViewport(0, 0, (int)mip.size.x, (int)mip.size.y);
        glNamedFramebufferTexture(m_FBO, GL_COLOR_ATTACHMENT0, mip.texture, 0);

        m_DownsampleShader.setInt("mipLevel", i);
        m_DownsampleShader.setFloat("threshold", (i == 0) ? thresh : 0.0f);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Next iteration reads from this mip
        m_DownsampleShader.setVec2("srcResolution", mip.size);
        glBindTexture(GL_TEXTURE_2D, mip.texture);
    }

    // Upsample pass
    m_UpsampleShader.use();
    m_UpsampleShader.setFloat("bloomIntensity", inten);
    m_UpsampleShader.setInt("srcTexture", 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);

    for (int i = (int)m_MipChain.size() - 1; i > 0; i--) {
        auto& mip = m_MipChain[i];
        auto& nextMip = m_MipChain[i - 1];

        glBindTexture(GL_TEXTURE_2D, mip.texture);
        glViewport(0, 0, (int)nextMip.size.x, (int)nextMip.size.y);
        glNamedFramebufferTexture(m_FBO, GL_COLOR_ATTACHMENT0, nextMip.texture, 0);

        m_UpsampleShader.setVec2("srcResolution", mip.size);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint BloomRenderer::GetBloomTexture() const {
    if (!m_MipChain.empty()) return m_MipChain[0].texture;
    return 0;
}
