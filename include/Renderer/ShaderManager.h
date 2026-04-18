#pragma once

#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Shader;

namespace Mist::Renderer {

// Central registry for every Shader the engine has loaded. Two jobs:
//
// 1. Hot reload. PollAndReload() walks the registered shaders, stats each
//    source file, and calls Shader::Reload() on any whose mtime advanced.
//    The existing Shader::Reload() was dead code before — nothing called it.
//
// 2. Single source of truth for shader paths. The call-site migration from
//    hardcoded "shaders/..." literals to ShaderManager::ResolvePath() is
//    intentionally gradual; new code should prefer the registry, existing
//    sites can stay on their literal until touched.
//
// Thread-safety: registration + PollAndReload take a mutex. Shaders are
// never compiled off-thread (OpenGL forbids it), so actual reloads always
// run on the caller's thread, which must hold the GL context.
class ShaderManager {
  public:
    static ShaderManager& Instance();

    // Opt a Shader into hot-reload tracking. The manager does not take
    // ownership; it only holds a non-owning pointer plus the source file
    // paths extracted via Shader::GetVertexPath/GetFragmentPath.
    void Register(Shader* shader);

    // Remove a shader before it's destroyed. Critical: if the shader's
    // owning object dies before the manager, the dangling pointer would
    // segfault the next PollAndReload().
    void Unregister(Shader* shader);

    // Walk the registry and reload any shader whose source file has a
    // newer mtime than the last-known value. Returns the number of shaders
    // actually reloaded this call — useful for logging / HUD.
    int PollAndReload();

    // Helper — resolves "pbr_vertex.glsl" to "shaders/pbr_vertex.glsl" or
    // wherever the runtime asset root is. Stays simple for now; a future
    // pass will let contributors configure multiple roots.
    static std::string ResolvePath(std::string_view name);

  private:
    ShaderManager() = default;

    struct TrackedShader {
        Shader* shader;
        std::filesystem::file_time_type lastVertexMTime{};
        std::filesystem::file_time_type lastFragmentMTime{};
        std::filesystem::file_time_type lastComputeMTime{};
    };

    static std::filesystem::file_time_type mtimeOrZero(const std::string& path);

    std::mutex m_Mutex;
    std::vector<TrackedShader> m_Tracked;
};

} // namespace Mist::Renderer
