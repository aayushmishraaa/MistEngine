#pragma once
#ifndef MIST_SSGI_RENDERER_H
#define MIST_SSGI_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"
#include "Framebuffer.h"

class SSGIRenderer {
public:
    SSGIRenderer() = default;
    ~SSGIRenderer() = default;

    void Init(int width, int height);
    void Resize(int width, int height);

    // Render SSGI from depth + color buffers, returns GI texture
    void Render(GLuint depthTexture, GLuint colorTexture,
                const glm::mat4& projection, const glm::mat4& view,
                GLuint fullscreenVAO);

    GLuint GetGITexture() const { return m_BlurFBO[1].GetColorTexture(); }

    bool enabled = false;
    float radius = 3.0f;
    float intensity = 1.0f;

private:
    int m_Width = 0, m_Height = 0;

    // Half-resolution GI buffer
    Framebuffer m_GIFBO;
    Framebuffer m_BlurFBO[2]; // ping-pong for bilateral blur

    Shader m_SSGIShader;
    Shader m_BlurShader;
};

#endif // MIST_SSGI_RENDERER_H
