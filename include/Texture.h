#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <string>
#include <stb_image.h> // Ensure you have stb_image.h for image loading

class Texture {
public:
    Texture() : m_ID(0), m_Width(0), m_Height(0), m_NrChannels(0) {}
    ~Texture() {
        if (m_ID != 0) {
            glDeleteTextures(1, &m_ID);
        }
    }

    bool LoadFromFile(const std::string& path) {
        // Load image data using stb_image
        stbi_set_flip_vertically_on_load(true); // Flip image vertically
        unsigned char* data = stbi_load(path.c_str(), &m_Width, &m_Height, &m_NrChannels, 0);
        if (!data) {
            std::cerr << "Failed to load texture: " << path << std::endl;
            return false;
        }

        // Generate and bind texture
        glGenTextures(1, &m_ID);
        glBindTexture(GL_TEXTURE_2D, m_ID);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload texture data
        GLenum format = (m_NrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Free image data and unbind texture
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);

        return true;
    }

    void Bind(unsigned int unit = 0) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_ID);
    }

    unsigned int GetID() const { return m_ID; }

private:
    unsigned int m_ID;
    int m_Width, m_Height, m_NrChannels;
};

#endif
