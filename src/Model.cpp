
#include "Model.h"
#include "Material.h"
#include <iostream>

Model::Model(const std::string& path) {
    loadModel(path);
}

Model::~Model() {
}

void Model::Draw(Shader& shader) {
    for (unsigned int i = 0; i < meshes.size(); i++) {
        meshes[i].Draw(shader);
    }
}

void Model::loadModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        // Position
        vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        // Normal
        if (mesh->mNormals) {
            vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }
        // Texture coordinates
        if (mesh->mTextureCoords[0]) {
            vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        } else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }
        // Tangent
        if (mesh->mTangents) {
            vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
        }
        // Bitangent
        if (mesh->mBitangents) {
            vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        }
        vertices.push_back(vertex);
    }

    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Create PBR material
    auto pbrMat = std::make_shared<PBRMaterial>();

    // Process material
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // Diffuse/Albedo maps (sRGB)
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", true);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        if (!diffuseMaps.empty()) pbrMat->albedoMap = std::make_shared<Texture>(diffuseMaps[0]);

        // Specular maps
        std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", false);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        // Normal maps (linear)
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", false);
        if (normalMaps.empty()) normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal", false);
        if (!normalMaps.empty()) pbrMat->normalMap = std::make_shared<Texture>(normalMaps[0]);

        // Metallic maps (linear)
        std::vector<Texture> metallicMaps = loadMaterialTextures(material, aiTextureType_METALNESS, "texture_metallic", false);
        if (!metallicMaps.empty()) pbrMat->metallicMap = std::make_shared<Texture>(metallicMaps[0]);

        // Roughness maps (linear)
        std::vector<Texture> roughnessMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness", false);
        if (!roughnessMaps.empty()) pbrMat->roughnessMap = std::make_shared<Texture>(roughnessMaps[0]);

        // AO maps (linear)
        std::vector<Texture> aoMaps = loadMaterialTextures(material, aiTextureType_AMBIENT_OCCLUSION, "texture_ao", false);
        if (aoMaps.empty()) aoMaps = loadMaterialTextures(material, aiTextureType_LIGHTMAP, "texture_ao", false);
        if (!aoMaps.empty()) pbrMat->aoMap = std::make_shared<Texture>(aoMaps[0]);

        // Emissive maps (sRGB)
        std::vector<Texture> emissiveMaps = loadMaterialTextures(material, aiTextureType_EMISSIVE, "texture_emissive", true);
        if (!emissiveMaps.empty()) pbrMat->emissiveMap = std::make_shared<Texture>(emissiveMaps[0]);
    }

    Mesh resultMesh(vertices, indices, textures);
    resultMesh.pbrMaterial = pbrMat;
    return resultMesh;
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, bool sRGB) {
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++) {
            if (std::strcmp(textures_loaded[j].path.c_str(), str.C_Str()) == 0) {
                textures.push_back(textures_loaded[j]);
                skip = true;
                break;
            }
        }
        if (!skip) {
            Texture texture;
            if (texture.LoadFromFile(directory + "/" + str.C_Str(), sRGB)) {
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            } else {
                std::cerr << "Warning: Failed to load texture: " << str.C_Str() << std::endl;
            }
        }
    }
    return textures;
}
