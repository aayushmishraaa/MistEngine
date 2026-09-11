
#ifndef MODEL_H
#define MODEL_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>

#include "Mesh.h"
#include "Shader.h"
#include "Renderable.h"

class Model : public Renderable {
public:
    Model(const std::string& path);
    ~Model();

    void Draw(Shader& shader) override;

    // Union of the sub-mesh bounds. A Model draws all of its meshes in one
    // call, so it is culled as one unit.
    bool GetLocalBounds(AABB& out) const override {
        AABB merged;
        for (const auto& m : meshes) {
            AABB sub;
            if (m.GetLocalBounds(sub)) merged.Merge(sub);
        }
        if (!merged.IsValid()) return false;
        out = merged;
        return true;
    }

private:
    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<Texture> textures_loaded;

    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, bool sRGB = false);
};

#endif // MODEL_H
