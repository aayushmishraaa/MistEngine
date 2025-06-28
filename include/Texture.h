#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <string>

class Texture {
public:
    Texture();
    ~Texture();

    bool LoadFromFile(const std::string& path);
    void Bind(unsigned int unit = 0) const;
    unsigned int GetID() const { return m_ID; }

    std::string path;
    std::string type;
  

private:
    unsigned int m_ID;
    int m_Width, m_Height, m_NrChannels;
};

#endif