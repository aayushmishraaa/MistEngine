#include "Model.h"
#include <iostream>

// C++14 compatible file existence check
#ifdef _WIN32
#include <windows.h>
inline bool file_exists(const std::string& path) {
    DWORD attributes = GetFileAttributesA(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}
#else
#include <sys/stat.h>
inline bool file_exists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}
#endif

Model::Model(const std::string& path) {
    std::cout << "Model: Attempting to load model from: " << path << std::endl;
    
    // Check if file exists before trying to load it
    if (!file_exists(path)) {
        std::cerr << "ERROR::MODEL::FILE_NOT_FOUND: " << path << std::endl;
        throw std::runtime_error("Model file not found: " + path);
    }
    
    // Initialize transform
    m_position = glm::vec3(0.0f, 0.0f, 0.0f);
    m_rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
    UpdateModelMatrix();
    
    loadModel(path);
    std::cout << "Model: Successfully loaded " << meshes.size() << " meshes from " << path << std::endl;
}

Model::~Model() {
    // Meshes and textures are handled by their respective destructors
}

void Model::Draw(Shader& shader) {
    // Set the model matrix uniform
    shader.setMat4("model", m_modelMatrix);
    
    // Iterate through meshes and draw them
    for (unsigned int i = 0; i < meshes.size(); i++) {
        meshes[i].Draw(shader);
    }
}

void Model::loadModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiPostProcessSteps::aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::string errorMsg = "ERROR::ASSIMP::" + std::string(importer.GetErrorString());
        std::cerr << errorMsg << std::endl;
        throw std::runtime_error(errorMsg);
    }
    
    // Extract directory from path for texture loading
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos) {
        lastSlash = path.find_last_of('\\');  // Windows path separator
    }
    
    if (lastSlash != std::string::npos) {
        directory = path.substr(0, lastSlash);
    } else {
        directory = ".";  // Current directory if no path separator found
    }
    
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
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
        if (mesh->HasNormals()) {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        } else {
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);  // Default up normal
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
    }

    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Process material
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        // diffuse maps
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // specular maps
        std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }

    return Mesh(vertices, indices, textures);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName) {
    std::vector<Texture> textures;
    unsigned int textureCount = mat->GetTextureCount(type);
    
    std::cout << "Loading " << textureCount << " textures of type: " << typeName << std::endl;
    
    for (unsigned int i = 0; i < textureCount; i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        
        std::string textureFile = str.C_Str();
        std::cout << "Processing texture file: " << textureFile << std::endl;
        
        bool skip = false;
        // Check if texture already loaded
        for (unsigned int j = 0; j < textures_loaded.size(); j++) {
            if (std::strcmp(textures_loaded[j].path.c_str(), textureFile.c_str()) == 0) {
                textures.push_back(textures_loaded[j]);
                skip = true;
                std::cout << "Texture already loaded, reusing: " << textureFile << std::endl;
                break;
            }
        }
        
        if (!skip) {
            Texture texture;
            
            // Try multiple possible paths for the texture
            std::vector<std::string> possiblePaths = {
                directory + "/" + textureFile,
                directory + "\\" + textureFile,
                textureFile,  // Try the raw filename
                "assets/models/backpack/" + textureFile,  // Common asset path
                "models/backpack/" + textureFile
            };
            
            bool loaded = false;
            for (const auto& texturePath : possiblePaths) {
                if (texture.LoadFromFile(texturePath)) {
                    texture.type = typeName;
                    texture.path = textureFile;  // Store original path
                    textures.push_back(texture);
                    textures_loaded.push_back(texture);
                    std::cout << "Model: Successfully loaded texture: " << texturePath << std::endl;
                    loaded = true;
                    break;
                }
            }
            
            if (!loaded) {
                std::cout << "Warning: Failed to load texture: " << textureFile << " (tried multiple paths)" << std::endl;
                
                // Create a default white texture to avoid rendering issues
                Texture defaultTexture;
                if (CreateDefaultTexture(defaultTexture)) {
                    defaultTexture.type = typeName;
                    defaultTexture.path = "default_white";
                    textures.push_back(defaultTexture);
                    std::cout << "Using default white texture instead" << std::endl;
                }
            }
        }
    }
    
    if (textures.empty() && (typeName == "texture_diffuse")) {
        // Always ensure we have at least a default diffuse texture
        Texture defaultTexture;
        if (CreateDefaultTexture(defaultTexture)) {
            defaultTexture.type = typeName;
            defaultTexture.path = "default_white";
            textures.push_back(defaultTexture);
            std::cout << "Created default diffuse texture for material without textures" << std::endl;
        }
    }
    
    return textures;
}

bool Model::CreateDefaultTexture(Texture& texture) {
    // Create a simple 2x2 white texture
    unsigned char whitePixels[] = {
        255, 255, 255, 255,  // White pixel
        255, 255, 255, 255,  // White pixel
        255, 255, 255, 255,  // White pixel
        255, 255, 255, 255   // White pixel
    };
    
    unsigned int textureID;
    glGenTextures(1, &textureID);
    if (textureID == 0) {
        return false;
    }
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Set the texture ID
    texture.SetID(textureID);
    
    return true;
}

void Model::SetTextureID(Texture& texture, unsigned int id) {
    texture.SetID(id);
}

void Model::UpdateModelMatrix() {
    m_modelMatrix = glm::mat4(1.0f);
    
    // Apply transformations: Scale, then Rotate, then Translate
    m_modelMatrix = glm::translate(m_modelMatrix, m_position);
    
    // Apply rotations in Y-X-Z order (Yaw, Pitch, Roll)
    m_modelMatrix = glm::rotate(m_modelMatrix, glm::radians(m_rotation.y), glm::vec3(0, 1, 0));
    m_modelMatrix = glm::rotate(m_modelMatrix, glm::radians(m_rotation.x), glm::vec3(1, 0, 0));
    m_modelMatrix = glm::rotate(m_modelMatrix, glm::radians(m_rotation.z), glm::vec3(0, 0, 1));
    
    m_modelMatrix = glm::scale(m_modelMatrix, m_scale);
}
