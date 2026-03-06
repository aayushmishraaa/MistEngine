#include "Debug/DebugDraw.h"
#include "Core/Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

std::vector<DebugDraw::DebugVertex> DebugDraw::s_Lines;
GLuint DebugDraw::s_VAO = 0;
GLuint DebugDraw::s_VBO = 0;
Shader* DebugDraw::s_Shader = nullptr;
bool DebugDraw::s_Enabled = true;
bool DebugDraw::s_Initialized = false;

static const char* DEBUG_VERT_SRC = R"(
#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 viewProjection;
out vec3 vColor;
void main() {
    vColor = aColor;
    gl_Position = viewProjection * vec4(aPos, 1.0);
}
)";

static const char* DEBUG_FRAG_SRC = R"(
#version 460 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

void DebugDraw::Init() {
    if (s_Initialized) return;

    glCreateVertexArrays(1, &s_VAO);
    glCreateBuffers(1, &s_VBO);

    // Pre-allocate buffer (will grow if needed)
    glNamedBufferStorage(s_VBO, 65536 * sizeof(DebugVertex), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glVertexArrayVertexBuffer(s_VAO, 0, s_VBO, 0, sizeof(DebugVertex));

    glEnableVertexArrayAttrib(s_VAO, 0);
    glVertexArrayAttribFormat(s_VAO, 0, 3, GL_FLOAT, GL_FALSE, offsetof(DebugVertex, position));
    glVertexArrayAttribBinding(s_VAO, 0, 0);

    glEnableVertexArrayAttrib(s_VAO, 1);
    glVertexArrayAttribFormat(s_VAO, 1, 3, GL_FLOAT, GL_FALSE, offsetof(DebugVertex, color));
    glVertexArrayAttribBinding(s_VAO, 1, 0);

    // Compile inline shader
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &DEBUG_VERT_SRC, nullptr);
    glCompileShader(vert);

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &DEBUG_FRAG_SRC, nullptr);
    glCompileShader(frag);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);

    s_Shader = new Shader();
    s_Shader->ID = prog;

    s_Initialized = true;
    s_Lines.reserve(4096);
    LOG_INFO("DebugDraw initialized");
}

void DebugDraw::Shutdown() {
    if (!s_Initialized) return;
    if (s_VAO) glDeleteVertexArrays(1, &s_VAO);
    if (s_VBO) glDeleteBuffers(1, &s_VBO);
    delete s_Shader;
    s_Shader = nullptr;
    s_Initialized = false;
}

void DebugDraw::Line(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) {
    if (!s_Enabled) return;
    s_Lines.push_back({from, color});
    s_Lines.push_back({to, color});
}

void DebugDraw::Box(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color) {
    if (!s_Enabled) return;
    // Bottom face
    Line({min.x, min.y, min.z}, {max.x, min.y, min.z}, color);
    Line({max.x, min.y, min.z}, {max.x, min.y, max.z}, color);
    Line({max.x, min.y, max.z}, {min.x, min.y, max.z}, color);
    Line({min.x, min.y, max.z}, {min.x, min.y, min.z}, color);
    // Top face
    Line({min.x, max.y, min.z}, {max.x, max.y, min.z}, color);
    Line({max.x, max.y, min.z}, {max.x, max.y, max.z}, color);
    Line({max.x, max.y, max.z}, {min.x, max.y, max.z}, color);
    Line({min.x, max.y, max.z}, {min.x, max.y, min.z}, color);
    // Verticals
    Line({min.x, min.y, min.z}, {min.x, max.y, min.z}, color);
    Line({max.x, min.y, min.z}, {max.x, max.y, min.z}, color);
    Line({max.x, min.y, max.z}, {max.x, max.y, max.z}, color);
    Line({min.x, min.y, max.z}, {min.x, max.y, max.z}, color);
}

void DebugDraw::AABB(const struct ::AABB& aabb, const glm::vec3& color) {
    Box(aabb.min, aabb.max, color);
}

void DebugDraw::Sphere(const glm::vec3& center, float radius, const glm::vec3& color, int segments) {
    if (!s_Enabled) return;
    float step = 2.0f * 3.14159265f / static_cast<float>(segments);
    for (int i = 0; i < segments; i++) {
        float a0 = step * i;
        float a1 = step * (i + 1);
        // XY circle
        Line(center + glm::vec3(std::cos(a0), std::sin(a0), 0) * radius,
             center + glm::vec3(std::cos(a1), std::sin(a1), 0) * radius, color);
        // XZ circle
        Line(center + glm::vec3(std::cos(a0), 0, std::sin(a0)) * radius,
             center + glm::vec3(std::cos(a1), 0, std::sin(a1)) * radius, color);
        // YZ circle
        Line(center + glm::vec3(0, std::cos(a0), std::sin(a0)) * radius,
             center + glm::vec3(0, std::cos(a1), std::sin(a1)) * radius, color);
    }
}

void DebugDraw::Frustum(const glm::mat4& viewProj, const glm::vec3& color) {
    if (!s_Enabled) return;
    glm::mat4 inv = glm::inverse(viewProj);

    // NDC corners
    glm::vec3 ndc[8] = {
        {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1},
        {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1}
    };

    glm::vec3 world[8];
    for (int i = 0; i < 8; i++) {
        glm::vec4 w = inv * glm::vec4(ndc[i], 1.0f);
        world[i] = glm::vec3(w) / w.w;
    }

    // Near face
    for (int i = 0; i < 4; i++) Line(world[i], world[(i + 1) % 4], color);
    // Far face
    for (int i = 0; i < 4; i++) Line(world[4 + i], world[4 + (i + 1) % 4], color);
    // Connecting edges
    for (int i = 0; i < 4; i++) Line(world[i], world[i + 4], color);
}

void DebugDraw::Flush(const glm::mat4& viewProjection) {
    if (!s_Enabled || !s_Initialized || s_Lines.empty()) return;

    // Re-create buffer if needed (exceeded pre-allocated size)
    size_t dataSize = s_Lines.size() * sizeof(DebugVertex);
    if (dataSize > 65536 * sizeof(DebugVertex)) {
        glDeleteBuffers(1, &s_VBO);
        glCreateBuffers(1, &s_VBO);
        glNamedBufferStorage(s_VBO, dataSize, s_Lines.data(), GL_DYNAMIC_STORAGE_BIT);
        glVertexArrayVertexBuffer(s_VAO, 0, s_VBO, 0, sizeof(DebugVertex));
    } else {
        glNamedBufferSubData(s_VBO, 0, dataSize, s_Lines.data());
    }

    glUseProgram(s_Shader->ID);
    s_Shader->setMat4("viewProjection", viewProjection);

    glBindVertexArray(s_VAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(s_Lines.size()));
    glBindVertexArray(0);

    s_Lines.clear();
}
