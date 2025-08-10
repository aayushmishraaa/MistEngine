#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>
#include <fstream>

Texture::Texture() : m_ID(0), m_Width(0), m_Height(0), m_NrChannels(0) {}

Texture::~Texture() {
    if (m_ID) {
        glDeleteTextures(1, &m_ID);
    }
}

bool Texture::LoadFromFile(const std::string& path) {
    // Check if file exists first
    std::ifstream file(path);
    if (!file.good()) {
        std::cerr << "ERROR: Texture file does not exist: " << path << std::endl;
        return false;
    }
    file.close();

    // Clear any existing OpenGL errors
    while (glGetError() != GL_NO_ERROR) { /* clear error queue */ }

    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &m_Width, &m_Height, &m_NrChannels, 0);

    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        std::cerr << "STB Image error: " << stbi_failure_reason() << std::endl;
        return false;
    }

    std::cout << "Loading texture: " << path << " (" << m_Width << "x" << m_Height << ", channels: " << m_NrChannels << ")" << std::endl;

    // Delete existing texture if any
    if (m_ID != 0) {
        glDeleteTextures(1, &m_ID);
        m_ID = 0;
    }

    glGenTextures(1, &m_ID);
    if (m_ID == 0) {
        std::cerr << "ERROR: Failed to generate texture ID for: " << path << std::endl;
        stbi_image_free(data);
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, m_ID);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Determine format
    GLenum format = GL_RGB;
    GLenum internalFormat = GL_RGB;
    if (m_NrChannels == 1) {
        format = GL_RED;
        internalFormat = GL_RED;
    } else if (m_NrChannels == 3) {
        format = GL_RGB;
        internalFormat = GL_RGB;
    } else if (m_NrChannels == 4) {
        format = GL_RGBA;
        internalFormat = GL_RGBA;
    }

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, data);
    
    // Check for errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "ERROR: OpenGL error uploading texture " << path << ": " << error << std::endl;
        stbi_image_free(data);
        glDeleteTextures(1, &m_ID);
        m_ID = 0;
        return false;
    }

    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Check mipmap generation
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "WARNING: OpenGL error generating mipmaps for " << path << ": " << error << std::endl;
        // Continue anyway, mipmaps are not critical
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    
    std::cout << "Successfully loaded texture: " << path << " (ID: " << m_ID << ")" << std::endl;
    return true;
}

void Texture::Bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_ID);
}