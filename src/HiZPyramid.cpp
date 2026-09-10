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
    m_Initialized = false;
}

void HiZPyramid::Build(GLuint prepassDepthTexture) {
    if (!m_Initialized || prepassDepthTexture == 0) return;

    // Split-ownership contract: mip 0 of this pyramid is NOT written. Consumers
    // read the full-resolution level straight from the prepass depth texture
    // and levels 1..4 from here — see `sampleHiZ` in ssr.comp, which branches
    // on `lod <= 0` for exactly this reason.
    //
    // Previously this function created an FBO, attached the prepass depth to
    // it, and deleted it again on every single frame — leftovers from an
    // abandoned depth-to-colour blit (which isn't legal in core GL anyway).
    // It produced nothing and cost two driver round-trips per frame. The
    // `m_CopyFBO` member it was paired with is gone too.

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
