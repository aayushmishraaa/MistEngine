#pragma once
#ifndef MIST_PARTICLE_SYSTEM_H
#define MIST_PARTICLE_SYSTEM_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"

enum class EmitterShape { Point, Sphere, Cone };

struct ParticleEmitter {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f, 1.0f, 0.0f);
    EmitterShape shape = EmitterShape::Point;
    float radius = 1.0f;
    float coneAngle = 45.0f;
    float emitRate = 100.0f;
    float lifetime = 2.0f;
    float speed = 5.0f;
    float speedVariance = 1.0f;
    glm::vec4 startColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
    glm::vec4 endColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    float startSize = 0.5f;
    float endSize = 0.0f;
    glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
};

class GPUParticleSystem {
public:
    static constexpr int MAX_PARTICLES = 1000000;

    GPUParticleSystem() = default;
    ~GPUParticleSystem();

    void Init();
    void Update(float dt, const glm::vec3& cameraPos);
    void Render(const glm::mat4& view, const glm::mat4& projection, GLuint depthTexture);

    void AddEmitter(const ParticleEmitter& emitter);
    void ClearEmitters();

    bool enabled = true;

private:
    GLuint m_ParticleSSBO[2] = {0, 0}; // ping-pong
    GLuint m_AliveListSSBO = 0;
    GLuint m_DeadListSSBO = 0;
    GLuint m_CounterBuffer = 0;
    GLuint m_IndirectDrawBuffer = 0;
    GLuint m_VAO = 0;
    int m_CurrentBuffer = 0;

    Shader m_EmitShader;
    Shader m_SimulateShader;
    Shader m_RenderVert;
    Shader m_RenderFrag;

    std::vector<ParticleEmitter> m_Emitters;
    float m_AccumulatedTime = 0.0f;
};

#endif
