#pragma once
#ifndef MIST_FRAMEBUFFER_H
#define MIST_FRAMEBUFFER_H

#include <glad/glad.h>
#include <vector>

#include "Renderer/RID.h"

class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer();

    void Create(int width, int height, GLenum internalFormat = GL_RGBA16F,
                bool hasDepth = true, int numColorAttachments = 1);
    void Resize(int width, int height);
    void Bind() const;
    void Unbind() const;

    GLuint GetColorTexture(int index = 0) const;
    GLuint GetDepthTexture() const { return m_DepthTexture; }
    GLuint GetFBO() const { return m_FBO; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    bool IsValid() const { return m_Valid; }

private:
    GLuint m_FBO = 0;

    // Texture *handles* stay cached alongside RIDs because downstream code
    // (shader samplers, post-process bind paths) still needs raw GLuints —
    // the bind/draw path hasn't been abstracted yet. Destruction routes
    // through the device so lifetimes stay backend-agnostic.
    std::vector<GLuint> m_ColorTextures;
    std::vector<RID>    m_ColorTextureRIDs;
    GLuint              m_DepthTexture = 0;
    RID                 m_DepthTextureRID{};

    int m_Width = 0, m_Height = 0;
    GLenum m_InternalFormat = GL_RGBA16F;
    bool m_HasDepth = true;
    int m_NumColorAttachments = 1;
    // False when the last Create/Resize left the FBO in an incomplete state.
    // Bind() short-circuits so callers don't silently render into a broken
    // attachment (which looks like "nothing draws" but actually produces
    // undefined results).
    bool m_Valid = false;

    void createTextures();
    void cleanup();
};

#endif // MIST_FRAMEBUFFER_H
