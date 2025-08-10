#include "Mesh.h"
#include <glad/glad.h>
#include <iostream>

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::vector<Texture>& textures)
    : vertices(vertices), indices(indices), textures(textures) {
    setupMesh();
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Mesh::Draw(Shader& shader) {
    // Bind textures
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    
    // Set default texture unit for the main diffuse texture that the fragment shader expects
    shader.setInt("diffuseTexture", 1);  // Default to texture unit 1
    
    for (unsigned int i = 0; i < textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i + 1);  // Start from texture unit 1 (unit 0 is reserved for shadow map)
        
        std::string number;
        std::string name = textures[i].type;
        if (name == "texture_diffuse") {
            number = std::to_string(diffuseNr++);
            // Set the main diffuse texture uniform that the fragment shader expects
            shader.setInt("diffuseTexture", i + 1);
        } else if (name == "texture_specular") {
            number = std::to_string(specularNr++);
        }

        // Also set the detailed texture uniform for advanced shaders
        shader.setInt((name + number).c_str(), i + 1);
        textures[i].Bind();
    }
    
    // If no diffuse texture was found, ensure we have a fallback
    if (diffuseNr == 1) {
        // No diffuse textures found, but shader still expects diffuseTexture uniform
        // The fragment shader will use whatever is bound to the default texture unit
        std::cout << "Warning: No diffuse texture found for mesh, using default" << std::endl;
    }

    // Bind and draw
    glBindVertexArray(VAO);
    
    // Quick error check only in debug builds
    #ifdef _DEBUG
    if (VAO == 0 || VBO == 0 || EBO == 0 || indices.empty()) {
        std::cout << "ERROR: Mesh::Draw - Invalid OpenGL state!" << std::endl;
        return;
    }
    #endif
    
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    
    // Check for OpenGL errors after drawing
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "WARNING: OpenGL error after drawing mesh: " << error << std::endl;
    }
    
    glBindVertexArray(0);
    
    // Reset to default texture
    glActiveTexture(GL_TEXTURE0);
}

void Mesh::setupMesh() {
    // Validate input data
    if (vertices.empty() || indices.empty()) {
        std::cout << "ERROR: Mesh::setupMesh - Empty vertices or indices!" << std::endl;
        return;
    }
    
    // Clear any existing OpenGL errors
    while (glGetError() != GL_NO_ERROR) { /* clear error queue */ }
    
    // Generate OpenGL objects
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    if (VAO == 0 || VBO == 0 || EBO == 0) {
        std::cout << "ERROR: Mesh::setupMesh - Failed to generate OpenGL objects!" << std::endl;
        return;
    }

    glBindVertexArray(VAO);
    
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    // Check for errors after vertex buffer upload
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "ERROR: OpenGL error after vertex buffer upload: " << error << std::endl;
        return;
    }

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Check for errors after index buffer upload
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "ERROR: OpenGL error after index buffer upload: " << error << std::endl;
        return;
    }

    // Setup vertex attributes
    // Vertex Positions (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "ERROR: OpenGL error setting position attribute: " << error << std::endl;
        return;
    }
    
    // Vertex Normals (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "ERROR: OpenGL error setting normal attribute: " << error << std::endl;
        return;
    }
    
    // Vertex Texture Coords (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    // Final error check
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "ERROR: OpenGL error during Mesh::setupMesh: " << error << std::endl;
        // Don't return here, unbind properly
    } else {
        std::cout << "Mesh setup successful: " << vertices.size() << " vertices, " << indices.size() << " indices" << std::endl;
    }

    // Always unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
