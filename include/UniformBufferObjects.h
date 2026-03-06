#pragma once
#ifndef MIST_UNIFORM_BUFFER_OBJECTS_H
#define MIST_UNIFORM_BUFFER_OBJECTS_H

#include <glad/glad.h>
#include <glm/glm.hpp>

// Binding 0: Per-frame data (192 bytes, std140)
struct PerFrameUBO {
    glm::mat4 view;           // 0   (64 bytes)
    glm::mat4 projection;     // 64  (64 bytes)
    glm::vec4 viewPos;        // 128 (16 bytes, w unused)
    glm::vec4 lightDir;       // 144 (16 bytes, w unused)
    glm::vec4 lightColor;     // 160 (16 bytes, w unused)
    float time;               // 176
    float deltaTime;          // 180
    float nearPlane;          // 184
    float farPlane;           // 188
};                            // Total: 192 bytes

// Binding 1: Per-object data (192 bytes, std140)
struct PerObjectUBO {
    glm::mat4 model;          // 0   (64 bytes)
    glm::mat4 normalMatrix;   // 64  (64 bytes, mat4 for std140 alignment of mat3)
};

class UBOManager {
public:
    UBOManager() : m_PerFrameUBO(0), m_PerObjectUBO(0) {}
    ~UBOManager();

    void Init();
    void UpdatePerFrame(const PerFrameUBO& data);
    void UpdatePerObject(const PerObjectUBO& data);

private:
    GLuint m_PerFrameUBO;
    GLuint m_PerObjectUBO;
};

#endif // MIST_UNIFORM_BUFFER_OBJECTS_H
