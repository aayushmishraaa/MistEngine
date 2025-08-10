#include "Model.h"
#include <iostream>
#include <fstream>
#include <cfloat>

Model::Model(const std::string& path) {
    std::cout << "[DEBUG] Model constructor called with path: " << path << std::endl;
    loadModel(path);
}

Model::~Model() {
    // Meshes and textures are handled by their respective destructors
}

void Model::Draw(Shader& shader) {
    // The model matrix is set by the caller (Renderer) before calling Model::Draw
    // Iterate through meshes and draw them
    if (meshes.empty()) {
        std::cout << "[DEBUG] Model::Draw - No meshes to draw!" << std::endl;
        return;
    }
    
    std::cout << "[DEBUG] Model::Draw - Drawing " << meshes.size() << " meshes" << std::endl;
    
    for (unsigned int i = 0; i < meshes.size(); i++) {
        std::cout << "[DEBUG] Drawing mesh " << i << " with " << meshes[i].vertices.size() << " vertices" << std::endl;
        meshes[i].Draw(shader);
    }
}

void Model::loadModel(const std::string& path) {
    std::cout << "[DEBUG] Attempting to load model: " << path << std::endl;
    
    // Check if file exists
    std::ifstream file(path);
    if (file.good()) {
        std::cout << "[DEBUG] File exists and is accessible" << std::endl;
        file.close();
    } else {
        std::cout << "[DEBUG] ERROR: File does not exist or is not accessible: " << path << std::endl;
        return;
    }
    
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiPostProcessSteps::aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        std::cout << "[DEBUG] Model loading FAILED for: " << path << std::endl;
        return;
    }
    
    std::cout << "[DEBUG] Model loaded successfully!" << std::endl;
    std::cout << "[DEBUG] Number of meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "[DEBUG] Number of materials: " << scene->mNumMaterials << std::endl;
    std::cout << "[DEBUG] Root node name: " << (scene->mRootNode->mName.length > 0 ? scene->mRootNode->mName.C_Str() : "Unnamed") << std::endl;
    std::cout << "[DEBUG] Root node has " << scene->mRootNode->mNumChildren << " children" << std::endl;
    
    directory = path.substr(0, path.find_last_of('/'));
    std::cout << "[DEBUG] Directory set to: " << directory << std::endl;

    processNode(scene->mRootNode, scene);
    
    std::cout << "[DEBUG] Final mesh count: " << meshes.size() << std::endl;
    
    if (meshes.empty()) {
        std::cout << "[DEBUG] ERROR: No meshes were created from the model!" << std::endl;
    } else {
        std::cout << "[DEBUG] SUCCESS: Model loaded with " << meshes.size() << " meshes" << std::endl;
        // Print info about each mesh
        for (size_t i = 0; i < meshes.size(); ++i) {
            std::cout << "[DEBUG] Mesh " << i << ": " << meshes[i].vertices.size() << " vertices, " 
                      << meshes[i].indices.size() << " indices" << std::endl;
        }
    }
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    std::cout << "[DEBUG] Processing node with " << node->mNumMeshes << " meshes" << std::endl;
    
    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        std::cout << "[DEBUG] Processing mesh " << i << " with " << mesh->mNumVertices << " vertices" << std::endl;
        meshes.push_back(processMesh(mesh, scene));
    }
    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    std::cout << "[DEBUG] Processing mesh with " << mesh->mNumVertices << " vertices and " << mesh->mNumFaces << " faces" << std::endl;
    std::cout << "[DEBUG] Mesh name: " << (mesh->mName.length > 0 ? mesh->mName.C_Str() : "Unnamed") << std::endl;

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        glm::vec3 vector;
        // positions
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;
        // normals
        if (mesh->mNormals) {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        } else {
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f); // Default up normal
        }
        // texture coordinates
        if (mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        } else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }
        vertices.push_back(vertex);
        
        // Debug: Print first few vertices to understand the geometry
        if (i < 3) {
            std::cout << "[DEBUG] Vertex " << i << ": pos(" << vertex.Position.x << ", " << vertex.Position.y << ", " << vertex.Position.z << ")" << std::endl;
        }
    }

    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        if (face.mNumIndices != 3) {
            std::cout << "[DEBUG] Warning: Face " << i << " has " << face.mNumIndices << " indices (expected 3)" << std::endl;
        }
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Calculate bounding box for debugging
    glm::vec3 minBounds = glm::vec3(FLT_MAX);
    glm::vec3 maxBounds = glm::vec3(-FLT_MAX);
    for (const auto& vertex : vertices) {
        minBounds = glm::min(minBounds, vertex.Position);
        maxBounds = glm::max(maxBounds, vertex.Position);
    }
    std::cout << "[DEBUG] Mesh bounds: min(" << minBounds.x << ", " << minBounds.y << ", " << minBounds.z << "), max(" << maxBounds.x << ", " << maxBounds.y << ", " << maxBounds.z << ")" << std::endl;
    glm::vec3 size = maxBounds - minBounds;
    std::cout << "[DEBUG] Mesh size: " << size.x << " x " << size.y << " x " << size.z << std::endl;

    // Process material
    if (mesh->mMaterialIndex >= 0) {
        std::cout << "[DEBUG] Processing material index: " << mesh->mMaterialIndex << std::endl;
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        // diffuse maps
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // specular maps
        std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        
        std::cout << "[DEBUG] Loaded " << textures.size() << " textures for this mesh" << std::endl;
    }

    std::cout << "[DEBUG] Created mesh with " << vertices.size() << " vertices, " << indices.size() << " indices, " << textures.size() << " textures" << std::endl;
    return Mesh(vertices, indices, textures);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName) {
    std::vector<Texture> textures;
    unsigned int textureCount = mat->GetTextureCount(type);
    std::cout << "[DEBUG] Loading " << textureCount << " textures of type " << typeName << std::endl;
    
    for (unsigned int i = 0; i < textureCount; i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::cout << "[DEBUG] Texture filename: " << str.C_Str() << std::endl;
        
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
            std::string fullPath = directory + "/" + str.C_Str();
            std::cout << "[DEBUG] Attempting to load texture: " << fullPath << std::endl;
            
            // Assuming Texture class has a LoadFromFile method that takes directory and filename
            if (texture.LoadFromFile(fullPath)) {
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
                std::cout << "[DEBUG] Successfully loaded texture: " << fullPath << std::endl;
            } else {
                std::cerr << "Warning: Failed to load texture: " << fullPath << std::endl;
            }
        }
    }
    return textures;
}
