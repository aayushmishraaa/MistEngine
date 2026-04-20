#include "Import/SceneImporter.h"

#include "Core/Logger.h"
#include "ECS/Components/HierarchyComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Coordinator.h"
#include "ECS/Systems/HierarchySystem.h"
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <filesystem>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace Mist::Import {

namespace {

// Process-wide owner. RenderComponent holds a non-owning `Renderable*`
// so the Mesh has to live somewhere. Keyed by source path so repeated
// imports of the same .glb don't rebuild the geometry — we just append
// once and reference the same shared_ptrs. Textures are pinned here
// too since PBRMaterial holds shared_ptr<Texture> and those need to
// survive material rebinding.
struct ImportStore {
    std::mutex mu;
    std::vector<std::shared_ptr<Mesh>>    meshes;
    std::vector<std::shared_ptr<Texture>> textures;
};
ImportStore& store() {
    static ImportStore s;
    return s;
}

std::string ascii_lower(std::string s) {
    for (char& c : s) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + 32);
    return s;
}

void decomposeNode(const aiMatrix4x4& m,
                   glm::vec3& pos,
                   glm::vec3& eulerDeg,
                   glm::vec3& scale) {
    aiVector3D   s, p;
    aiQuaternion q;
    m.Decompose(s, q, p);
    pos   = {p.x, p.y, p.z};
    scale = {s.x, s.y, s.z};

    glm::quat gq(q.w, q.x, q.y, q.z);
    // eulerAngles gives radians in XYZ order. TransformComponent
    // stores degrees pitch/yaw/roll matching HierarchySystem's build.
    glm::vec3 e = glm::eulerAngles(gq);
    eulerDeg = glm::degrees(e);
}

std::shared_ptr<Texture> loadFirstTextureOfType(aiMaterial* aim,
                                                aiTextureType t,
                                                bool sRGB,
                                                const std::filesystem::path& baseDir) {
    if (aim->GetTextureCount(t) == 0) return nullptr;
    aiString s;
    aim->GetTexture(t, 0, &s);
    auto tex = std::make_shared<Texture>();
    std::filesystem::path p(s.C_Str());
    if (p.is_relative()) p = baseDir / p;
    if (!tex->LoadFromFile(p.string(), sRGB)) {
        LOG_WARN("SceneImporter: failed to load texture: ", p.string());
        return nullptr;
    }
    {
        std::lock_guard<std::mutex> lk(store().mu);
        store().textures.push_back(tex);
    }
    return tex;
}

void assignTextures(aiMaterial* aim,
                    const std::filesystem::path& baseDir,
                    PBRMaterial& out) {
    out.albedoMap    = loadFirstTextureOfType(aim, aiTextureType_DIFFUSE,           true,  baseDir);
    out.normalMap    = loadFirstTextureOfType(aim, aiTextureType_NORMALS,           false, baseDir);
    if (!out.normalMap) out.normalMap = loadFirstTextureOfType(aim, aiTextureType_HEIGHT, false, baseDir);
    out.metallicMap  = loadFirstTextureOfType(aim, aiTextureType_METALNESS,         false, baseDir);
    out.roughnessMap = loadFirstTextureOfType(aim, aiTextureType_DIFFUSE_ROUGHNESS, false, baseDir);
    // glTF packs metallic+roughness into UNKNOWN; use as fallback.
    if (!out.metallicMap && !out.roughnessMap) {
        auto mr = loadFirstTextureOfType(aim, aiTextureType_UNKNOWN, false, baseDir);
        out.metallicMap  = mr;
        out.roughnessMap = mr;
    }
    out.aoMap        = loadFirstTextureOfType(aim, aiTextureType_AMBIENT_OCCLUSION, false, baseDir);
    if (!out.aoMap) out.aoMap = loadFirstTextureOfType(aim, aiTextureType_LIGHTMAP, false, baseDir);
    out.emissiveMap  = loadFirstTextureOfType(aim, aiTextureType_EMISSIVE,          true,  baseDir);

    aiColor3D col(1, 1, 1);
    if (aim->Get(AI_MATKEY_COLOR_DIFFUSE, col) == AI_SUCCESS)
        out.albedo = {col.r, col.g, col.b};
    aiColor3D emi(0, 0, 0);
    if (aim->Get(AI_MATKEY_COLOR_EMISSIVE, emi) == AI_SUCCESS)
        out.emissive = {emi.r, emi.g, emi.b};
    float f;
    if (aim->Get(AI_MATKEY_METALLIC_FACTOR, f)  == AI_SUCCESS) out.metallic  = f;
    if (aim->Get(AI_MATKEY_ROUGHNESS_FACTOR, f) == AI_SUCCESS) out.roughness = f;
}

std::shared_ptr<Mesh> buildMesh(aiMesh* aim,
                                aiMaterial* aimat,
                                const std::filesystem::path& baseDir) {
    std::vector<Vertex> verts;
    verts.reserve(aim->mNumVertices);
    for (unsigned int i = 0; i < aim->mNumVertices; ++i) {
        Vertex v{};
        v.Position = {aim->mVertices[i].x, aim->mVertices[i].y, aim->mVertices[i].z};
        if (aim->mNormals) {
            v.Normal = {aim->mNormals[i].x, aim->mNormals[i].y, aim->mNormals[i].z};
        }
        if (aim->mTextureCoords[0]) {
            v.TexCoords = {aim->mTextureCoords[0][i].x, aim->mTextureCoords[0][i].y};
        }
        if (aim->mTangents) {
            v.Tangent = {aim->mTangents[i].x, aim->mTangents[i].y, aim->mTangents[i].z};
        }
        if (aim->mBitangents) {
            v.Bitangent = {aim->mBitangents[i].x, aim->mBitangents[i].y, aim->mBitangents[i].z};
        }
        verts.push_back(v);
    }

    std::vector<unsigned int> idx;
    idx.reserve(aim->mNumFaces * 3);
    for (unsigned int i = 0; i < aim->mNumFaces; ++i) {
        const aiFace& f = aim->mFaces[i];
        for (unsigned int k = 0; k < f.mNumIndices; ++k) idx.push_back(f.mIndices[k]);
    }

    auto mesh = std::make_shared<Mesh>(verts, idx, std::vector<Texture>{});
    auto pbr  = std::make_shared<PBRMaterial>();
    if (aimat) assignTextures(aimat, baseDir, *pbr);
    mesh->pbrMaterial = pbr;
    return mesh;
}

Entity importNodeTree(aiNode* node,
                      const aiScene* scene,
                      const std::filesystem::path& baseDir,
                      Coordinator& coord,
                      std::vector<std::shared_ptr<Mesh>>& outMeshes,
                      std::unordered_map<Entity, std::string>* outNames) {
    Entity e = coord.CreateEntity();

    TransformComponent t;
    decomposeNode(node->mTransformation, t.position, t.rotation, t.scale);
    coord.AddComponent(e, t);
    coord.AddComponent(e, HierarchyComponent{});

    if (outNames) {
        std::string nm = node->mName.C_Str();
        (*outNames)[e] = nm.empty() ? "Node" : nm;
    }

    // One-mesh node: attach RenderComponent directly. Multi-mesh node:
    // spawn a child entity per aiMesh so each gets its own material
    // and can be selected/hidden independently.
    if (node->mNumMeshes == 1) {
        auto* aim = scene->mMeshes[node->mMeshes[0]];
        auto m = buildMesh(aim, scene->mMaterials[aim->mMaterialIndex], baseDir);
        outMeshes.push_back(m);
        RenderComponent r;
        r.renderable = m.get();
        r.visible    = true;
        coord.AddComponent(e, r);
    } else {
        for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
            auto* aim = scene->mMeshes[node->mMeshes[i]];
            auto m = buildMesh(aim, scene->mMaterials[aim->mMaterialIndex], baseDir);
            outMeshes.push_back(m);

            Entity me = coord.CreateEntity();
            coord.AddComponent(me, TransformComponent{});
            coord.AddComponent(me, HierarchyComponent{});
            RenderComponent r;
            r.renderable = m.get();
            r.visible    = true;
            coord.AddComponent(me, r);
            if (outNames) {
                std::string nm = aim->mName.C_Str();
                (*outNames)[me] = nm.empty() ? "Mesh" : nm;
            }
            HierarchySystem::Attach(coord, e, me);
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        Entity childE = importNodeTree(node->mChildren[i], scene, baseDir,
                                       coord, outMeshes, outNames);
        HierarchySystem::Attach(coord, e, childE);
    }
    return e;
}

} // namespace

bool SceneImporter::IsSupportedPath(std::string_view path) {
    auto dot = path.find_last_of('.');
    if (dot == std::string_view::npos) return false;
    std::string ext = ascii_lower(std::string(path.substr(dot)));
    return ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb";
}

Entity SceneImporter::ImportToScene(const std::string& path,
                                     Coordinator& coord,
                                     std::unordered_map<Entity, std::string>* outNames) {
    Assimp::Importer imp;
    const aiScene* scene = imp.ReadFile(path,
        aiProcess_Triangulate            |
        aiProcess_GenSmoothNormals       |
        aiProcess_CalcTangentSpace       |
        aiProcess_FlipUVs                |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_LimitBoneWeights);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        LOG_ERROR("SceneImporter: ", imp.GetErrorString(), " (path=", path, ")");
        return static_cast<Entity>(-1);
    }

    std::filesystem::path baseDir = std::filesystem::path(path).parent_path();

    std::vector<std::shared_ptr<Mesh>> imported;
    Entity root = importNodeTree(scene->mRootNode, scene, baseDir,
                                 coord, imported, outNames);

    {
        std::lock_guard<std::mutex> lk(store().mu);
        store().meshes.insert(store().meshes.end(), imported.begin(), imported.end());
    }
    LOG_INFO("SceneImporter: imported '", path,
             "' -> ", imported.size(), " meshes, root entity ",
             static_cast<int>(root));
    return root;
}

} // namespace Mist::Import
