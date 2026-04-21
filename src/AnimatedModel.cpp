#include "AnimatedModel.h"
#include "Core/Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// --- SkinnedMesh ---

void SkinnedMesh::Setup() {
    glCreateVertexArrays(1, &VAO);
    glCreateBuffers(1, &VBO);
    glCreateBuffers(1, &EBO);

    glNamedBufferStorage(VBO, vertices.size() * sizeof(SkinnedVertex), vertices.data(), 0);
    glNamedBufferStorage(EBO, indices.size() * sizeof(unsigned int), indices.data(), 0);

    glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(SkinnedVertex));
    glVertexArrayElementBuffer(VAO, EBO);

    // Position
    glEnableVertexArrayAttrib(VAO, 0);
    glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, offsetof(SkinnedVertex, Position));
    glVertexArrayAttribBinding(VAO, 0, 0);
    // Normal
    glEnableVertexArrayAttrib(VAO, 1);
    glVertexArrayAttribFormat(VAO, 1, 3, GL_FLOAT, GL_FALSE, offsetof(SkinnedVertex, Normal));
    glVertexArrayAttribBinding(VAO, 1, 0);
    // TexCoords
    glEnableVertexArrayAttrib(VAO, 2);
    glVertexArrayAttribFormat(VAO, 2, 2, GL_FLOAT, GL_FALSE, offsetof(SkinnedVertex, TexCoords));
    glVertexArrayAttribBinding(VAO, 2, 0);
    // Tangent
    glEnableVertexArrayAttrib(VAO, 3);
    glVertexArrayAttribFormat(VAO, 3, 3, GL_FLOAT, GL_FALSE, offsetof(SkinnedVertex, Tangent));
    glVertexArrayAttribBinding(VAO, 3, 0);
    // Bitangent
    glEnableVertexArrayAttrib(VAO, 4);
    glVertexArrayAttribFormat(VAO, 4, 3, GL_FLOAT, GL_FALSE, offsetof(SkinnedVertex, Bitangent));
    glVertexArrayAttribBinding(VAO, 4, 0);
    // BoneIDs (integer attribute)
    glEnableVertexArrayAttrib(VAO, 5);
    glVertexArrayAttribIFormat(VAO, 5, 4, GL_INT, offsetof(SkinnedVertex, BoneIDs));
    glVertexArrayAttribBinding(VAO, 5, 0);
    // BoneWeights
    glEnableVertexArrayAttrib(VAO, 6);
    glVertexArrayAttribFormat(VAO, 6, 4, GL_FLOAT, GL_FALSE, offsetof(SkinnedVertex, BoneWeights));
    glVertexArrayAttribBinding(VAO, 6, 0);
}

void SkinnedMesh::Draw(Shader& shader) {
    if (material) material->Bind(shader);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void SkinnedMesh::Cleanup() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    VAO = VBO = EBO = 0;
}

// --- AnimatedModel ---

AnimatedModel::~AnimatedModel() {
    for (auto& mesh : m_Meshes) mesh.Cleanup();
}

bool AnimatedModel::Load(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace | aiProcess_FlipUVs |
        aiProcess_LimitBoneWeights);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        LOG_ERROR("AnimatedModel: Failed to load: ", path, " - ", importer.GetErrorString());
        return false;
    }

    m_Directory = path.substr(0, path.find_last_of('/'));
    m_GlobalInverseTransform = glm::mat4(1.0f);

    // Convert aiMatrix4x4 to glm
    auto aiToGlm = [](const aiMatrix4x4& m) -> glm::mat4 {
        return glm::transpose(glm::make_mat4(&m.a1));
    };
    m_GlobalInverseTransform = glm::inverse(aiToGlm(scene->mRootNode->mTransformation));

    m_RootNodeIndex = buildNodeHierarchy(scene->mRootNode);
    processNode(scene->mRootNode, scene);

    for (auto& mesh : m_Meshes) mesh.Setup();

    m_Animator.Init();

    // Parse embedded aiAnimations into engine Animation objects. Done
    // here (while the aiScene is live) so ExtractAllAnimations doesn't
    // need to re-parse the file. Each bone animation maps back to its
    // bone via the name -> m_BoneInfoMap id lookup; keys come straight
    // from Assimp channel data.
    m_Clips.clear();
    m_Clips.reserve(scene->mNumAnimations);
    for (unsigned int a = 0; a < scene->mNumAnimations; ++a) {
        const aiAnimation* ai = scene->mAnimations[a];
        auto clip = std::make_shared<Animation>();
        clip->name = ai->mName.length > 0 ? ai->mName.C_Str()
                                           : ("clip_" + std::to_string(a));
        clip->duration       = static_cast<float>(ai->mDuration);
        clip->ticksPerSecond = ai->mTicksPerSecond != 0.0
                                ? static_cast<float>(ai->mTicksPerSecond) : 25.0f;

        clip->boneAnimations.reserve(ai->mNumChannels);
        for (unsigned int c = 0; c < ai->mNumChannels; ++c) {
            const aiNodeAnim* ch = ai->mChannels[c];
            BoneAnimation ba;
            ba.name = ch->mNodeName.C_Str();

            auto bi = m_BoneInfoMap.find(ba.name);
            ba.boneID = (bi != m_BoneInfoMap.end()) ? bi->second.id : -1;

            ba.positions.reserve(ch->mNumPositionKeys);
            for (unsigned int k = 0; k < ch->mNumPositionKeys; ++k) {
                const auto& key = ch->mPositionKeys[k];
                ba.positions.push_back(KeyPosition{
                    static_cast<float>(key.mTime),
                    glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z)});
            }
            ba.rotations.reserve(ch->mNumRotationKeys);
            for (unsigned int k = 0; k < ch->mNumRotationKeys; ++k) {
                const auto& key = ch->mRotationKeys[k];
                ba.rotations.push_back(KeyRotation{
                    static_cast<float>(key.mTime),
                    glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z)});
            }
            ba.scales.reserve(ch->mNumScalingKeys);
            for (unsigned int k = 0; k < ch->mNumScalingKeys; ++k) {
                const auto& key = ch->mScalingKeys[k];
                ba.scales.push_back(KeyScale{
                    static_cast<float>(key.mTime),
                    glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z)});
            }

            clip->boneNameToIndex[ba.name] = static_cast<int>(clip->boneAnimations.size());
            clip->boneAnimations.push_back(std::move(ba));
        }
        m_Clips.push_back(std::move(clip));
    }

    LOG_INFO("AnimatedModel loaded: ", path, " (", m_Meshes.size(), " meshes, ",
             m_BoneCounter, " bones, ", m_Clips.size(), " clips)");
    return true;
}

void AnimatedModel::Draw(Shader& shader) {
    m_Animator.BindBoneSSBO(6);
    for (auto& mesh : m_Meshes) {
        mesh.Draw(shader);
    }
}

void AnimatedModel::processNode(aiNode* node, const aiScene* scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_Meshes.push_back(processMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

SkinnedMesh AnimatedModel::processMesh(aiMesh* mesh, const aiScene* scene) {
    SkinnedMesh result;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        SkinnedVertex v;
        v.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
        v.Normal = mesh->mNormals
            ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)
            : glm::vec3(0, 1, 0);
        v.TexCoords = mesh->mTextureCoords[0]
            ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
            : glm::vec2(0);
        if (mesh->mTangents) {
            v.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
            v.Bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
        }
        result.vertices.push_back(v);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            result.indices.push_back(face.mIndices[j]);
    }

    extractBones(mesh, result);

    // Material
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
        auto pbr = std::make_shared<PBRMaterial>();
        auto albedoTex = loadTexture(mat, aiTextureType_DIFFUSE, true);
        if (albedoTex) pbr->albedoMap = albedoTex;
        auto normalTex = loadTexture(mat, aiTextureType_NORMALS, false);
        if (!normalTex) normalTex = loadTexture(mat, aiTextureType_HEIGHT, false);
        if (normalTex) pbr->normalMap = normalTex;
        auto metallicTex = loadTexture(mat, aiTextureType_METALNESS, false);
        if (metallicTex) pbr->metallicMap = metallicTex;
        auto roughnessTex = loadTexture(mat, aiTextureType_DIFFUSE_ROUGHNESS, false);
        if (roughnessTex) pbr->roughnessMap = roughnessTex;
        auto aoTex = loadTexture(mat, aiTextureType_AMBIENT_OCCLUSION, false);
        if (aoTex) pbr->aoMap = aoTex;
        auto emissiveTex = loadTexture(mat, aiTextureType_EMISSIVE, true);
        if (emissiveTex) pbr->emissiveMap = emissiveTex;
        result.material = pbr;
    }

    return result;
}

void AnimatedModel::extractBones(aiMesh* mesh, SkinnedMesh& skinnedMesh) {
    for (unsigned int boneIdx = 0; boneIdx < mesh->mNumBones; boneIdx++) {
        aiBone* bone = mesh->mBones[boneIdx];
        std::string boneName(bone->mName.C_Str());

        int boneID;
        auto it = m_BoneInfoMap.find(boneName);
        if (it == m_BoneInfoMap.end()) {
            BoneInfo info;
            info.id = m_BoneCounter++;
            info.offset = glm::transpose(glm::make_mat4(&bone->mOffsetMatrix.a1));
            m_BoneInfoMap[boneName] = info;
            boneID = info.id;
        } else {
            boneID = it->second.id;
        }

        for (unsigned int w = 0; w < bone->mNumWeights; w++) {
            unsigned int vertexID = bone->mWeights[w].mVertexId;
            float weight = bone->mWeights[w].mWeight;
            if (vertexID >= skinnedMesh.vertices.size()) continue;

            auto& v = skinnedMesh.vertices[vertexID];
            for (int slot = 0; slot < 4; slot++) {
                if (v.BoneIDs[slot] < 0) {
                    v.BoneIDs[slot] = boneID;
                    v.BoneWeights[slot] = weight;
                    break;
                }
            }
        }
    }
}

int AnimatedModel::buildNodeHierarchy(aiNode* node) {
    int idx = static_cast<int>(m_BoneNodes.size());
    BoneNode bn;
    bn.name = node->mName.C_Str();
    bn.transform = glm::transpose(glm::make_mat4(&node->mTransformation.a1));
    m_BoneNodes.push_back(bn);

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        int childIdx = buildNodeHierarchy(node->mChildren[i]);
        m_BoneNodes[idx].children.push_back(childIdx);
    }
    return idx;
}

std::shared_ptr<Animation> AnimatedModel::ExtractAnimation(int index) {
    if (index >= 0 && index < static_cast<int>(m_Clips.size())) {
        return m_Clips[index];
    }
    return nullptr;
}

std::vector<std::shared_ptr<Animation>> AnimatedModel::ExtractAllAnimations() {
    return m_Clips;
}

std::shared_ptr<Texture> AnimatedModel::loadTexture(aiMaterial* mat, aiTextureType type, bool sRGB) {
    if (mat->GetTextureCount(type) == 0) return nullptr;

    aiString str;
    mat->GetTexture(type, 0, &str);
    std::string path = m_Directory + "/" + str.C_Str();

    auto it = m_TextureCache.find(path);
    if (it != m_TextureCache.end()) return it->second;

    auto tex = std::make_shared<Texture>();
    tex->isSRGB = sRGB;
    if (tex->LoadFromFile(path, sRGB)) {
        m_TextureCache[path] = tex;
        return tex;
    }
    return nullptr;
}
