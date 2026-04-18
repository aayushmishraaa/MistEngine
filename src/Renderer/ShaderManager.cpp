#include "Renderer/ShaderManager.h"

#include "Shader.h"

#include "Core/Logger.h"

#include <algorithm>
#include <system_error>

namespace Mist::Renderer {

ShaderManager& ShaderManager::Instance() {
    static ShaderManager inst;
    return inst;
}

std::filesystem::file_time_type ShaderManager::mtimeOrZero(const std::string& path) {
    if (path.empty())
        return {};
    std::error_code ec;
    auto t = std::filesystem::last_write_time(path, ec);
    if (ec)
        return {};
    return t;
}

void ShaderManager::Register(Shader* shader) {
    if (!shader)
        return;
    std::lock_guard<std::mutex> lock(m_Mutex);
    TrackedShader t;
    t.shader = shader;
    t.lastVertexMTime = mtimeOrZero(shader->GetVertexPath());
    t.lastFragmentMTime = mtimeOrZero(shader->GetFragmentPath());
    // Compute path isn't exposed publicly today; treat missing as zero and
    // skip hot reload for compute shaders until a getter is added.
    t.lastComputeMTime = {};
    m_Tracked.push_back(t);
}

void ShaderManager::Unregister(Shader* shader) {
    if (!shader)
        return;
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Tracked.erase(std::remove_if(m_Tracked.begin(), m_Tracked.end(),
                                   [shader](const TrackedShader& t) { return t.shader == shader; }),
                    m_Tracked.end());
}

int ShaderManager::PollAndReload() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    int reloaded = 0;
    for (auto& t : m_Tracked) {
        if (!t.shader)
            continue;
        auto vt = mtimeOrZero(t.shader->GetVertexPath());
        auto ft = mtimeOrZero(t.shader->GetFragmentPath());
        const bool changed = (vt != t.lastVertexMTime) || (ft != t.lastFragmentMTime);
        if (!changed)
            continue;

        t.shader->Reload();
        t.lastVertexMTime = vt;
        t.lastFragmentMTime = ft;
        ++reloaded;
    }
    if (reloaded > 0) {
        LOG_INFO("ShaderManager: hot-reloaded ", reloaded, " shader(s)");
    }
    return reloaded;
}

std::string ShaderManager::ResolvePath(std::string_view name) {
    // Single runtime root today. Kept as a function (rather than a constant
    // concatenation at every call site) so we can later accept multiple
    // roots or a config-driven override without touching callers.
    std::string out;
    out.reserve(8 + name.size());
    out.append("shaders/");
    out.append(name);
    return out;
}

} // namespace Mist::Renderer
