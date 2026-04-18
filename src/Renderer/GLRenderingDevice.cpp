#include "Renderer/GLRenderingDevice.h"

#include <glad/glad.h>

namespace Mist::GPU {

namespace {

// Process-wide active device. Renderer::Init sets this after GL context
// creation; scene-side code reads via Device(). Raw pointer is fine:
// lifetime is owned by Renderer and outlives every subsystem that uses it.
RenderingDevice* g_active = nullptr;

GLenum toGLInternalFormat(TextureFormat f) {
    switch (f) {
        case TextureFormat::RGBA8:   return GL_RGBA8;
        case TextureFormat::RGBA16F: return GL_RGBA16F;
        case TextureFormat::R8:      return GL_R8;
        case TextureFormat::RG16F:   return GL_RG16F;
        case TextureFormat::R16F:    return GL_R16F;
        case TextureFormat::RGB16F:  return GL_RGB16F;
        case TextureFormat::DEPTH24:  return GL_DEPTH_COMPONENT24;
        case TextureFormat::DEPTH32F: return GL_DEPTH_COMPONENT32F;
    }
    return GL_RGBA8;
}

GLenum toGLBufferTarget(BufferUsage u) {
    switch (u) {
        case BufferUsage::Vertex:  return GL_ARRAY_BUFFER;
        case BufferUsage::Index:   return GL_ELEMENT_ARRAY_BUFFER;
        case BufferUsage::Uniform: return GL_UNIFORM_BUFFER;
        case BufferUsage::Storage: return GL_SHADER_STORAGE_BUFFER;
    }
    return GL_ARRAY_BUFFER;
}

GLenum toGLShaderStage(ShaderStage s) {
    switch (s) {
        case ShaderStage::Vertex:   return GL_VERTEX_SHADER;
        case ShaderStage::Fragment: return GL_FRAGMENT_SHADER;
        case ShaderStage::Compute:  return GL_COMPUTE_SHADER;
    }
    return GL_VERTEX_SHADER;
}

} // namespace

RenderingDevice* Device()                         { return g_active; }
void             SetDevice(RenderingDevice* dev)  { g_active = dev;  }

RID GLRenderingDevice::CreateTexture(const TextureDesc& desc) {
    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, desc.mipmaps ? 4 : 1,
                       toGLInternalFormat(desc.format),
                       static_cast<GLsizei>(desc.width),
                       static_cast<GLsizei>(desc.height));

    const RID rid{m_NextId.fetch_add(1)};
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Live[rid.id] = Entry{Kind::Texture, tex};
    return rid;
}

RID GLRenderingDevice::CreateTextureArray(const TextureArrayDesc& desc) {
    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &tex);
    glTextureStorage3D(tex, 1,
                       toGLInternalFormat(desc.format),
                       static_cast<GLsizei>(desc.width),
                       static_cast<GLsizei>(desc.height),
                       static_cast<GLsizei>(desc.layers));

    const RID rid{m_NextId.fetch_add(1)};
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Live[rid.id] = Entry{Kind::TextureArray, tex};
    return rid;
}

RID GLRenderingDevice::CreateBuffer(const BufferDesc& desc) {
    GLuint buf = 0;
    glCreateBuffers(1, &buf);
    if (desc.size_bytes > 0) {
        glNamedBufferData(buf,
                          static_cast<GLsizeiptr>(desc.size_bytes),
                          desc.initial,
                          GL_DYNAMIC_DRAW);
    }
    (void)toGLBufferTarget; // reserved for bindings; not used at create time

    const RID rid{m_NextId.fetch_add(1)};
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Live[rid.id] = Entry{Kind::Buffer, buf};
    return rid;
}

RID GLRenderingDevice::CreateShader(const ShaderDesc& desc) {
    GLuint sh = glCreateShader(toGLShaderStage(desc.stage));
    if (desc.source) {
        glShaderSource(sh, 1, &desc.source, nullptr);
        glCompileShader(sh);
    }
    const RID rid{m_NextId.fetch_add(1)};
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Live[rid.id] = Entry{Kind::Shader, sh};
    return rid;
}

RID GLRenderingDevice::CreateShaderProgram(const ProgramDesc& desc) {
    // Snapshot stage handles under the lock, then link lock-free so a
    // slow driver link doesn't stall other creates/destroys.
    GLuint vs = 0, fs = 0, cs = 0;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto resolve = [&](RID r) -> GLuint {
            if (!r.IsValid()) return 0;
            auto it = m_Live.find(r.id);
            return it == m_Live.end() ? 0u : it->second.glHandle;
        };
        vs = resolve(desc.vertex);
        fs = resolve(desc.fragment);
        cs = resolve(desc.compute);
    }

    GLuint prog = glCreateProgram();
    if (cs != 0) {
        glAttachShader(prog, cs);
        glLinkProgram(prog);
        glDetachShader(prog, cs);
    } else {
        if (vs) glAttachShader(prog, vs);
        if (fs) glAttachShader(prog, fs);
        glLinkProgram(prog);
        if (vs) glDetachShader(prog, vs);
        if (fs) glDetachShader(prog, fs);
    }

    const RID rid{m_NextId.fetch_add(1)};
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Live[rid.id] = Entry{Kind::Program, prog};
    return rid;
}

void GLRenderingDevice::Destroy(RID rid) {
    if (!rid.IsValid()) return;
    Entry e{};
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Live.find(rid.id);
        if (it == m_Live.end()) return;
        e = it->second;
        m_Live.erase(it);
    }

    switch (e.kind) {
        case Kind::Texture:      glDeleteTextures(1, &e.glHandle); break;
        case Kind::TextureArray: glDeleteTextures(1, &e.glHandle); break;
        case Kind::Buffer:       glDeleteBuffers(1, &e.glHandle);  break;
        case Kind::Shader:       glDeleteShader(e.glHandle);       break;
        case Kind::Program:      glDeleteProgram(e.glHandle);      break;
        case Kind::None:         break;
    }
}

std::uint32_t GLRenderingDevice::GetGLHandle(RID rid) const {
    if (!rid.IsValid()) return 0;
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Live.find(rid.id);
    return it == m_Live.end() ? 0u : it->second.glHandle;
}

} // namespace Mist::GPU
