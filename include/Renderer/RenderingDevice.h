#pragma once
#ifndef MIST_RENDERING_DEVICE_H
#define MIST_RENDERING_DEVICE_H

#include "Renderer/RID.h"

#include <cstddef>
#include <cstdint>

// Backend-agnostic GPU resource interface. Today there's a single
// implementation (`GLRenderingDevice`) that maps calls to the existing
// OpenGL paths; the interface exists so future Vulkan/D3D12/Metal
// backends slot in without touching scene-side code.
//
// The contract is intentionally narrow in this first cut — just the
// resources the current Renderer + post-process stack actually create.
// Expand it opportunistically as new subsystems migrate.

namespace Mist::GPU {

enum class TextureFormat : std::uint8_t {
    RGBA8 = 0,
    RGBA16F,
    R8,
    RG16F,    // TAA velocity buffer
    R16F,
    RGB16F,
    DEPTH24,
    DEPTH32F, // CSM cascade array (float depth → better precision far out)
};

struct TextureDesc {
    std::uint32_t  width  = 0;
    std::uint32_t  height = 0;
    TextureFormat  format = TextureFormat::RGBA8;
    bool           mipmaps = false;
};

// Array-layered texture (e.g. cascaded shadow maps). `layers` = 1
// collapses to a regular 2D texture — prefer TextureDesc for that case.
struct TextureArrayDesc {
    std::uint32_t  width  = 0;
    std::uint32_t  height = 0;
    std::uint32_t  layers = 1;
    TextureFormat  format = TextureFormat::DEPTH24;
};

enum class BufferUsage : std::uint8_t {
    Vertex = 0,
    Index,
    Uniform,
    Storage,
};

struct BufferDesc {
    std::size_t  size_bytes = 0;
    BufferUsage  usage      = BufferUsage::Vertex;
    const void*  initial    = nullptr;  // optional initial data
};

enum class ShaderStage : std::uint8_t {
    Vertex = 0,
    Fragment,
    Compute,
};

struct ShaderDesc {
    ShaderStage stage  = ShaderStage::Vertex;
    const char* source = nullptr;
};

// Linked shader program — takes already-compiled stage RIDs. Backends
// that don't have a separate link step (Vulkan, D3D12) can treat this
// as a pipeline-state hand-off.
struct ProgramDesc {
    RID vertex   = {};
    RID fragment = {};
    RID compute  = {};   // if valid, vertex/fragment must be invalid
};

class RenderingDevice {
public:
    virtual ~RenderingDevice() = default;

    virtual RID CreateTexture(const TextureDesc&)           = 0;
    virtual RID CreateTextureArray(const TextureArrayDesc&) = 0;
    virtual RID CreateBuffer(const BufferDesc&)             = 0;
    virtual RID CreateShader(const ShaderDesc&)             = 0;
    virtual RID CreateShaderProgram(const ProgramDesc&)     = 0;

    // Release a resource. No-op for invalid RIDs. After Destroy the RID is
    // reusable — implementations may recycle IDs or bump a generation to
    // detect stale references (G6 follow-up cycle).
    virtual void Destroy(RID) = 0;

    // Backend identifier for diagnostics + feature detection.
    virtual const char* GetBackendName() const = 0;
};

// Process-wide accessor for the active rendering device. Set once at
// engine startup in Renderer::Init and reset at Shutdown; subsystem
// code queries it to route GL creation/destruction through the right
// backend without taking a pointer on every class.
RenderingDevice* Device();
void SetDevice(RenderingDevice*);

} // namespace Mist::GPU

#endif // MIST_RENDERING_DEVICE_H
