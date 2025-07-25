
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

private:
    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<Texture> textures_loaded;

    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};

#endif // MODEL_H
