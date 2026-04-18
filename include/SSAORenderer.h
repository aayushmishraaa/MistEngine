#pragma once
#ifndef MIST_SSAO_RENDERER_H
#define MIST_SSAO_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"
#include "Framebuffer.h"

class SSAORenderer {
public:
    SSAORenderer() = default;
    ~SSAORenderer();

    void Init(int width, int height);
    void Resize(int width, int height);
    void Render(GLuint depthTex, const glm::mat4& projection, const glm::mat4& view);
    GLuint GetSSAOTexture() const { return m_BlurFBO.GetColorTexture(); }

    bool enabled = true;
    float radius = 0.5f;
    float bias = 0.025f;

private:
    Framebuffer m_SSAOFBO;
    Framebuffer m_BlurFBO;
    Shader m_SSAOShader;
    Shader m_BlurShader;

    GLuint m_NoiseTexture = 0;
    // UBO containing the 64-sample kernel. Populated once in Init(); bound
    // to binding 6 each Render() instead of re-uploading 64 uniforms.
    GLuint m_SamplesUBO = 0;
    std::vector<glm::vec3> m_Kernel;
    int m_Width = 0, m_Height = 0;

    void generateKernel();
    void generateNoiseTexture();
    void uploadKernelUBO();
};

#endif
