#include "PostProcessStack.h"
#include "Core/Logger.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>

PostProcessStack::~PostProcessStack() {
    if (m_FullscreenVAO) glDeleteVertexArrays(1, &m_FullscreenVAO);
}

void PostProcessStack::Init(int width, int height) {
    m_Width = width;
    m_Height = height;

    m_HDRFramebuffer.Create(width, height, GL_RGBA16F, true, 1);
    m_IntermediateFBO.Create(width, height, GL_RGBA16F, false, 1);
    // FXAA runs on HDR (pre-tonemap) now, so its output needs to be
    // RGBA16F too. Separate FBO so we don't ping-pong over the same
    // buffer bloom is compositing into.
    m_FXAAIntermediate.Create(width, height, GL_RGBA16F, false, 1);
    m_MotionBlurFBO.Create(width, height, GL_RGBA16F, false, 1);
    // DOF: full-res R16F CoC, half-res RGBA16F bokeh, full-res composite.
    m_DOFCoCFBO.Create(width, height, GL_R16F, false, 1);
    m_DOFBokehFBO.Create(std::max(1, width / 2), std::max(1, height / 2),
                         GL_RGBA16F, false, 1);
    m_DOFCompositeFBO.Create(width, height, GL_RGBA16F, false, 1);
    // Prepass FBO — depth + one RGBA16F color attachment for packed
    // normal+roughness. Kept separate from m_HDRFramebuffer so the
    // main pass can still own its own depth (early-Z sharing is a
    // follow-up; this cycle just produces the data).
    m_PrepassFBO.Create(width, height, GL_RGBA16F, true, 1);

    m_ToneMapShader    = Shader("shaders/tonemap.vert", "shaders/tonemap.frag");
    m_FXAAShader       = Shader("shaders/tonemap.vert", "shaders/fxaa.frag");
    m_CompositeShader  = Shader("shaders/tonemap.vert", "shaders/bloom_composite.frag");
    m_MotionBlurShader   = Shader("shaders/tonemap.vert", "shaders/motion_blur.frag");
    m_DOFCoCShader       = Shader("shaders/tonemap.vert", "shaders/dof_coc.frag");
    m_DOFBokehShader     = Shader("shaders/tonemap.vert", "shaders/dof_bokeh.frag");
    m_DOFCompositeShader = Shader("shaders/tonemap.vert", "shaders/dof_composite.frag");

    bloom.Init(width, height);
    ssao.Init(width, height);
    taa.Init(width, height);
    ssgi.Init(width, height);
    ssr.Init(width, height);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        LOG_ERROR("GL error after TAA/SSGI init: 0x", std::hex, err);
    }

    setupFullscreenTriangle();

    LOG_INFO("PostProcessStack initialized: ", width, "x", height, " (TAA + SSGI ready)");
}

void PostProcessStack::Resize(int width, int height) {
    if (width == m_Width && height == m_Height) return;
    m_Width = width;
    m_Height = height;
    m_HDRFramebuffer.Resize(width, height);
    m_IntermediateFBO.Resize(width, height);
    m_FXAAIntermediate.Resize(width, height);
    m_MotionBlurFBO.Resize(width, height);
    m_DOFCoCFBO.Resize(width, height);
    m_DOFBokehFBO.Resize(std::max(1, width / 2), std::max(1, height / 2));
    m_DOFCompositeFBO.Resize(width, height);
    m_PrepassFBO.Resize(width, height);
    bloom.Resize(width, height);
    ssao.Resize(width, height);
    taa.Resize(width, height);
    ssgi.Resize(width, height);
    ssr.Resize(width, height);
}

void PostProcessStack::setupFullscreenTriangle() {
    glCreateVertexArrays(1, &m_FullscreenVAO);
}

void PostProcessStack::BeginSceneCapture() {
    m_HDRFramebuffer.Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessStack::EndSceneCapture() {
    m_HDRFramebuffer.Unbind();
}

void PostProcessStack::BeginPrepass() {
    m_PrepassFBO.Bind();
    glViewport(0, 0, m_Width, m_Height);
    // Clear color to (0,0,0,0) = encoded-normal at +Z hemisphere
    // origin with zero roughness; depth to 1.0 (far). Any fragment
    // the prepass draws overwrites both channels.
    glClearColor(0.5f, 0.5f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessStack::EndPrepass() {
    m_PrepassFBO.Unbind();
}

void PostProcessStack::Execute(float exposure, const glm::mat4& projection, const glm::mat4& view,
                               GLuint hiZTexture) {
    glBindVertexArray(m_FullscreenVAO);
    glDisable(GL_DEPTH_TEST);

    GLuint currentTexture = m_HDRFramebuffer.GetColorTexture();

    // 1. SSAO
    if (enableSSAO && ssao.enabled) {
        ssao.Render(m_HDRFramebuffer.GetDepthTexture(), projection, view);
    }

    // 2. SSGI (screen-space global illumination)
    if (enableSSGI && ssgi.enabled) {
        ssgi.Render(m_HDRFramebuffer.GetDepthTexture(), currentTexture,
                    projection, view, m_FullscreenVAO);
        // SSGI output is available via ssgi.GetGITexture() for compositing in PBR shader
        glBindVertexArray(m_FullscreenVAO);
        glDisable(GL_DEPTH_TEST);
    }

    // 3. Bloom
    if (enableBloom && bloom.enabled) {
        bloom.RenderBloom(currentTexture, bloom.threshold, bloom.intensity);

        // Composite bloom onto scene
        m_IntermediateFBO.Bind();
        glClear(GL_COLOR_BUFFER_BIT);
        m_CompositeShader.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTexture);
        m_CompositeShader.setInt("sceneTexture", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bloom.GetBloomTexture());
        m_CompositeShader.setInt("bloomTexture", 1);
        m_CompositeShader.setFloat("bloomStrength", bloom.intensity);

        glDrawArrays(GL_TRIANGLES, 0, 3);
        m_IntermediateFBO.Unbind();

        currentTexture = m_IntermediateFBO.GetColorTexture();
    }

    // 4. TAA resolve (before FXAA, still in linear HDR)
    if (enableTAA && taa.enabled) {
        // Prepass depth feeds the closest-depth velocity pick — passing
        // zero makes the resolve shader fall back to central-pixel
        // sampling, which is still correct, just less crisp on edges.
        taa.Resolve(currentTexture, m_PrepassFBO.GetDepthTexture(), m_FullscreenVAO);
        currentTexture = taa.GetResolvedTexture();
        taa.NextFrame();
        glBindVertexArray(m_FullscreenVAO);
        glDisable(GL_DEPTH_TEST);
    }

    // 4.5. SSR — Hi-Z hierarchical ray march. Reads the post-TAA
    //      HDR + prepass material buffers + Hi-Z, composites a
    //      reflection term back into HDR. Gated on Hi-Z availability
    //      (hiZTexture != 0) so callers without prepass wiring don't
    //      get garbage.
    if (enableSSR && ssr.enabled && hiZTexture != 0) {
        ssr.Render(currentTexture,
                   m_PrepassFBO.GetDepthTexture(),
                   m_PrepassFBO.GetColorTexture(),
                   hiZTexture,
                   view,
                   projection);
        currentTexture = ssr.GetOutputTexture();
        glBindVertexArray(m_FullscreenVAO);
        glDisable(GL_DEPTH_TEST);
    }

    // 4.7. Bokeh DOF (optional) — three sub-passes: CoC, half-res
    //      bokeh, full-res composite. Physically the lens effect
    //      (defocus) happens before the shutter integration (motion
    //      blur), so DOF runs first in the chain.
    if (enableDOF) {
        // CoC pass — full-res R16F, signed blur radius.
        m_DOFCoCFBO.Bind();
        glClear(GL_COLOR_BUFFER_BIT);
        m_DOFCoCShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_PrepassFBO.GetDepthTexture());
        m_DOFCoCShader.setInt("depthTex", 0);
        m_DOFCoCShader.setMat4("uInvProj", glm::inverse(projection));
        m_DOFCoCShader.setFloat("uFocusDistance", dofFocusDistance);
        m_DOFCoCShader.setFloat("uAperture",      dofAperture);
        m_DOFCoCShader.setFloat("uMaxRadius",     dofMaxRadius);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        m_DOFCoCFBO.Unbind();

        // Bokeh pass — half-res.
        int halfW = std::max(1, m_Width  / 2);
        int halfH = std::max(1, m_Height / 2);
        m_DOFBokehFBO.Bind();
        glViewport(0, 0, halfW, halfH);
        glClear(GL_COLOR_BUFFER_BIT);
        m_DOFBokehShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTexture);
        m_DOFBokehShader.setInt("sceneHDR", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_DOFCoCFBO.GetColorTexture());
        m_DOFBokehShader.setInt("cocTex", 1);
        m_DOFBokehShader.setVec2("uHalfResInvSize",
            glm::vec2(1.0f / (float)halfW, 1.0f / (float)halfH));
        glDrawArrays(GL_TRIANGLES, 0, 3);
        m_DOFBokehFBO.Unbind();

        // Composite — full-res, lerps sharp <-> bokeh by CoC.
        m_DOFCompositeFBO.Bind();
        glViewport(0, 0, m_Width, m_Height);
        glClear(GL_COLOR_BUFFER_BIT);
        m_DOFCompositeShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTexture);
        m_DOFCompositeShader.setInt("sharpHDR", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_DOFBokehFBO.GetColorTexture());
        m_DOFCompositeShader.setInt("bokehHDR", 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_DOFCoCFBO.GetColorTexture());
        m_DOFCompositeShader.setInt("cocTex", 2);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        m_DOFCompositeFBO.Unbind();
        currentTexture = m_DOFCompositeFBO.GetColorTexture();
    }

    // 4.8. Motion blur (optional) — samples HDR along the velocity
    //      vector. Uses the velocity buffer whether or not TAA is
    //      active (velocity pass is decoupled from TAA in Renderer).
    if (enableMotionBlur) {
        m_MotionBlurFBO.Bind();
        glClear(GL_COLOR_BUFFER_BIT);
        m_MotionBlurShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTexture);
        m_MotionBlurShader.setInt("sceneHDR", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, taa.GetVelocityTexture());
        m_MotionBlurShader.setInt("velocityBuffer", 1);
        m_MotionBlurShader.setFloat("uStrength", motionBlurStrength);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        m_MotionBlurFBO.Unbind();
        currentTexture = m_MotionBlurFBO.GetColorTexture();
    }

    // 5. FXAA in HDR (pre-tonemap). Running before tonemap means
    //    specular highlights get anti-aliased in linear space before
    //    the tonemap operator hard-clips them; the old post-tonemap
    //    FXAA couldn't fix the aliasing that tonemap baked in.
    //    Gated off when TAA is active to avoid compound blurring.
    bool applyFXAA = enableFXAA && !enableTAA;
    if (applyFXAA) {
        m_FXAAIntermediate.Bind();
        glClear(GL_COLOR_BUFFER_BIT);
        m_FXAAShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTexture);
        m_FXAAShader.setInt("screenTexture", 0);
        m_FXAAShader.setVec2("inverseScreenSize", glm::vec2(1.0f / m_Width, 1.0f / m_Height));
        glDrawArrays(GL_TRIANGLES, 0, 3);
        m_FXAAIntermediate.Unbind();
        currentTexture = m_FXAAIntermediate.GetColorTexture();
    }

    // 6. Tone mapping + gamma -> screen. Reads FXAA-smoothed HDR if
    //    FXAA ran, otherwise reads post-bloom/TAA HDR directly.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_Width, m_Height);

    m_ToneMapShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, currentTexture);
    m_ToneMapShader.setInt("hdrBuffer", 0);
    m_ToneMapShader.setFloat("exposure", exposure);
    {
        // uTonemapOp is an int uniform in the shader; Shader helper
        // sends floats so go raw here.
        GLint loc = glGetUniformLocation(m_ToneMapShader.ID, "uTonemapOp");
        if (loc >= 0) glUniform1i(loc, tonemapOperator);
    }
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}
