#pragma once
#ifndef MIST_FRAMEBUFFER_H
#define MIST_FRAMEBUFFER_H

#include <glad/glad.h>
#include <vector>

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

private:
    GLuint m_FBO = 0;
    std::vector<GLuint> m_ColorTextures;
    GLuint m_DepthTexture = 0;
    int m_Width = 0, m_Height = 0;
    GLenum m_InternalFormat = GL_RGBA16F;
    bool m_HasDepth = true;
    int m_NumColorAttachments = 1;

    void createTextures();
    void cleanup();
};

#endif // MIST_FRAMEBUFFER_H
