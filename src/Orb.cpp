
#include "Orb.h"
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846 // Define PI if not already defined
#endif

Orb::Orb(const glm::vec3& position, float radius, const glm::vec3& color)
    : position(position), radius(radius), color(color), indexCount(0) {
    setupMesh();
}

Orb::~Orb() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Orb::Draw(Shader& shader) {
    // Set the model matrix for the orb
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(radius));
    shader.setMat4("model", model);

    // Pass orb specific uniforms (assuming your glow shader uses these)
    shader.setVec3("orbColor", color);
    shader.setVec3("orbPosition", position); // Pass position for potential lighting calculations

    // Bind VAO and draw elements
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Orb::setupMesh() {
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;
    std::vector<glm::vec3> normals; // Add normals for lighting

    const unsigned int X_SEGMENTS = 64;
    const unsigned int Y_SEGMENTS = 64;
    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = std::cos(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);
            float yPos = std::cos(ySegment * M_PI);
            float zPos = std::sin(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);

            vertices.push_back(glm::vec3(xPos, yPos, zPos));
            normals.push_back(glm::vec3(xPos, yPos, zPos)); // For a sphere, normal is just the position (when centered at origin)
        }
    }

    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x < X_SEGMENTS; ++x) {
            indices.push_back((y + 0) * (X_SEGMENTS + 1) + (x + 0));
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + (x + 0));
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + (x + 1));

            indices.push_back((y + 0) * (X_SEGMENTS + 1) + (x + 0));
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + (x + 1));
            indices.push_back((y + 0) * (X_SEGMENTS + 1) + (x + 1));
        }
    }
    indexCount = indices.size();

    std::vector<float> data;
    for(size_t i = 0; i < vertices.size(); ++i) {
        data.push_back(vertices[i].x);
        data.push_back(vertices[i].y);
        data.push_back(vertices[i].z);
        data.push_back(normals[i].x); // Add normal data
        data.push_back(normals[i].y);
        data.push_back(normals[i].z);
    }


    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}
