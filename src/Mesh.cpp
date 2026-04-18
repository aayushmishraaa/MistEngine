
#include "Mesh.h"
#include "Material.h"
#include "Renderer/RenderingDevice.h"
#include "Renderer/GLRenderingDevice.h"
#include <glad/glad.h>
#include <cstdint>

static GLuint getDummyWhiteTexture() {
    static GLuint tex = 0;
    if (tex == 0) {
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        uint8_t white[] = {255, 255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    return tex;
}

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::vector<Texture>& textures)
    : vertices(vertices), indices(indices), textures(textures) {
    setupMesh();
}

Mesh::~Mesh() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (auto* dev = Mist::GPU::Device()) {
        if (m_VboRid.IsValid()) dev->Destroy(m_VboRid);
        if (m_EboRid.IsValid()) dev->Destroy(m_EboRid);
    }
}

void Mesh::Draw(Shader& shader) {
    // If we have a PBR material, use it
    if (pbrMaterial) {
        pbrMaterial->Bind(shader, 1);
    } else {
        // No PBR material — set default scalar uniforms and bind dummy texture
        // to prevent GL_INVALID_OPERATION on Mesa drivers (unbound samplers)
        GLuint dummy = getDummyWhiteTexture();
        for (int unit = 1; unit <= 6; ++unit) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, dummy);
        }
        shader.setBool("material.hasAlbedoMap", false);
        shader.setBool("material.hasNormalMap", false);
        shader.setBool("material.hasMetallicMap", false);
        shader.setBool("material.hasRoughnessMap", false);
        shader.setBool("material.hasAOMap", false);
        shader.setBool("material.hasEmissiveMap", false);
        shader.setVec3("material.albedoColor", glm::vec3(0.8f));
        shader.setFloat("material.metallicValue", 0.0f);
        shader.setFloat("material.roughnessValue", 0.5f);
        shader.setFloat("material.aoValue", 1.0f);
        shader.setVec3("material.emissiveColor", glm::vec3(0.0f));

        // Legacy texture binding on top
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        for (unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            std::string number;
            std::string name = textures[i].type;
            if (name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if (name == "texture_specular")
                number = std::to_string(specularNr++);

            shader.setInt((name + number).c_str(), i);
            textures[i].Bind();
        }
        glActiveTexture(GL_TEXTURE0);
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::setupMesh() {
    auto* dev = static_cast<Mist::GPU::GLRenderingDevice*>(Mist::GPU::Device());
    // A Mesh constructed before Renderer::Init is an asset-pipeline bug —
    // below we fall through to leave VBO/EBO zero so the failure is noisy
    // (empty draw) rather than a crash.

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    Mist::GPU::BufferDesc vbo{};
    vbo.size_bytes = vertices.size() * sizeof(Vertex);
    vbo.usage      = Mist::GPU::BufferUsage::Vertex;
    vbo.initial    = vertices.empty() ? nullptr : vertices.data();
    m_VboRid       = dev ? dev->CreateBuffer(vbo) : RID{};
    VBO            = dev ? dev->GetGLHandle(m_VboRid) : 0;

    Mist::GPU::BufferDesc ebo{};
    ebo.size_bytes = indices.size() * sizeof(unsigned int);
    ebo.usage      = Mist::GPU::BufferUsage::Index;
    ebo.initial    = indices.empty() ? nullptr : indices.data();
    m_EboRid       = dev ? dev->CreateBuffer(ebo) : RID{};
    EBO            = dev ? dev->GetGLHandle(m_EboRid) : 0;

    // VAO needs the buffers bound through the classic target points so the
    // glVertexAttribPointer calls below capture them into the VAO state.
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // Location 0: Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    // Location 1: Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    // Location 2: TexCoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    // Location 3: Tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    // Location 4: Bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

    glBindVertexArray(0);
}
