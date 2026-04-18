#include "Framebuffer.h"
#include "Core/Logger.h"
#include "Renderer/RenderingDevice.h"
#include "Renderer/GLRenderingDevice.h"

namespace {

// Framebuffer's public API still accepts raw GLenum internal formats
// (GL_RGBA16F, GL_R8, GL_RG16F, etc.). Migrating every call site to the
// backend-agnostic enum is a follow-up cycle; for now this adapter maps
// the ones we actually use. Unknown formats fall back to RGBA8 with a
// warning so regressions are visible instead of silent.
using Mist::GPU::TextureFormat;

TextureFormat fromGLInternalFormat(GLenum gl) {
    switch (gl) {
        case GL_RGBA8:             return TextureFormat::RGBA8;
        case GL_RGBA16F:           return TextureFormat::RGBA16F;
        case GL_R8:                return TextureFormat::R8;
        case GL_RG16F:             return TextureFormat::RG16F;
        case GL_R16F:              return TextureFormat::R16F;
        case GL_RGB16F:            return TextureFormat::RGB16F;
        case GL_DEPTH_COMPONENT24: return TextureFormat::DEPTH24;
        default:
            LOG_WARN("Framebuffer: unmapped GLenum format 0x",
                     std::hex, gl, ", defaulting to RGBA8");
            return TextureFormat::RGBA8;
    }
}

} // namespace

Framebuffer::~Framebuffer() {
    cleanup();
}

void Framebuffer::cleanup() {
    auto* dev = static_cast<Mist::GPU::GLRenderingDevice*>(Mist::GPU::Device());
    for (auto rid : m_ColorTextureRIDs) {
        if (dev) dev->Destroy(rid);
    }
    m_ColorTextures.clear();
    m_ColorTextureRIDs.clear();

    if (m_DepthTextureRID.IsValid() && dev) dev->Destroy(m_DepthTextureRID);
    m_DepthTextureRID = {};
    m_DepthTexture    = 0;

    if (m_FBO) { glDeleteFramebuffers(1, &m_FBO); m_FBO = 0; }
    m_Valid = false;
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
    m_Valid = (status == GL_FRAMEBUFFER_COMPLETE);
    if (!m_Valid) {
        LOG_ERROR("Framebuffer not complete: ", status);
        cleanup();
    }
}

void Framebuffer::createTextures() {
    auto* dev = static_cast<Mist::GPU::GLRenderingDevice*>(Mist::GPU::Device());
    if (!dev) {
        LOG_ERROR("Framebuffer: no active RenderingDevice — "
                  "Renderer::Init must run before Framebuffer::Create");
        m_Valid = false;
        return;
    }

    m_ColorTextures.resize(m_NumColorAttachments);
    m_ColorTextureRIDs.resize(m_NumColorAttachments);

    std::vector<GLenum> attachments;
    for (int i = 0; i < m_NumColorAttachments; i++) {
        Mist::GPU::TextureDesc desc{};
        desc.width   = static_cast<std::uint32_t>(m_Width);
        desc.height  = static_cast<std::uint32_t>(m_Height);
        desc.format  = fromGLInternalFormat(m_InternalFormat);
        desc.mipmaps = false;
        m_ColorTextureRIDs[i] = dev->CreateTexture(desc);
        m_ColorTextures[i]    = dev->GetGLHandle(m_ColorTextureRIDs[i]);

        glTextureParameteri(m_ColorTextures[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_ColorTextures[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_ColorTextures[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_ColorTextures[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glNamedFramebufferTexture(m_FBO, GL_COLOR_ATTACHMENT0 + i, m_ColorTextures[i], 0);
        attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
    }
    glNamedFramebufferDrawBuffers(m_FBO, attachments.size(), attachments.data());

    if (m_HasDepth) {
        Mist::GPU::TextureDesc desc{};
        desc.width  = static_cast<std::uint32_t>(m_Width);
        desc.height = static_cast<std::uint32_t>(m_Height);
        desc.format = Mist::GPU::TextureFormat::DEPTH24;
        m_DepthTextureRID = dev->CreateTexture(desc);
        m_DepthTexture    = dev->GetGLHandle(m_DepthTextureRID);

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

    GLenum status = glCheckNamedFramebufferStatus(m_FBO, GL_FRAMEBUFFER);
    m_Valid = (status == GL_FRAMEBUFFER_COMPLETE);
    if (!m_Valid) {
        LOG_ERROR("Framebuffer resize incomplete: ", status);
        cleanup();
    }
}

void Framebuffer::Bind() const {
    if (!m_Valid) return;
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
