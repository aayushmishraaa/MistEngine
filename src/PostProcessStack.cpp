#include "PostProcessStack.h"
#include "Core/Logger.h"

PostProcessStack::~PostProcessStack() {
    if (m_FullscreenVAO) glDeleteVertexArrays(1, &m_FullscreenVAO);
}

void PostProcessStack::Init(int width, int height) {
    m_Width = width;
    m_Height = height;

    m_HDRFramebuffer.Create(width, height, GL_RGBA16F, true, 1);
    m_IntermediateFBO.Create(width, height, GL_RGBA16F, false, 1);

    m_ToneMapShader = Shader("shaders/tonemap.vert", "shaders/tonemap.frag");
    m_FXAAShader = Shader("shaders/tonemap.vert", "shaders/fxaa.frag");
    m_CompositeShader = Shader("shaders/tonemap.vert", "shaders/bloom_composite.frag");

    bloom.Init(width, height);
    ssao.Init(width, height);
    taa.Init(width, height);
    ssgi.Init(width, height);

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
    bloom.Resize(width, height);
    ssao.Resize(width, height);
    taa.Resize(width, height);
    ssgi.Resize(width, height);
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

void PostProcessStack::Execute(float exposure, const glm::mat4& projection, const glm::mat4& view) {
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

    // 4. TAA resolve (before tone mapping, in linear HDR space)
    if (enableTAA && taa.enabled) {
        taa.Resolve(currentTexture, m_FullscreenVAO);
        currentTexture = taa.GetResolvedTexture();
        taa.NextFrame();
        glBindVertexArray(m_FullscreenVAO);
        glDisable(GL_DEPTH_TEST);
    }

    // 5. Tone mapping + gamma -> screen (or intermediate if FXAA)
    bool applyFXAA = enableFXAA && !enableTAA; // TAA replaces FXAA when active
    if (applyFXAA) {
        // Tone map to intermediate
        m_IntermediateFBO.Bind();
        glClear(GL_COLOR_BUFFER_BIT);
    } else {
        // Tone map directly to screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, m_Width, m_Height);
    }

    m_ToneMapShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, currentTexture);
    m_ToneMapShader.setInt("hdrBuffer", 0);
    m_ToneMapShader.setFloat("exposure", exposure);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    if (applyFXAA) {
        m_IntermediateFBO.Unbind();

        // 6. FXAA -> screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, m_Width, m_Height);
        glClear(GL_COLOR_BUFFER_BIT);

        m_FXAAShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_IntermediateFBO.GetColorTexture());
        m_FXAAShader.setInt("screenTexture", 0);
        m_FXAAShader.setVec2("inverseScreenSize", glm::vec2(1.0f / m_Width, 1.0f / m_Height));
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}
