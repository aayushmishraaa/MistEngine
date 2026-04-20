#pragma once
#ifndef MIST_SSR_RENDERER_H
#define MIST_SSR_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

// Screen-Space Reflections via Hi-Z hierarchical ray march.
// Consumes the prepass buffers + Hi-Z pyramid built earlier in the
// frame, composites a reflection contribution onto the HDR color
// produced by TAA. Output is promoted to the chain's `currentTexture`
// so downstream effects (FXAA / motion blur / tonemap) see SSR-
// enhanced HDR.
class SSRRenderer {
public:
    SSRRenderer() = default;
    ~SSRRenderer();

    void Init(int width, int height);
    void Resize(int width, int height);

    // Dispatch the compute shader. `currentTexture` is the HDR color
    // after TAA. The returned texture holds `currentTexture + reflections`.
    void Render(GLuint hdrColor,
                GLuint prepassDepth,
                GLuint prepassNormalRoughness,
                GLuint hiZ,
                const glm::mat4& view,
                const glm::mat4& proj);

    GLuint GetOutputTexture() const { return m_OutputTex; }

    bool  enabled          = true;
    float roughnessCutoff  = 0.7f;   // pixels rougher than this skip SSR
    float maxDistance      = 100.0f; // view-space march distance cap
    int   maxSteps         = 64;     // iteration cap per pixel

private:
    int    m_Width        = 0;
    int    m_Height       = 0;
    GLuint m_OutputTex    = 0;       // RGBA16F image result
    Shader m_Shader;
    bool   m_Initialized  = false;

    void allocateOutput();
    void destroyOutput();
};

#endif // MIST_SSR_RENDERER_H
