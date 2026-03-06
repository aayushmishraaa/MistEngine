#pragma once
#ifndef MIST_BLOOM_RENDERER_H
#define MIST_BLOOM_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"

struct BloomMip {
    glm::vec2 size;
    GLuint texture;
};

class BloomRenderer {
public:
    BloomRenderer() = default;
    ~BloomRenderer();

    void Init(int width, int height, int mipLevels = 6);
    void Resize(int width, int height);
    void RenderBloom(GLuint srcTexture, float threshold, float intensity);
    GLuint GetBloomTexture() const;

    bool enabled = true;
    float threshold = 1.0f;
    float intensity = 0.5f;

private:
    GLuint m_FBO = 0;
    std::vector<BloomMip> m_MipChain;
    int m_MipLevels = 6;
    int m_SrcWidth = 0, m_SrcHeight = 0;

    Shader m_DownsampleShader;
    Shader m_UpsampleShader;

    void createMipChain(int width, int height);
    void cleanup();
};

#endif
