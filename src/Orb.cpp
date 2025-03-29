#include "Orb.h"
#include <vector>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

const float PI = 3.14159265358979323846f;

Orb::Orb(glm::vec3 position, float radius, glm::vec3 color)
    : position(position), radius(radius), color(color) {
    setupMesh();
}

Orb::~Orb() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Orb::setupMesh() {
    std::vector<glm::vec3> vertices;

    // Generate sphere vertices
    const int segments = 32;
    const int rings = 16;

    for (int i = 0; i <= rings; ++i) {
        float theta = i * PI / rings;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (int j = 0; j <= segments; ++j) {
            float phi = j * 2 * PI / segments;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            vertices.push_back(glm::vec3(
                radius * sinTheta * cosPhi,
                radius * sinTheta * sinPhi,
                radius * cosTheta
            ));
        }
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Orb::Draw(Shader& shader) {
    shader.use();
    shader.setVec3("glowColor", color);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    shader.setMat4("model", model);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 34 * 34); // Adjust based on your segments/rings
    glBindVertexArray(0);
}
