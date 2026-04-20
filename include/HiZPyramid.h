#pragma once
#ifndef MIST_HIZ_PYRAMID_H
#define MIST_HIZ_PYRAMID_H

#include <glad/glad.h>
#include "Shader.h"

// Hi-Z depth pyramid — built once per frame from the depth prepass's
// full-res depth texture. Stored as R32F with 5 mips; mip 0 carries
// linear-ish depth copied from the prepass, mip 1..4 are successive
// 2x2 min-reductions.
//
// Consumers (future SSR, GPU culling) read via conservative
// hierarchical ray marching: test against coarsest mip first, descend
// only where the ray intersects. Nothing consumes the pyramid this
// cycle — it's built and stored; SSR next cycle reads it.
class HiZPyramid {
public:
    HiZPyramid() = default;
    ~HiZPyramid();

    void Init(int width, int height);
    void Resize(int width, int height);

    // Copy prepass depth into mip 0 then run 4 compute dispatches
    // (one per subsequent mip) to fill mips 1..4.
    void Build(GLuint prepassDepthTexture);

    GLuint GetTexture() const { return m_Texture; }
    static constexpr int MIP_COUNT = 5;

private:
    GLuint m_Texture    = 0;   // R32F, 5 mips
    GLuint m_CopyFBO    = 0;   // framebuffer for copying prepass depth -> mip 0
    Shader m_ReduceShader;
    int    m_Width      = 0;
    int    m_Height     = 0;
    bool   m_Initialized = false;

    void allocateTexture();
    void destroyTexture();
};

#endif // MIST_HIZ_PYRAMID_H
