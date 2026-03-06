
#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>

#include "Shader.h"
#include "Texture.h"
#include "Renderable.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

// Forward declaration
struct PBRMaterial;

class Mesh : public Renderable {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    std::shared_ptr<PBRMaterial> pbrMaterial;

    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::vector<Texture>& textures);
    ~Mesh();

    void Draw(Shader& shader) override;

private:
    unsigned int VAO, VBO, EBO;

    void setupMesh();
};

#endif // MESH_H
