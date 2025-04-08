#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include <string>
#include <vector>
#include <map>
#include <iostream>

class Model {
public:
    Model(const char* path) {
        loadModel(path);
    }

    void Draw(Shader& shader) {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

private:
    // Model data
    std::vector<Mesh> meshes;
    std::string directory;
    std::map<std::string, Texture> loadedTextures;

    void loadModel(std::string path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return;
        }
        directory = path.substr(0, path.find_last_of('/'));

        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode* node, const aiScene* scene) {
        // Process all the node's meshes (if any)
        for(unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

        // Then do the same for each of its children
        for(unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        // Process vertices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            // Position
            vertex.Position = glm::vec3(mesh->mVertices[i].x,
                                      mesh->mVertices[i].y,
                                      mesh->mVertices[i].z);
            // Normal
            if (mesh->HasNormals()) {
                vertex.Normal = glm::vec3(mesh->mNormals[i].x,
                                        mesh->mNormals[i].y,
                                        mesh->mNormals[i].z);
            }
            // Texture coordinates
            if(mesh->mTextureCoords[0]) {
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x,
                                           mesh->mTextureCoords[0][i].y);
            } else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }
            vertices.push_back(vertex);
        }

        // Process indices
        for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Process material
        if(mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            std::vector<Texture> diffuseMaps = loadMaterialTextures(material,
                aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            std::vector<Texture> specularMaps = loadMaterialTextures(material,
                aiTextureType_SPECULAR, "texture_specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        }

        return Mesh(vertices, indices, textures);
    }

    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName) {
        std::vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            std::string path = str.C_Str();

            // Check if texture was loaded before
            if(loadedTextures.find(path) == loadedTextures.end()) {
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), directory);
                texture.type = typeName;
                texture.path = path;
                textures.push_back(texture);
                loadedTextures[path] = texture;
            } else {
                textures.push_back(loadedTextures[path]);
            }
        }
        return textures;
    }

    unsigned int TextureFromFile(const char* path, const std::string& directory) {
        std::string filename = std::string(path);
        filename = directory + '/' + filename;

        Texture tex;
        if(!tex.LoadFromFile(filename.c_str())) {
            std::cout << "Texture failed to load at path: " << path << std::endl;
            return 0;
        }

        return tex.ID;
    }
};

#endif