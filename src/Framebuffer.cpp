#include "Framebuffer.h"
#include "Core/Logger.h"

Framebuffer::~Framebuffer() {
    cleanup();
}

void Framebuffer::cleanup() {
    for (auto tex : m_ColorTextures) {
        if (tex) glDeleteTextures(1, &tex);
    }
    m_ColorTextures.clear();
    if (m_DepthTexture) { glDeleteTextures(1, &m_DepthTexture); m_DepthTexture = 0; }
    if (m_FBO) { glDeleteFramebuffers(1, &m_FBO); m_FBO = 0; }
}

void Framebuffer::Create(int width, int height, GLenum internalFormat, bool hasDepth, int numColorAttachments) {
    m_Width = width;
    m_Height = height;
    m_InternalFormat = internalFormat;
    m_HasDepth = hasDepth;
    m_NumColorAttachments = numColorAttachments;

    glCreateFramebuffers(1, &m_FBO);
    createTextures();

    GLenum status = glCheckNamedFramebufferStatus(m_FBO, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Framebuffer not complete: ", status);
    }
}

void Framebuffer::createTextures() {
    m_ColorTextures.resize(m_NumColorAttachments);

    std::vector<GLenum> attachments;
    for (int i = 0; i < m_NumColorAttachments; i++) {
        glCreateTextures(GL_TEXTURE_2D, 1, &m_ColorTextures[i]);
        glTextureStorage2D(m_ColorTextures[i], 1, m_InternalFormat, m_Width, m_Height);
        glTextureParameteri(m_ColorTextures[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_ColorTextures[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_ColorTextures[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_ColorTextures[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glNamedFramebufferTexture(m_FBO, GL_COLOR_ATTACHMENT0 + i, m_ColorTextures[i], 0);
        attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
    }
    glNamedFramebufferDrawBuffers(m_FBO, attachments.size(), attachments.data());

    if (m_HasDepth) {
        glCreateTextures(GL_TEXTURE_2D, 1, &m_DepthTexture);
        glTextureStorage2D(m_DepthTexture, 1, GL_DEPTH_COMPONENT24, m_Width, m_Height);
        glTextureParameteri(m_DepthTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(m_DepthTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glNamedFramebufferTexture(m_FBO, GL_DEPTH_ATTACHMENT, m_DepthTexture, 0);
    }
}

void Framebuffer::Resize(int width, int height) {
    if (width == m_Width && height == m_Height) return;
    cleanup();
    m_Width = width;
    m_Height = height;
    glCreateFramebuffers(1, &m_FBO);
    createTextures();
}

void Framebuffer::Bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glViewport(0, 0, m_Width, m_Height);
}

void Framebuffer::Unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint Framebuffer::GetColorTexture(int index) const {
    if (index < (int)m_ColorTextures.size()) return m_ColorTextures[index];
    return 0;
}
