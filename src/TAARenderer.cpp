#include "TAARenderer.h"
#include "Core/Logger.h"

TAARenderer::~TAARenderer() {
}

float TAARenderer::halton(int index, int base) {
    float result = 0.0f;
    float f = 1.0f / static_cast<float>(base);
    int i = index;
    while (i > 0) {
        result += f * (i % base);
        i /= base;
        f /= static_cast<float>(base);
    }
    return result;
}

void TAARenderer::generateHaltonSequence() {
    for (int i = 0; i < JITTER_SEQUENCE_LENGTH; i++) {
        m_JitterSequence[i] = glm::vec2(
            halton(i + 1, 2) - 0.5f,
            halton(i + 1, 3) - 0.5f
        );
    }
}

void TAARenderer::Init(int width, int height) {
    m_Width = width;
    m_Height = height;

    generateHaltonSequence();

    // Velocity buffer: RG16F for motion vectors
    m_VelocityFBO.Create(width, height, GL_RG16F, true, 1);

    // History buffers: full HDR
    m_HistoryFBO[0].Create(width, height, GL_RGBA16F, false, 1);
    m_HistoryFBO[1].Create(width, height, GL_RGBA16F, false, 1);

    // Shaders
    m_VelocityShader = Shader("shaders/velocity.vert", "shaders/velocity.frag");
    m_TAAResolveShader = Shader("shaders/tonemap.vert", "shaders/taa.frag");

    if (m_VelocityShader.ID == 0)
        LOG_ERROR("TAA velocity shader FAILED to compile");
    if (m_TAAResolveShader.ID == 0)
        LOG_ERROR("TAA resolve shader FAILED to compile");

    LOG_INFO("TAA initialized: ", width, "x", height, " with ", JITTER_SEQUENCE_LENGTH, "-sample Halton sequence");
}

void TAARenderer::Resize(int width, int height) {
    if (width == m_Width && height == m_Height) return;
    m_Width = width;
    m_Height = height;

    m_VelocityFBO.Resize(width, height);
    m_HistoryFBO[0].Resize(width, height);
    m_HistoryFBO[1].Resize(width, height);
}

void TAARenderer::NextFrame() {
    m_PrevJitter = m_Jitter;
    m_FrameIndex = (m_FrameIndex + 1) % JITTER_SEQUENCE_LENGTH;

    if (enabled) {
        // Jitter in pixel units, will be converted to clip space by caller
        m_Jitter = m_JitterSequence[m_FrameIndex];
    } else {
        m_Jitter = glm::vec2(0.0f);
    }
}

void TAARenderer::BeginVelocityPass() {
    m_VelocityFBO.Bind();
    glViewport(0, 0, m_Width, m_Height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void TAARenderer::EndVelocityPass() {
    m_VelocityFBO.Unbind();
}

void TAARenderer::Resolve(GLuint currentFrameTexture, GLuint fullscreenVAO) {
    int outputIdx = m_CurrentHistory;
    int historyIdx = 1 - m_CurrentHistory;

    // Write to current output
    m_HistoryFBO[outputIdx].Bind();
    glViewport(0, 0, m_Width, m_Height);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_TAAResolveShader.use();

    // Current frame
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, currentFrameTexture);
    m_TAAResolveShader.setInt("currentFrame", 0);

    // History frame
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_HistoryFBO[historyIdx].GetColorTexture());
    m_TAAResolveShader.setInt("historyFrame", 1);

    // Velocity buffer
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_VelocityFBO.GetColorTexture());
    m_TAAResolveShader.setInt("velocityBuffer", 2);

    m_TAAResolveShader.setVec2("inverseScreenSize", glm::vec2(1.0f / m_Width, 1.0f / m_Height));

    glBindVertexArray(fullscreenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    m_HistoryFBO[outputIdx].Unbind();
    glEnable(GL_DEPTH_TEST);

    // Swap ping-pong
    m_CurrentHistory = historyIdx;
}

GLuint TAARenderer::GetResolvedTexture() const {
    // The most recently written buffer
    int outputIdx = 1 - m_CurrentHistory;
    return m_HistoryFBO[outputIdx].GetColorTexture();
}
