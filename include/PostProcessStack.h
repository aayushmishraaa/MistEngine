#pragma once
#ifndef MIST_POST_PROCESS_STACK_H
#define MIST_POST_PROCESS_STACK_H

#include "Framebuffer.h"
#include "BloomRenderer.h"
#include "SSAORenderer.h"
#include "TAARenderer.h"
#include "SSGIRenderer.h"
#include "SSRRenderer.h"
#include "Environment.h"
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
    // Every knob this stack used to own now lives on `env`. The pass order
    // is fixed; what each pass does with it is the scene's business.
    void Execute(const Environment& env, const glm::mat4& projection, const glm::mat4& view,
                 GLuint hiZTexture = 0);

    // Depth prepass — runs BEFORE scene capture. Writes depth to
    // m_PrepassFBO's depth attachment and packed normal+roughness to
    // its single RGBA16F color attachment. Consumers (TAA, Hi-Z)
    // sample the two textures via GetPrepassDepth / GetPrepassNormalRoughness.
    void BeginPrepass();
    void EndPrepass();
    GLuint GetPrepassDepth()            const { return m_PrepassFBO.GetDepthTexture(); }
    GLuint GetPrepassNormalRoughness()  const { return m_PrepassFBO.GetColorTexture(); }

    GLuint GetHDRTexture() const { return m_HDRFramebuffer.GetColorTexture(); }
    GLuint GetDepthTexture() const { return m_HDRFramebuffer.GetDepthTexture(); }

    // Final post-tonemap LDR texture. This is what the editor's Scene
    // View panel samples and what the fullscreen present path blits.
    // GetHDRTexture() returns the *pre-post-process* linear scene —
    // do not use it for presentation or you'll show raw HDR and every
    // post-effect (FXAA, bloom, DOF, motion blur, tonemap) becomes
    // invisible.
    GLuint GetPresentedTexture() const { return m_PresentFBO.GetColorTexture(); }
    const Framebuffer& GetPresentFramebuffer() const { return m_PresentFBO; }

    // Exposed so the Renderer's fullscreen-present path can glBlitFramebuffer
    // from the HDR FBO to the default framebuffer when the Scene View panel
    // is hidden.
    const Framebuffer& GetHDRFramebuffer() const { return m_HDRFramebuffer; }
    GLuint GetFullscreenVAO() const { return m_FullscreenVAO; }

    // Sub-renderers own GPU resources and pass logic. Their settings do not
    // live here — see Environment.
    BloomRenderer bloom;
    SSAORenderer ssao;
    TAARenderer taa;
    SSGIRenderer ssgi;
    SSRRenderer  ssr;

private:
    Framebuffer m_HDRFramebuffer;
    Framebuffer m_IntermediateFBO;   // bloom ping-pong + tonemap-to-LDR slot
    Framebuffer m_PresentFBO;        // tonemap output (LDR) — what editor Scene View + fullscreen present read
    Framebuffer m_FXAAIntermediate;  // FXAA-in-HDR output (pre-tonemap)
    Framebuffer m_MotionBlurFBO;     // motion blur output (HDR)
    Framebuffer m_DOFCoCFBO;         // full-res R16F CoC
    Framebuffer m_DOFBokehFBO;       // half-res HDR bokeh
    Framebuffer m_DOFCompositeFBO;   // full-res HDR DOF composite
    Framebuffer m_PrepassFBO;        // depth + packed normal/roughness
    Shader m_ToneMapShader;
    Shader m_FXAAShader;
    Shader m_CompositeShader;
    Shader m_MotionBlurShader;
    Shader m_DOFCoCShader;
    Shader m_DOFBokehShader;
    Shader m_DOFCompositeShader;

    GLuint m_FullscreenVAO = 0;
    int m_Width = 0, m_Height = 0;

    void setupFullscreenTriangle();
};

#endif
