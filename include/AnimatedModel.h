#pragma once
#ifndef MIST_ANIMATED_MODEL_H
#define MIST_ANIMATED_MODEL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Animation.h"
#include "Animator.h"
#include "Material.h"
#include "Shader.h"
#include "Texture.h"

struct SkinnedVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    glm::ivec4 BoneIDs = glm::ivec4(-1);
    glm::vec4 BoneWeights = glm::vec4(0.0f);
};

struct SkinnedMesh {
    std::vector<SkinnedVertex> vertices;
    std::vector<unsigned int> indices;
    std::shared_ptr<PBRMaterial> material;
    GLuint VAO = 0, VBO = 0, EBO = 0;

    void Setup();
    void Draw(Shader& shader);
    void Cleanup();
};

struct BoneNode {
    std::string name;
    glm::mat4 transform;
    std::vector<int> children;
};

class AnimatedModel {
public:
    AnimatedModel() = default;
    ~AnimatedModel();

    bool Load(const std::string& path);
    void Draw(Shader& shader);

    std::shared_ptr<Animation> ExtractAnimation(int index = 0);
    std::vector<std::shared_ptr<Animation>> ExtractAllAnimations();

    const std::map<std::string, BoneInfo>& GetBoneMap() const { return m_BoneInfoMap; }
    int GetBoneCount() const { return m_BoneCounter; }

    Animator& GetAnimator() { return m_Animator; }

private:
    std::vector<SkinnedMesh> m_Meshes;
    std::string m_Directory;
    std::map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;
    Animator m_Animator;

    std::vector<BoneNode> m_BoneNodes;
    int m_RootNodeIndex = -1;
    glm::mat4 m_GlobalInverseTransform = glm::mat4(1.0f);

    std::map<std::string, std::shared_ptr<Texture>> m_TextureCache;

    void processNode(aiNode* node, const aiScene* scene);
    SkinnedMesh processMesh(aiMesh* mesh, const aiScene* scene);
    void extractBones(aiMesh* mesh, SkinnedMesh& skinnedMesh);
    int buildNodeHierarchy(aiNode* node);
    void calculateBoneTransformsHierarchy(Animation* anim, float time,
        int nodeIndex, const glm::mat4& parentTransform,
        std::vector<glm::mat4>& boneMatrices);

    std::shared_ptr<Texture> loadTexture(aiMaterial* mat, aiTextureType type, bool sRGB);
};

#endif
