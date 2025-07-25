#ifndef SHADER_H  
#define SHADER_H  

#include <glad/glad.h>  
#include <glm/glm.hpp>  
#include <string>  

class Shader {  
public:  
   unsigned int ID; // Add ID as a non-static data member  
   Shader() : ID(0) {}  
   Shader(const char* vertexPath, const char* fragmentPath);  
   void use();  
   void setMat4(const std::string& name, const glm::mat4& mat) const;  
   void setVec3(const std::string& name, const glm::vec3& value) const;  
   void setInt(const std::string& name, int value) const;  
   void setFloat(const std::string& name, float value) const;  

private:  
   void checkCompileErrors(unsigned int shader, const std::string& type);  
};  

#endif
