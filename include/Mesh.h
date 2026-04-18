
#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>

#include "Shader.h"
#include "Texture.h"
#include "Renderable.h"
#include "Renderer/RID.h"

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
    // VAO stays raw — vertex layout is a GL concept. Future Vulkan/D3D12
    // backends express this via pipeline-state objects instead. VBO + EBO
    // lifetimes are owned by the device; the GLuint fields are cached for
    // the per-draw bind path.
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    RID          m_VboRid{};
    RID          m_EboRid{};

    void setupMesh();
};

#endif // MESH_H
