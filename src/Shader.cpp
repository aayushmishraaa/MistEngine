#include "Shader.h"
#include "Core/Logger.h"
#include "Renderer/RenderingDevice.h"
#include "Renderer/GLRenderingDevice.h"
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

Shader::Shader(const char* vertexPath, const char* fragmentPath)
    : ID(0), m_VertexPath(vertexPath), m_FragmentPath(fragmentPath) {
    auto* dev = static_cast<Mist::GPU::GLRenderingDevice*>(Mist::GPU::Device());
    if (!dev) { LOG_ERROR("Shader: no RenderingDevice active"); return; }

    std::string vertexCode   = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);
    if (vertexCode.empty() || fragmentCode.empty()) return;

    // Compile stages through the device. CreateShader doesn't surface a
    // compile-status return, so we inspect the raw GL handle afterwards to
    // preserve the "log and bail" behaviour existing code relies on.
    Mist::GPU::ShaderDesc vdesc{Mist::GPU::ShaderStage::Vertex,   vertexCode.c_str()};
    Mist::GPU::ShaderDesc fdesc{Mist::GPU::ShaderStage::Fragment, fragmentCode.c_str()};
    RID vrid = dev->CreateShader(vdesc);
    RID frid = dev->CreateShader(fdesc);
    GLuint vh = dev->GetGLHandle(vrid);
    GLuint fh = dev->GetGLHandle(frid);
    if (!checkCompileErrors(vh, "VERTEX") || !checkCompileErrors(fh, "FRAGMENT")) {
        dev->Destroy(vrid);
        dev->Destroy(frid);
        return;
    }

    Mist::GPU::ProgramDesc pdesc{vrid, frid, RID{}};
    m_ProgramRID = dev->CreateShaderProgram(pdesc);
    // Stages are detached inside CreateShaderProgram and no longer needed.
    dev->Destroy(vrid);
    dev->Destroy(frid);

    ID = dev->GetGLHandle(m_ProgramRID);
    if (!checkCompileErrors(ID, "PROGRAM")) {
        dev->Destroy(m_ProgramRID);
        m_ProgramRID = {};
        ID = 0;
    }
}

Shader::Shader(const char* computePath) : ID(0), m_ComputePath(computePath) {
    auto* dev = static_cast<Mist::GPU::GLRenderingDevice*>(Mist::GPU::Device());
    if (!dev) { LOG_ERROR("Shader: no RenderingDevice active"); return; }

    std::string code = readFile(computePath);
    if (code.empty()) return;

    Mist::GPU::ShaderDesc cdesc{Mist::GPU::ShaderStage::Compute, code.c_str()};
    RID crid = dev->CreateShader(cdesc);
    GLuint ch = dev->GetGLHandle(crid);
    if (!checkCompileErrors(ch, "COMPUTE")) {
        dev->Destroy(crid);
        return;
    }

    Mist::GPU::ProgramDesc pdesc{RID{}, RID{}, crid};
    m_ProgramRID = dev->CreateShaderProgram(pdesc);
    dev->Destroy(crid);

    ID = dev->GetGLHandle(m_ProgramRID);
    if (!checkCompileErrors(ID, "PROGRAM")) {
        dev->Destroy(m_ProgramRID);
        m_ProgramRID = {};
        ID = 0;
    }
}

Shader::~Shader() {
    if (m_ProgramRID.IsValid()) {
        if (auto* dev = Mist::GPU::Device()) dev->Destroy(m_ProgramRID);
        m_ProgramRID = {};
    }
    ID = 0;
}

Shader::Shader(Shader&& other) noexcept
    : ID(other.ID)
    , m_VertexPath(std::move(other.m_VertexPath))
    , m_FragmentPath(std::move(other.m_FragmentPath))
    , m_ComputePath(std::move(other.m_ComputePath))
    , m_ProgramRID(other.m_ProgramRID)
    , m_UniformLocationCache(std::move(other.m_UniformLocationCache)) {
    other.ID = 0;
    other.m_ProgramRID = {};
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_ProgramRID.IsValid()) {
            if (auto* dev = Mist::GPU::Device()) dev->Destroy(m_ProgramRID);
        }
        ID = other.ID;
        m_ProgramRID = other.m_ProgramRID;
        m_VertexPath = std::move(other.m_VertexPath);
        m_FragmentPath = std::move(other.m_FragmentPath);
        m_ComputePath = std::move(other.m_ComputePath);
        m_UniformLocationCache = std::move(other.m_UniformLocationCache);
        other.ID = 0;
        other.m_ProgramRID = {};
    }
    return *this;
}

void Shader::Reload() {
    // Build the replacement first; only swap on success so a failed reload
    // leaves the currently-running program bound and usable.
    Shader replacement;
    if (!m_ComputePath.empty()) {
        replacement = Shader(m_ComputePath.c_str());
    } else if (!m_VertexPath.empty() && !m_FragmentPath.empty()) {
        replacement = Shader(m_VertexPath.c_str(), m_FragmentPath.c_str());
    } else {
        return;
    }
    if (replacement.ID == 0) {
        return;
    }
    *this = std::move(replacement);
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
