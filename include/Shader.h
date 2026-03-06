#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

class Shader {
public:
   unsigned int ID;
   Shader() : ID(0) {}
   Shader(const char* vertexPath, const char* fragmentPath);
   Shader(const char* computePath);
   ~Shader();

   void use() const;

   // Uniform setters (all use location cache)
   void setBool(const std::string& name, bool value) const;
   void setInt(const std::string& name, int value) const;
   void setFloat(const std::string& name, float value) const;
   void setVec2(const std::string& name, const glm::vec2& value) const;
   void setVec3(const std::string& name, const glm::vec3& value) const;
   void setVec4(const std::string& name, const glm::vec4& value) const;
   void setMat3(const std::string& name, const glm::mat3& mat) const;
   void setMat4(const std::string& name, const glm::mat4& mat) const;

   bool isValid() const { return ID != 0; }

   // Hot-reload support
   void Reload();
   std::string GetVertexPath() const { return m_VertexPath; }
   std::string GetFragmentPath() const { return m_FragmentPath; }

private:
   std::string m_VertexPath;
   std::string m_FragmentPath;
   std::string m_ComputePath;

   mutable std::unordered_map<std::string, GLint> m_UniformLocationCache;

   GLint getUniformLocation(const std::string& name) const;
   bool checkCompileErrors(unsigned int shader, const std::string& type);
   static std::string readFile(const char* path);
   unsigned int compileShader(GLenum type, const char* source);
};

#endif
