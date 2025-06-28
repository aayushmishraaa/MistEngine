
#ifndef ORB_H
#define ORB_H

#include <glm/glm.hpp>
#include <glad/glad.h>
#include "Shader.h"
#include "Renderable.h" // Include Renderable

// Inherit from Renderable
class Orb : public Renderable {
public:
    Orb(const glm::vec3& position, float radius, const glm::vec3& color);
    ~Orb();

    // Implement the Draw method from Renderable
    void Draw(Shader& shader) override;

    glm::vec3 GetPosition() const { return position; }
    glm::vec3 GetColor() const { return color; }

private:
    glm::vec3 position;
    float radius;
    glm::vec3 color;

    unsigned int VAO, VBO, EBO;
    int indexCount;

    void setupMesh();
};

#endif // ORB_H
