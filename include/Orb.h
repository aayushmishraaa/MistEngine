#ifndef ORB_H
#define ORB_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

class Orb {
public:
    Orb(glm::vec3 position = glm::vec3(0.0f), float radius = 0.5f, glm::vec3 color = glm::vec3(1.0f));
    ~Orb();

    void Draw(Shader& shader);
    glm::vec3 GetPosition() const { return position; }
    glm::vec3 GetColor() const { return color; }

private:
    unsigned int VAO, VBO;
    glm::vec3 position;
    glm::vec3 color;
    float radius;

    void setupMesh();
};

#endif