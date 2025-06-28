
#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>
#include <vector>
#include <string>

#include "Shader.h"
#include "Texture.h" // Assuming you have a Texture class

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::vector<Texture>& textures);
    ~Mesh();

    void Draw(Shader& shader);

private:
    unsigned int VAO, VBO, EBO;

    void setupMesh();
};

#endif // MESH_H
