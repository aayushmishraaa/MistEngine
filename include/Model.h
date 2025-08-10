#ifndef MODEL_H
#define MODEL_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>

#include "Mesh.h" // Assuming you have a Mesh class
#include "Shader.h"
#include "Renderable.h" // Include Renderable

// Inherit from Renderable
class Model : public Renderable {
public:
    Model(const std::string& path);
    ~Model();

    // Implement the Draw method from Renderable
    void Draw(Shader& shader) override;
    
    // Transformation methods
    void SetPosition(const glm::vec3& position) { m_position = position; UpdateModelMatrix(); }
    void SetRotation(const glm::vec3& rotation) { m_rotation = rotation; UpdateModelMatrix(); }
    void SetScale(const glm::vec3& scale) { m_scale = scale; UpdateModelMatrix(); }
    
    glm::vec3 GetPosition() const { return m_position; }
    glm::vec3 GetRotation() const { return m_rotation; }
    glm::vec3 GetScale() const { return m_scale; }
    
    glm::mat4 GetModelMatrix() const { return m_modelMatrix; }

private:
    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<Texture> textures_loaded;
    
    // Transform properties
    glm::vec3 m_position{0.0f, 0.0f, 0.0f};
    glm::vec3 m_rotation{0.0f, 0.0f, 0.0f}; // Euler angles in degrees
    glm::vec3 m_scale{1.0f, 1.0f, 1.0f};
    glm::mat4 m_modelMatrix{1.0f};

    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
    bool CreateDefaultTexture(Texture& texture);
    void SetTextureID(Texture& texture, unsigned int id);
    void UpdateModelMatrix();
};

#endif // MODEL_H
