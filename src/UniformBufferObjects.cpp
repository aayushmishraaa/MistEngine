#include "UniformBufferObjects.h"
#include "Core/Logger.h"

UBOManager::~UBOManager() {
    if (m_PerFrameUBO)  glDeleteBuffers(1, &m_PerFrameUBO);
    if (m_PerObjectUBO) glDeleteBuffers(1, &m_PerObjectUBO);
}

void UBOManager::Init() {
    // Per-frame UBO at binding 0
    glCreateBuffers(1, &m_PerFrameUBO);
    glNamedBufferStorage(m_PerFrameUBO, sizeof(PerFrameUBO), nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_PerFrameUBO);

    // Per-object UBO at binding 1
    glCreateBuffers(1, &m_PerObjectUBO);
    glNamedBufferStorage(m_PerObjectUBO, sizeof(PerObjectUBO), nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_PerObjectUBO);

    LOG_INFO("UBOManager initialized: PerFrame=", sizeof(PerFrameUBO), "B, PerObject=", sizeof(PerObjectUBO), "B");
}

void UBOManager::UpdatePerFrame(const PerFrameUBO& data) {
    glNamedBufferSubData(m_PerFrameUBO, 0, sizeof(PerFrameUBO), &data);
}

void UBOManager::UpdatePerObject(const PerObjectUBO& data) {
    glNamedBufferSubData(m_PerObjectUBO, 0, sizeof(PerObjectUBO), &data);
}
