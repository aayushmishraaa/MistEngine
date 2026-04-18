#pragma once
#ifndef MIST_GL_RENDERING_DEVICE_H
#define MIST_GL_RENDERING_DEVICE_H

#include "Renderer/RenderingDevice.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <unordered_map>

// OpenGL-backed RenderingDevice. First concrete implementation — thin
// shim over glCreateXxx / glDeleteXxx. Migrated subsystems use this via
// the process-wide `Mist::GPU::Device()` accessor; non-migrated sites
// still call raw GL.
namespace Mist::GPU {

class GLRenderingDevice : public RenderingDevice {
public:
    RID  CreateTexture(const TextureDesc&)           override;
    RID  CreateTextureArray(const TextureArrayDesc&) override;
    RID  CreateBuffer(const BufferDesc&)             override;
    RID  CreateShader(const ShaderDesc&)             override;
    RID  CreateShaderProgram(const ProgramDesc&)     override;
    void Destroy(RID)                                override;
    const char* GetBackendName() const               override { return "OpenGL 4.6"; }

    // Non-virtual bridge used by migrated subsystems that still need to
    // hand the raw GL handle to bind/draw entry points (those aren't in
    // the abstract interface yet). Returns 0 for invalid/unknown RIDs.
    std::uint32_t GetGLHandle(RID) const;

private:
    // Kind is stored so Destroy routes to the correct glDelete* call
    // without callers having to remember what they created.
    enum class Kind : std::uint8_t {
        None = 0,
        Texture,
        TextureArray,
        Buffer,
        Shader,
        Program,
    };

    struct Entry {
        Kind          kind     = Kind::None;
        std::uint32_t glHandle = 0;   // GLuint; kept as uint32 so this header doesn't include glad
    };

    mutable std::mutex                       m_Mutex;
    std::atomic<std::uint64_t>               m_NextId{1};
    std::unordered_map<std::uint64_t, Entry> m_Live;
};

} // namespace Mist::GPU

#endif // MIST_GL_RENDERING_DEVICE_H
