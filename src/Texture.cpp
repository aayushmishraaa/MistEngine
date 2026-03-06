#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>

Texture::Texture() : m_ID(0), m_Width(0), m_Height(0), m_NrChannels(0) {}

Texture::~Texture() {
    if (m_ID) {
        glDeleteTextures(1, &m_ID);
    }
}

bool Texture::LoadFromFile(const std::string& path, bool sRGB) {
    this->isSRGB = sRGB;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &m_Width, &m_Height, &m_NrChannels, 0);

    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return false;
    }

    glGenTextures(1, &m_ID);
    glBindTexture(GL_TEXTURE_2D, m_ID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = GL_RGB;
    GLenum internalFormat = GL_RGB8;

    if (m_NrChannels == 1) {
        format = GL_RED;
        internalFormat = GL_R8;
    } else if (m_NrChannels == 3) {
        format = GL_RGB;
        internalFormat = sRGB ? GL_SRGB8 : GL_RGB8;
    } else if (m_NrChannels == 4) {
        format = GL_RGBA;
        internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    return true;
}

void Texture::Bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_ID);
}
