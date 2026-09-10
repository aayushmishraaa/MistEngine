#include "Assets/MaterialSerializer.h"

#include "Core/Logger.h"
#include "Core/PathGuard.h"
#include "Core/Reflection.h"
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

// Reflection-driven field write. Dispatched on PropertyType so adding
// a new reflected field in Material.h serialises for free.
void writeField(json& j, const PBRMaterial& mat, const Mist::PropertyInfo& p) {
    const auto* base = reinterpret_cast<const char*>(&mat);
    const void* field = base + p.offset;

    switch (p.type) {
        case Mist::PropertyType::Bool:
            j[p.name] = *reinterpret_cast<const bool*>(field);
            break;
        case Mist::PropertyType::Int:
            j[p.name] = *reinterpret_cast<const int*>(field);
            break;
        case Mist::PropertyType::Enum:
            // Persisted as the ordinal. Read back through set_enum_value so
            // the field's real width is respected.
            j[p.name] = Mist::enum_value(field, p.size);
            break;
        case Mist::PropertyType::Float:
            j[p.name] = *reinterpret_cast<const float*>(field);
            break;
        case Mist::PropertyType::Vec2: {
            const auto* v = reinterpret_cast<const glm::vec2*>(field);
            j[p.name] = {v->x, v->y};
            break;
        }
        case Mist::PropertyType::Vec3: {
            const auto* v = reinterpret_cast<const glm::vec3*>(field);
            j[p.name] = {v->x, v->y, v->z};
            break;
        }
        case Mist::PropertyType::Vec4: {
            const auto* v = reinterpret_cast<const glm::vec4*>(field);
            j[p.name] = {v->x, v->y, v->z, v->w};
            break;
        }
        case Mist::PropertyType::String:
            j[p.name] = *reinterpret_cast<const std::string*>(field);
            break;
        default:
            break;
    }
}

void readField(const json& j, PBRMaterial& mat, const Mist::PropertyInfo& p) {
    auto it = j.find(p.name);
    if (it == j.end()) return;

    auto* base = reinterpret_cast<char*>(&mat);
    void* field = base + p.offset;

    try {
        switch (p.type) {
            case Mist::PropertyType::Bool:
                *reinterpret_cast<bool*>(field) = it->get<bool>();
                break;
            case Mist::PropertyType::Int:
                *reinterpret_cast<int*>(field) = it->get<int>();
                break;
            case Mist::PropertyType::Enum:
                Mist::set_enum_value(field, p.size, it->get<long long>());
                break;
            case Mist::PropertyType::Float:
                *reinterpret_cast<float*>(field) = it->get<float>();
                break;
            case Mist::PropertyType::Vec2:
                if (it->is_array() && it->size() >= 2) {
                    auto* v = reinterpret_cast<glm::vec2*>(field);
                    v->x = (*it)[0].get<float>();
                    v->y = (*it)[1].get<float>();
                }
                break;
            case Mist::PropertyType::Vec3:
                if (it->is_array() && it->size() >= 3) {
                    auto* v = reinterpret_cast<glm::vec3*>(field);
                    v->x = (*it)[0].get<float>();
                    v->y = (*it)[1].get<float>();
                    v->z = (*it)[2].get<float>();
                }
                break;
            case Mist::PropertyType::Vec4:
                if (it->is_array() && it->size() >= 4) {
                    auto* v = reinterpret_cast<glm::vec4*>(field);
                    v->x = (*it)[0].get<float>();
                    v->y = (*it)[1].get<float>();
                    v->z = (*it)[2].get<float>();
                    v->w = (*it)[3].get<float>();
                }
                break;
            case Mist::PropertyType::String:
                *reinterpret_cast<std::string*>(field) = it->get<std::string>();
                break;
            default:
                break;
        }
    } catch (const std::exception& e) {
        LOG_WARN("MaterialSerializer: failed to read field '", p.name, "': ", e.what());
    }
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
