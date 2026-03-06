#pragma once
#ifndef MIST_DEBUG_DRAW_H
#define MIST_DEBUG_DRAW_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"
#include "Scene/AABB.h"

class DebugDraw {
public:
    static void Init();
    static void Shutdown();

    // Primitives
    static void Line(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color = glm::vec3(1));
    static void Box(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color = glm::vec3(0, 1, 0));
    static void AABB(const struct ::AABB& aabb, const glm::vec3& color = glm::vec3(0, 1, 0));
    static void Sphere(const glm::vec3& center, float radius, const glm::vec3& color = glm::vec3(0, 0, 1), int segments = 16);
    static void Frustum(const glm::mat4& viewProj, const glm::vec3& color = glm::vec3(1, 1, 0));

    // Render all accumulated lines and clear
    static void Flush(const glm::mat4& viewProjection);

    static void SetEnabled(bool enabled) { s_Enabled = enabled; }
    static bool IsEnabled() { return s_Enabled; }

private:
    struct DebugVertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    static std::vector<DebugVertex> s_Lines;
    static GLuint s_VAO, s_VBO;
    static Shader* s_Shader;
    static bool s_Enabled;
    static bool s_Initialized;
};

#endif
