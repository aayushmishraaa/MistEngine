#pragma once
#ifndef MIST_POST_PROCESS_STACK_H
#define MIST_POST_PROCESS_STACK_H

#include "Framebuffer.h"
#include "BloomRenderer.h"
#include "SSAORenderer.h"
#include "TAARenderer.h"
#include "SSGIRenderer.h"
#include "SSRRenderer.h"
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
    void Execute(float exposure, const glm::mat4& projection, const glm::mat4& view,
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

    // Exposed so the Renderer's fullscreen-present path can glBlitFramebuffer
    // from the HDR FBO to the default framebuffer when the Scene View panel
    // is hidden.
    const Framebuffer& GetHDRFramebuffer() const { return m_HDRFramebuffer; }
    GLuint GetFullscreenVAO() const { return m_FullscreenVAO; }

    BloomRenderer bloom;
    SSAORenderer ssao;
    TAARenderer taa;
    SSGIRenderer ssgi;
    SSRRenderer  ssr;

    bool enableBloom = true;
    bool enableSSAO = true;
    bool enableFXAA = true;
    // Tonemap operator: 0 = ACES, 1 = Reinhard, 2 = AgX (default).
    // Matches Godot 4.3's flip to AgX. View menu selector wires to
    // this field directly.
    int  tonemapOperator = 2;

    // PCSS soft shadow controls (Phase I). softness = Godot's "light
    // angular size" surrogate — scales penumbra search + PCF kernel.
    // 0 disables PCSS (hard shadows). Quality 0 = 4/16 taps, 1 = 8/32.
    float shadowSoftness = 1.0f;
    int   shadowQuality  = 0;
    bool enableTAA  = false;  // Off by default, user enables
    bool enableSSGI = false;  // Off by default, user enables
    bool enableSSR  = true;   // On by default — visible win on ground reflections
    bool  enableMotionBlur     = false;
    float motionBlurStrength   = 0.5f;

    // Bokeh DOF — off by default since a good focus-target UX is
    // future work. Users can still enable from the post-process panel.
    bool  enableDOF        = false;
    float dofFocusDistance = 10.0f;   // view-space forward distance
    float dofAperture      = 0.15f;   // scales CoC — 0.15 matches a
                                      // moderate cinematic effect at f/2.8-ish
    float dofMaxRadius     = 10.0f;   // max blur radius in pixels

private:
    Framebuffer m_HDRFramebuffer;
    Framebuffer m_IntermediateFBO;   // bloom ping-pong + tonemap-to-LDR slot
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
