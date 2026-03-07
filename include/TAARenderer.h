#pragma once
#ifndef MIST_TAA_RENDERER_H
#define MIST_TAA_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"
#include "Framebuffer.h"

class TAARenderer {
public:
    TAARenderer() = default;
    ~TAARenderer();

    void Init(int width, int height);
    void Resize(int width, int height);

    // Get jitter offset for current frame (apply to projection matrix)
    glm::vec2 GetJitter() const { return m_Jitter; }
    glm::vec2 GetPreviousJitter() const { return m_PrevJitter; }

    // Advance to next jitter sample
    void NextFrame();

    // Render the velocity buffer pass (call before main scene render)
    GLuint GetVelocityFBO() const { return m_VelocityFBO.GetFBO(); }
    void BeginVelocityPass();
    void EndVelocityPass();

    // Resolve TAA: blend current + history using velocity
    void Resolve(GLuint currentFrameTexture, GLuint fullscreenVAO);

    // Get the resolved (anti-aliased) output texture
    GLuint GetResolvedTexture() const;
    GLuint GetVelocityTexture() const { return m_VelocityFBO.GetColorTexture(); }

    Shader& GetVelocityShader() { return m_VelocityShader; }

    bool enabled = true;

private:
    int m_Width = 0, m_Height = 0;

    // Halton sequence for sub-pixel jitter
    static constexpr int JITTER_SEQUENCE_LENGTH = 16;
    glm::vec2 m_JitterSequence[JITTER_SEQUENCE_LENGTH];
    int m_FrameIndex = 0;
    glm::vec2 m_Jitter = glm::vec2(0.0f);
    glm::vec2 m_PrevJitter = glm::vec2(0.0f);

    // Velocity buffer (RG16F: per-pixel screen-space motion)
    Framebuffer m_VelocityFBO;

    // Ping-pong history buffers
    Framebuffer m_HistoryFBO[2];
    int m_CurrentHistory = 0;

    // Shaders
    Shader m_VelocityShader;
    Shader m_TAAResolveShader;

    void generateHaltonSequence();
    static float halton(int index, int base);
};

#endif // MIST_TAA_RENDERER_H
