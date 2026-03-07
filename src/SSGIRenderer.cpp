#include "SSGIRenderer.h"
#include "Core/Logger.h"

void SSGIRenderer::Init(int width, int height) {
    m_Width = width;
    m_Height = height;

    // Half-resolution for performance
    int halfW = width / 2;
    int halfH = height / 2;

    m_GIFBO.Create(halfW, halfH, GL_RGBA16F, false, 1);
    m_BlurFBO[0].Create(halfW, halfH, GL_RGBA16F, false, 1);
    m_BlurFBO[1].Create(halfW, halfH, GL_RGBA16F, false, 1);

    m_SSGIShader = Shader("shaders/tonemap.vert", "shaders/ssgi.frag");
    m_BlurShader = Shader("shaders/tonemap.vert", "shaders/ssgi_blur.frag");

    if (m_SSGIShader.ID == 0)
        LOG_ERROR("SSGI shader FAILED to compile");
    if (m_BlurShader.ID == 0)
        LOG_ERROR("SSGI blur shader FAILED to compile");

    LOG_INFO("SSGI initialized at half-res: ", halfW, "x", halfH);
}

void SSGIRenderer::Resize(int width, int height) {
    if (width == m_Width && height == m_Height) return;
    m_Width = width;
    m_Height = height;

    int halfW = width / 2;
    int halfH = height / 2;

    m_GIFBO.Resize(halfW, halfH);
    m_BlurFBO[0].Resize(halfW, halfH);
    m_BlurFBO[1].Resize(halfW, halfH);
}

void SSGIRenderer::Render(GLuint depthTexture, GLuint colorTexture,
                           const glm::mat4& projection, const glm::mat4& view,
                           GLuint fullscreenVAO) {
    if (!enabled) return;

    int halfW = m_Width / 2;
    int halfH = m_Height / 2;

    glm::mat4 invProjection = glm::inverse(projection);
    glm::mat4 invView = glm::inverse(view);

    glBindVertexArray(fullscreenVAO);
    glDisable(GL_DEPTH_TEST);

    // Pass 1: SSGI computation at half-res
    m_GIFBO.Bind();
    glViewport(0, 0, halfW, halfH);
    glClear(GL_COLOR_BUFFER_BIT);

    m_SSGIShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    m_SSGIShader.setInt("depthTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    m_SSGIShader.setInt("colorTexture", 1);

    m_SSGIShader.setMat4("projection", projection);
    m_SSGIShader.setMat4("view", view);
    m_SSGIShader.setMat4("invProjection", invProjection);
    m_SSGIShader.setMat4("invView", invView);
    m_SSGIShader.setFloat("radius", radius);
    m_SSGIShader.setFloat("intensity", intensity);
    m_SSGIShader.setVec2("noiseScale", glm::vec2(halfW / 4.0f, halfH / 4.0f));

    glDrawArrays(GL_TRIANGLES, 0, 3);
    m_GIFBO.Unbind();

    // Pass 2: Bilateral blur (horizontal)
    m_BlurFBO[0].Bind();
    glViewport(0, 0, halfW, halfH);
    glClear(GL_COLOR_BUFFER_BIT);

    m_BlurShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_GIFBO.GetColorTexture());
    m_BlurShader.setInt("ssgiTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    m_BlurShader.setInt("depthTexture", 1);

    m_BlurShader.setVec2("direction", glm::vec2(1.0f, 0.0f));

    glDrawArrays(GL_TRIANGLES, 0, 3);
    m_BlurFBO[0].Unbind();

    // Pass 3: Bilateral blur (vertical)
    m_BlurFBO[1].Bind();
    glViewport(0, 0, halfW, halfH);
    glClear(GL_COLOR_BUFFER_BIT);

    m_BlurShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_BlurFBO[0].GetColorTexture());
    m_BlurShader.setInt("ssgiTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    m_BlurShader.setInt("depthTexture", 1);

    m_BlurShader.setVec2("direction", glm::vec2(0.0f, 1.0f));

    glDrawArrays(GL_TRIANGLES, 0, 3);
    m_BlurFBO[1].Unbind();

    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}
