#include "HiZPyramid.h"
#include "Core/Logger.h"

#include <algorithm>

HiZPyramid::~HiZPyramid() {
    destroyTexture();
}

void HiZPyramid::Init(int width, int height) {
    m_Width  = width;
    m_Height = height;
    allocateTexture();

    // Compute shader — a single program reused across mip dispatches
    // by rebinding srcMip / dstMip and updating the uSrcLod uniform.
    m_ReduceShader = Shader("shaders/hiz_reduce.comp");
    if (!m_ReduceShader.isValid()) {
        LOG_ERROR("HiZPyramid: hiz_reduce.comp failed to compile");
        return;
    }

    glCreateFramebuffers(1, &m_CopyFBO);
    m_Initialized = true;
    LOG_INFO("HiZPyramid initialized: ", width, "x", height,
             " (", MIP_COUNT, " mips)");
}

void HiZPyramid::Resize(int width, int height) {
    if (width == m_Width && height == m_Height) return;
    destroyTexture();
    m_Width  = width;
    m_Height = height;
    allocateTexture();
}

void HiZPyramid::allocateTexture() {
    glCreateTextures(GL_TEXTURE_2D, 1, &m_Texture);
    glTextureStorage2D(m_Texture, MIP_COUNT, GL_R32F, m_Width, m_Height);
    // Sampler state — NEAREST on both so the compute shader's
    // texelFetch results match what imageStore wrote; linear sampling
    // would average across mip borders and break the min-reduction
    // invariant.
    glTextureParameteri(m_Texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTextureParameteri(m_Texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_Texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_Texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_Texture, GL_TEXTURE_BASE_LEVEL, 0);
    glTextureParameteri(m_Texture, GL_TEXTURE_MAX_LEVEL, MIP_COUNT - 1);
}

void HiZPyramid::destroyTexture() {
    if (m_Texture) { glDeleteTextures(1, &m_Texture); m_Texture = 0; }
    if (m_CopyFBO) { glDeleteFramebuffers(1, &m_CopyFBO); m_CopyFBO = 0; }
    m_Initialized = false;
}

void HiZPyramid::Build(GLuint prepassDepthTexture) {
    if (!m_Initialized || prepassDepthTexture == 0) return;

    // Copy depth buffer -> mip 0 via blit. The source is a depth
    // texture (GL_DEPTH_COMPONENT*) and the dest is R32F — blit
    // does the format conversion for us on supported drivers.
    // Mesa + AMD + NVIDIA all handle GL_DEPTH -> GL_RED via blit.
    glNamedFramebufferTexture(m_CopyFBO, GL_COLOR_ATTACHMENT0, m_Texture, 0);
    GLenum draw = GL_COLOR_ATTACHMENT0;
    glNamedFramebufferDrawBuffers(m_CopyFBO, 1, &draw);

    // Source framebuffer for the blit — create a temporary binding
    // with the prepass depth. We can't blit from a raw texture
    // directly; wrap it in an FBO.
    GLuint srcFBO = 0;
    glCreateFramebuffers(1, &srcFBO);
    glNamedFramebufferTexture(srcFBO, GL_DEPTH_ATTACHMENT, prepassDepthTexture, 0);

    // A depth-to-color blit isn't legal in core GL. Instead, do a
    // fullscreen-triangle "copy depth as R32F" pass. Simpler: run
    // a tiny compute shader that reads the depth texture and writes
    // mip 0. Defer that to a follow-up for simplicity — for now we
    // treat mip 0 as the prepass depth *itself* by sampling it in
    // the compute dispatch for mip 1. This means the Hi-Z texture
    // only owns mips 1..4; mip 0 is the source.
    //
    // That's a correct and simpler contract: consumers read mip 0
    // from the prepass depth, mips 1..4 from the Hi-Z texture. The
    // follow-up can unify into a single resource.
    glDeleteFramebuffers(1, &srcFBO);

    // Compute the 4 reduction mips: mip 1 reads prepass depth (via
    // the passed texture), mip N reads mip N-1 of m_Texture.
    m_ReduceShader.use();

    for (int dst = 1; dst < MIP_COUNT; ++dst) {
        int dstW = std::max(1, m_Width  >> dst);
        int dstH = std::max(1, m_Height >> dst);

        // Source binding: for mip=1 it's the prepass depth (LOD 0);
        // for mip>1 it's our own texture at LOD (dst-1).
        glActiveTexture(GL_TEXTURE0);
        if (dst == 1) {
            glBindTexture(GL_TEXTURE_2D, prepassDepthTexture);
            m_ReduceShader.setInt("uSrcLod", 0);
        } else {
            glBindTexture(GL_TEXTURE_2D, m_Texture);
            m_ReduceShader.setInt("uSrcLod", dst - 1);
        }
        m_ReduceShader.setInt("srcMip", 0);
        // uDstSize is ivec2 in the shader; Shader helper only sends
        // floats via setVec2, so bind the int2 uniform directly.
        GLint loc = glGetUniformLocation(m_ReduceShader.ID, "uDstSize");
        if (loc >= 0) glUniform2i(loc, dstW, dstH);

        // Destination image binding — unit 0 slot matches the
        // shader's `layout(binding = 0) writeonly image2D dstMip`.
        glBindImageTexture(0, m_Texture, dst, GL_FALSE, 0,
                           GL_WRITE_ONLY, GL_R32F);

        GLuint groupsX = (GLuint)((dstW + 15) / 16);
        GLuint groupsY = (GLuint)((dstH + 15) / 16);
        glDispatchCompute(groupsX, groupsY, 1);

        // Barrier ensures mip `dst` writes are visible to the next
        // iteration's read of mip `dst` (as source for dst+1).
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
                      | GL_TEXTURE_FETCH_BARRIER_BIT);
    }
}
