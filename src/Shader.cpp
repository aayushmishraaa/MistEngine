#include "Shader.h"
#include "Core/Logger.h"
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

std::string Shader::readFile(const char* path) {
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        file.open(path);
        std::stringstream stream;
        stream << file.rdbuf();
        file.close();
        return stream.str();
    } catch (std::ifstream::failure& e) {
        LOG_ERROR("SHADER::FILE_NOT_READ: ", path, " - ", e.what());
        return "";
    }
}

unsigned int Shader::compileShader(GLenum type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    const char* typeStr = (type == GL_VERTEX_SHADER) ? "VERTEX" :
                          (type == GL_FRAGMENT_SHADER) ? "FRAGMENT" :
                          (type == GL_GEOMETRY_SHADER) ? "GEOMETRY" : "COMPUTE";
    if (!checkCompileErrors(shader, typeStr)) {
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

Shader::Shader(const char* vertexPath, const char* fragmentPath)
    : ID(0), m_VertexPath(vertexPath), m_FragmentPath(fragmentPath) {
    std::string vertexCode = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);
    if (vertexCode.empty() || fragmentCode.empty()) return;

    unsigned int vertex = compileShader(GL_VERTEX_SHADER, vertexCode.c_str());
    if (!vertex) return;

    unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, fragmentCode.c_str());
    if (!fragment) { glDeleteShader(vertex); return; }

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    if (!checkCompileErrors(ID, "PROGRAM")) {
        glDeleteProgram(ID);
        ID = 0;
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::Shader(const char* computePath) : ID(0), m_ComputePath(computePath) {
    std::string code = readFile(computePath);
    if (code.empty()) return;

    unsigned int compute = compileShader(GL_COMPUTE_SHADER, code.c_str());
    if (!compute) return;

    ID = glCreateProgram();
    glAttachShader(ID, compute);
    glLinkProgram(ID);
    if (!checkCompileErrors(ID, "PROGRAM")) {
        glDeleteProgram(ID);
        ID = 0;
    }
    glDeleteShader(compute);
}

Shader::~Shader() {
    // Note: Not deleting here because Shader is often copied.
    // Resource management via ResourceManager handles cleanup.
}

void Shader::Reload() {
    m_UniformLocationCache.clear();
    if (ID) { glDeleteProgram(ID); ID = 0; }

    if (!m_ComputePath.empty()) {
        *this = Shader(m_ComputePath.c_str());
    } else if (!m_VertexPath.empty() && !m_FragmentPath.empty()) {
        *this = Shader(m_VertexPath.c_str(), m_FragmentPath.c_str());
    }
}

void Shader::use() const {
    glUseProgram(ID);
}

GLint Shader::getUniformLocation(const std::string& name) const {
    auto it = m_UniformLocationCache.find(name);
    if (it != m_UniformLocationCache.end()) return it->second;
    GLint location = glGetUniformLocation(ID, name.c_str());
    m_UniformLocationCache[name] = location;
    return location;
}

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(getUniformLocation(name), (int)value);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setMat3(const std::string& name, const glm::mat3& mat) const {
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

bool Shader::checkCompileErrors(unsigned int shader, const std::string& type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            LOG_ERROR("SHADER_COMPILATION (", type, "): ", infoLog);
            return false;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            LOG_ERROR("PROGRAM_LINKING: ", infoLog);
            return false;
        }
    }
    return true;
}
