#include "SSRRenderer.h"
#include "Core/Logger.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

SSRRenderer::~SSRRenderer() {
    destroyOutput();
}

void SSRRenderer::Init(int width, int height) {
    m_Width  = width;
    m_Height = height;
    allocateOutput();

    m_Shader = Shader("shaders/ssr.comp");
    if (!m_Shader.isValid()) {
        LOG_ERROR("SSRRenderer: ssr.comp failed to compile");
        return;
    }
    m_Initialized = true;
    LOG_INFO("SSRRenderer initialized: ", width, "x", height);
}

void SSRRenderer::Resize(int width, int height) {
    if (width == m_Width && height == m_Height) return;
    destroyOutput();
    m_Width  = width;
    m_Height = height;
    allocateOutput();
}

void SSRRenderer::allocateOutput() {
    glCreateTextures(GL_TEXTURE_2D, 1, &m_OutputTex);
    glTextureStorage2D(m_OutputTex, 1, GL_RGBA16F, m_Width, m_Height);
    glTextureParameteri(m_OutputTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_OutputTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_OutputTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_OutputTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void SSRRenderer::destroyOutput() {
    if (m_OutputTex) {
        glDeleteTextures(1, &m_OutputTex);
        m_OutputTex = 0;
    }
    m_Initialized = false;
}

void SSRRenderer::Render(GLuint hdrColor,
                         GLuint prepassDepth,
                         GLuint prepassNormalRoughness,
                         GLuint hiZ,
                         const glm::mat4& view,
                         const glm::mat4& proj) {
    if (!m_Initialized) return;

    m_Shader.use();

    // Texture unit bindings match layout(binding=N) in ssr.comp.
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, hdrColor);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, prepassDepth);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, prepassNormalRoughness);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, hiZ);

    // Output image bound at unit 0 (writeonly image2D in shader).
    glBindImageTexture(0, m_OutputTex, 0, GL_FALSE, 0,
                       GL_WRITE_ONLY, GL_RGBA16F);

    // Matrices. glm stores column-major; Shader::setMat4 already
    // uploads without transpose, matching GLSL's default layout.
    glm::mat4 invProj = glm::inverse(proj);
    m_Shader.setMat4("uInvProj", invProj);
    m_Shader.setMat4("uProj",    proj);
    m_Shader.setMat4("uView",    view);

    // ivec2 screen size — Shader helper only sends floats, bind the
    // int uniform directly.
    GLint locScr = glGetUniformLocation(m_Shader.ID, "uScreenSize");
    if (locScr >= 0) glUniform2i(locScr, m_Width, m_Height);

    m_Shader.setFloat("uRoughnessCutoff", roughnessCutoff);
    m_Shader.setFloat("uMaxDistance",     maxDistance);

    GLint locSteps = glGetUniformLocation(m_Shader.ID, "uMaxSteps");
    if (locSteps >= 0) glUniform1i(locSteps, maxSteps);

    GLuint gx = (GLuint)((m_Width  + 7) / 8);
    GLuint gy = (GLuint)((m_Height + 7) / 8);
    glDispatchCompute(gx, gy, 1);

    // Make the image write visible to subsequent fragment samplers.
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT
                  | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
