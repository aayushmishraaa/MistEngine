#include "ParticleSystem.h"
#include "Core/Logger.h"

struct GPUParticle {
    glm::vec4 position;  // xyz = pos, w = size
    glm::vec4 velocity;  // xyz = vel, w = life
    glm::vec4 color;
    glm::vec4 params;    // x = maxLife, y = unused, z = unused, w = unused
};

GPUParticleSystem::~GPUParticleSystem() {
    for (int i = 0; i < 2; i++) {
        if (m_ParticleSSBO[i]) glDeleteBuffers(1, &m_ParticleSSBO[i]);
    }
    if (m_AliveListSSBO) glDeleteBuffers(1, &m_AliveListSSBO);
    if (m_DeadListSSBO) glDeleteBuffers(1, &m_DeadListSSBO);
    if (m_CounterBuffer) glDeleteBuffers(1, &m_CounterBuffer);
    if (m_IndirectDrawBuffer) glDeleteBuffers(1, &m_IndirectDrawBuffer);
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
}

void GPUParticleSystem::Init() {
    m_EmitShader = Shader("shaders/particle_emit.comp");
    m_SimulateShader = Shader("shaders/particle_simulate.comp");

    // Particle SSBOs (ping-pong)
    for (int i = 0; i < 2; i++) {
        glCreateBuffers(1, &m_ParticleSSBO[i]);
        glNamedBufferStorage(m_ParticleSSBO[i], MAX_PARTICLES * sizeof(GPUParticle), nullptr, GL_DYNAMIC_STORAGE_BIT);
    }

    // Alive/Dead lists
    glCreateBuffers(1, &m_AliveListSSBO);
    glNamedBufferStorage(m_AliveListSSBO, MAX_PARTICLES * sizeof(int), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &m_DeadListSSBO);
    std::vector<int> deadList(MAX_PARTICLES);
    for (int i = 0; i < MAX_PARTICLES; i++) deadList[i] = i;
    glCreateBuffers(1, &m_DeadListSSBO);
    glNamedBufferStorage(m_DeadListSSBO, MAX_PARTICLES * sizeof(int), deadList.data(), GL_DYNAMIC_STORAGE_BIT);

    // Atomic counters
    glCreateBuffers(1, &m_CounterBuffer);
    int counters[4] = {0, MAX_PARTICLES, 0, 0}; // alive, dead, emit, unused
    glNamedBufferStorage(m_CounterBuffer, sizeof(counters), counters, GL_DYNAMIC_STORAGE_BIT);

    // Indirect draw buffer
    glCreateBuffers(1, &m_IndirectDrawBuffer);
    int drawCmd[4] = {0, 1, 0, 0}; // count, instanceCount, first, baseInstance
    glNamedBufferStorage(m_IndirectDrawBuffer, sizeof(drawCmd), drawCmd, GL_DYNAMIC_STORAGE_BIT);

    // VAO for rendering
    glCreateVertexArrays(1, &m_VAO);

    LOG_INFO("GPUParticleSystem initialized: max ", MAX_PARTICLES, " particles");
}

void GPUParticleSystem::Update(float dt, const glm::vec3& cameraPos) {
    if (!enabled || m_Emitters.empty()) return;

    // Emit pass
    if (m_EmitShader.isValid()) {
        m_EmitShader.use();
        m_EmitShader.setFloat("deltaTime", dt);
        m_EmitShader.setVec3("cameraPos", cameraPos);

        for (const auto& emitter : m_Emitters) {
            m_EmitShader.setVec3("emitterPos", emitter.position);
            m_EmitShader.setVec3("emitterDir", emitter.direction);
            m_EmitShader.setFloat("emitRate", emitter.emitRate);
            m_EmitShader.setFloat("lifetime", emitter.lifetime);
            m_EmitShader.setFloat("speed", emitter.speed);
            m_EmitShader.setVec4("startColor", emitter.startColor);

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ParticleSSBO[m_CurrentBuffer]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_DeadListSSBO);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_CounterBuffer);

            int newParticles = (int)(emitter.emitRate * dt);
            if (newParticles > 0) {
                glDispatchCompute((newParticles + 255) / 256, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            }
        }
    }

    // Simulate pass
    if (m_SimulateShader.isValid()) {
        m_SimulateShader.use();
        m_SimulateShader.setFloat("deltaTime", dt);
        m_SimulateShader.setVec3("gravity", m_Emitters[0].gravity);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ParticleSSBO[m_CurrentBuffer]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_AliveListSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_DeadListSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_CounterBuffer);

        glDispatchCompute((MAX_PARTICLES + 255) / 256, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
    }
}

void GPUParticleSystem::Render(const glm::mat4& view, const glm::mat4& projection, GLuint depthTexture) {
    if (!enabled) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending for fire
    glDepthMask(GL_FALSE);

    // Bind particle SSBO for vertex pulling
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ParticleSSBO[m_CurrentBuffer]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_AliveListSSBO);

    if (depthTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
    }

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_IndirectDrawBuffer);
    glDrawArraysIndirect(GL_POINTS, nullptr);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void GPUParticleSystem::AddEmitter(const ParticleEmitter& emitter) {
    m_Emitters.push_back(emitter);
}

void GPUParticleSystem::ClearEmitters() {
    m_Emitters.clear();
}
