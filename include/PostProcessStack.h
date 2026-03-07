#pragma once
#ifndef MIST_POST_PROCESS_STACK_H
#define MIST_POST_PROCESS_STACK_H

#include "Framebuffer.h"
#include "BloomRenderer.h"
#include "SSAORenderer.h"
#include "TAARenderer.h"
#include "SSGIRenderer.h"
#include "Shader.h"
#include <glm/glm.hpp>

class PostProcessStack {
public:
    PostProcessStack() = default;
    ~PostProcessStack();

    void Init(int width, int height);
    void Resize(int width, int height);

    void BeginSceneCapture();
    void EndSceneCapture();
    void Execute(float exposure, const glm::mat4& projection, const glm::mat4& view);

    GLuint GetHDRTexture() const { return m_HDRFramebuffer.GetColorTexture(); }
    GLuint GetDepthTexture() const { return m_HDRFramebuffer.GetDepthTexture(); }
    GLuint GetFullscreenVAO() const { return m_FullscreenVAO; }

    BloomRenderer bloom;
    SSAORenderer ssao;
    TAARenderer taa;
    SSGIRenderer ssgi;

    bool enableBloom = true;
    bool enableSSAO = true;
    bool enableFXAA = true;
    bool enableTAA = false;   // Off by default, user enables
    bool enableSSGI = false;  // Off by default, user enables

private:
    Framebuffer m_HDRFramebuffer;
    Framebuffer m_IntermediateFBO; // for ping-pong
    Shader m_ToneMapShader;
    Shader m_FXAAShader;
    Shader m_CompositeShader;

    GLuint m_FullscreenVAO = 0;
    int m_Width = 0, m_Height = 0;

    void setupFullscreenTriangle();
};

#endif
