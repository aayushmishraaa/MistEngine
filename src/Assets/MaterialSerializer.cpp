#include "Assets/MaterialSerializer.h"

#include "Core/Logger.h"
#include "Core/PathGuard.h"
#include "Core/Reflection.h"
#include "Core/ReflectionJson.h"
#include "Material.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace Mist::Assets {

namespace {
// Materials are project assets, so they live under the project root — the same
// boundary PathGuard already enforces for scenes, exports and module loading.
//
// This was the third unguarded path boundary in the codebase: Save and Load
// took a caller-supplied string straight to ifstream/ofstream. Both are
// reachable from untrusted input — the Assets -> New Material dialog takes a
// free-text path, and `materialPath` on any RenderComponent inside a scene or
// an imported .mistpkg is resolved through here.
std::filesystem::path ResolveMaterialPath(const std::string& path) {
    return Mist::PathGuard::resolve_under(Mist::PathGuard::project_root(), path);
}
} // namespace

using json = nlohmann::json;

namespace {

// Field-at-a-time reflection codec, shared with SceneSerializer and every
// other serializer via Core/ReflectionJson.h. This file used to carry its
// own copy of the PropertyType switch.
inline void writeField(json& j, const PBRMaterial& mat, const Mist::PropertyInfo& p) {
    Mist::Reflect::WriteField(j, &mat, p);
}

inline void readField(const json& j, PBRMaterial& mat, const Mist::PropertyInfo& p) {
    Mist::Reflect::ReadField(j, &mat, p);
}

} // namespace

std::string MaterialSerializer::ToJsonString(const PBRMaterial& mat) {
    const auto* props = Mist::TypeRegistry::Instance().Get("PBRMaterial");
    if (!props) {
        LOG_ERROR("MaterialSerializer: PBRMaterial not in TypeRegistry — "
                  "Material.h reflection block missing?");
        return "{}";
    }
    json j;
    j["type"]    = "PBRMaterial";
    j["version"] = "1.0";
    for (const auto& p : *props) {
        writeField(j, mat, p);
    }
    return j.dump(2);
}

bool MaterialSerializer::Save(const PBRMaterial& mat, const std::string& path) {
    const auto resolved = ResolveMaterialPath(path);
    if (resolved.empty()) {
        LOG_ERROR("MaterialSerializer: refusing to write outside the project "
                  "root: ", path);
        return false;
    }
    std::error_code ec;
    std::filesystem::create_directories(resolved.parent_path(), ec);

    std::ofstream out(resolved, std::ios::binary | std::ios::trunc);
    if (!out) {
        LOG_ERROR("MaterialSerializer: cannot open for write: ", path);
        return false;
    }
    out << ToJsonString(mat);
    return out.good();
}

bool MaterialSerializer::Load(const std::string& path, PBRMaterial& out) {
    const auto resolved = ResolveMaterialPath(path);
    if (resolved.empty()) {
        LOG_ERROR("MaterialSerializer: refusing to read outside the project "
                  "root: ", path);
        out = PBRMaterial{};
        return false;
    }
    std::ifstream in(resolved, std::ios::binary);
    if (!in) {
        LOG_ERROR("MaterialSerializer: cannot open for read: ", path);
        out = PBRMaterial{};
        return false;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    json j;
    try {
        j = json::parse(ss.str());
    } catch (const std::exception& e) {
        LOG_ERROR("MaterialSerializer: JSON parse error in '", path, "': ", e.what());
        out = PBRMaterial{};
        return false;
    }

    const auto* props = Mist::TypeRegistry::Instance().Get("PBRMaterial");
    if (!props) return false;

    out = PBRMaterial{};
    for (const auto& p : *props) {
        readField(j, out, p);
    }
    // Textures now resolved from the reloaded path fields. Bind path
    // below expects the runtime handles populated — if Refresh fails
    // to find a texture on disk we silently leave the slot empty; PBR
    // shader falls back to the scalar parameters.
    out.Refresh();
    return true;
}

} // namespace Mist::Assets
