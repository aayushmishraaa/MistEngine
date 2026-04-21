#include "Scene/SceneSerializer.h"

#include "Core/Logger.h"
#include "Core/PathGuard.h"
#include "Core/Reflection.h"
#include "ECS/Components/AnimationComponent.h"
#include "ECS/Components/HierarchyComponent.h"
#include "ECS/Components/LightComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Coordinator.h"
#include "Mesh.h"
#include "Renderable.h"
#include "Resources/AssetRegistry.h"
#include "Resources/Ref.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

extern Coordinator gCoordinator;

using json = nlohmann::json;

namespace {

// Cap scene files to 64 MiB. Legitimate scenes are well under a megabyte;
// anything above this is almost certainly a mistake or a zip bomb.
constexpr std::uintmax_t kMaxSceneBytes = 64ull * 1024ull * 1024ull;

// Current scene format version. Bumped from 0.5 in the assets cycle
// to include full component coverage + hierarchy + animation state.
constexpr const char* kSceneVersion = "1.0";

std::filesystem::path SceneSandboxRoot() {
    std::error_code ec;
    auto cwd = std::filesystem::current_path(ec);
    if (ec) return std::filesystem::path{"scenes"};
    return cwd / "scenes";
}

json vec3_to_json(const glm::vec3& v) {
    return json::array({v.x, v.y, v.z});
}

bool vec3_from_json(const json& arr, glm::vec3& out) {
    if (!arr.is_array() || arr.size() != 3) return false;
    out.x = arr[0].get<float>();
    out.y = arr[1].get<float>();
    out.z = arr[2].get<float>();
    return true;
}

// Reflection-driven write: dispatches on PropertyType so a single
// walk of TypeRegistry::Get("X") serialises every reflected field of
// component X. Adds a field to MIST_REFLECT → it appears in JSON
// automatically next save.
void writeReflectedFields(json& j, const void* obj, const Mist::PropertyList& props) {
    const auto* base = reinterpret_cast<const char*>(obj);
    for (const auto& p : props) {
        const void* field = base + p.offset;
        switch (p.type) {
            case Mist::PropertyType::Bool:
                j[p.name] = *reinterpret_cast<const bool*>(field); break;
            case Mist::PropertyType::Int:
                j[p.name] = *reinterpret_cast<const int*>(field); break;
            case Mist::PropertyType::Float:
                j[p.name] = *reinterpret_cast<const float*>(field); break;
            case Mist::PropertyType::Vec2: {
                const auto* v = reinterpret_cast<const glm::vec2*>(field);
                j[p.name] = {v->x, v->y}; break;
            }
            case Mist::PropertyType::Vec3: {
                const auto* v = reinterpret_cast<const glm::vec3*>(field);
                j[p.name] = {v->x, v->y, v->z}; break;
            }
            case Mist::PropertyType::Vec4: {
                const auto* v = reinterpret_cast<const glm::vec4*>(field);
                j[p.name] = {v->x, v->y, v->z, v->w}; break;
            }
            case Mist::PropertyType::String:
                j[p.name] = *reinterpret_cast<const std::string*>(field); break;
            default: break;
        }
    }
}

void readReflectedFields(const json& j, void* obj, const Mist::PropertyList& props) {
    if (!j.is_object()) return;
    auto* base = reinterpret_cast<char*>(obj);
    for (const auto& p : props) {
        auto it = j.find(p.name);
        if (it == j.end()) continue;
        void* field = base + p.offset;
        try {
            switch (p.type) {
                case Mist::PropertyType::Bool:
                    *reinterpret_cast<bool*>(field) = it->get<bool>(); break;
                case Mist::PropertyType::Int:
                    *reinterpret_cast<int*>(field) = it->get<int>(); break;
                case Mist::PropertyType::Float:
                    *reinterpret_cast<float*>(field) = it->get<float>(); break;
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
                    *reinterpret_cast<std::string*>(field) = it->get<std::string>(); break;
                default: break;
            }
        } catch (const std::exception& e) {
            LOG_WARN("SceneSerializer: field '", p.name, "' read failed: ", e.what());
        }
    }
}

// Mesh refs. Static meshes have no source-file tracking yet, so we
// emit the safe default `builtin://cube` if the Renderable pointer
// doesn't match a known mapping. Imported rigged/scene meshes round-
// trip as an out-of-band `imports` section at the top of the scene
// (Phase E documented limitation: we don't re-emit per-entity for
// imports; the user re-imports the source file at load).
json mesh_ref_for(Renderable* /*r*/) {
    return json{{"builtin", "cube"}};
}

Renderable* resolve_mesh_ref(const json& meshJson) {
    if (!meshJson.is_object()) return nullptr;
    auto& registry = Mist::Assets::AssetRegistry::Instance();
    if (meshJson.contains("builtin") && meshJson["builtin"].is_string()) {
        std::string path = "builtin://" + meshJson["builtin"].get<std::string>();
        auto ref = LoadRef(registry.meshes(), path);
        return ref.get();
    }
    if (meshJson.contains("ext") && meshJson["ext"].is_string()) {
        const std::string& uri = meshJson["ext"].get_ref<const std::string&>();
        LOG_WARN("SceneSerializer: ext mesh refs not yet loadable: ", uri);
        auto ref = LoadRef(registry.meshes(), "builtin://cube");
        return ref.get();
    }
    return nullptr;
}

} // namespace

bool SceneSerializer::Save(const std::string& filepath, Coordinator& /*coordinator*/,
                            int entityCount) {
    const auto sandbox = SceneSandboxRoot();
    std::filesystem::path resolved;
    if (!Mist::PathGuard::is_under(sandbox, filepath, &resolved)) {
        LOG_ERROR("Refusing to save outside scene sandbox: ", filepath);
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(resolved.parent_path(), ec);

    json root = {
        {"version",  kSceneVersion},
        {"engine",   "MistEngine"},
        {"entities", json::array()},
    };

    const auto* lightProps     = Mist::TypeRegistry::Instance().Get("LightComponent");
    const auto* physicsProps   = Mist::TypeRegistry::Instance().Get("PhysicsComponent");
    const auto* renderProps    = Mist::TypeRegistry::Instance().Get("RenderComponent");

    for (int i = 0; i < entityCount; ++i) {
        const Entity entity = static_cast<Entity>(i);

        json e = json::object();
        e["id"] = static_cast<int>(entity);

        // Transform is the gatekeeper — any entity without one is skipped.
        if (!gCoordinator.HasComponent<TransformComponent>(entity)) continue;
        const auto& t = gCoordinator.GetComponent<TransformComponent>(entity);
        e["transform"] = {
            {"pos",   vec3_to_json(t.position)},
            {"rot",   vec3_to_json(t.rotation)},
            {"scale", vec3_to_json(t.scale)},
        };

        if (gCoordinator.HasComponent<RenderComponent>(entity)) {
            const auto& r = gCoordinator.GetComponent<RenderComponent>(entity);
            json jr = {{"mesh", mesh_ref_for(r.renderable)}};
            if (renderProps) writeReflectedFields(jr, &r, *renderProps);
            e["render"] = jr;
        }

        if (gCoordinator.HasComponent<PhysicsComponent>(entity)) {
            const auto& p = gCoordinator.GetComponent<PhysicsComponent>(entity);
            json jp = json::object();
            if (physicsProps) writeReflectedFields(jp, &p, *physicsProps);
            e["physics"] = jp;
        }

        if (gCoordinator.HasComponent<LightComponent>(entity)) {
            const auto& lc = gCoordinator.GetComponent<LightComponent>(entity);
            json jl = {{"type", static_cast<int>(lc.type)}};
            if (lightProps) writeReflectedFields(jl, &lc, *lightProps);
            e["light"] = jl;
        }

        if (gCoordinator.HasComponent<HierarchyComponent>(entity)) {
            const auto& h = gCoordinator.GetComponent<HierarchyComponent>(entity);
            json jh = json::object();
            if (h.parent != HierarchyComponent::kNoParent) {
                jh["parent"] = static_cast<int>(h.parent);
            }
            jh["children"] = json::array();
            for (Entity c : h.children) jh["children"].push_back(static_cast<int>(c));
            e["hierarchy"] = jh;
        }

        if (gCoordinator.HasComponent<AnimationComponent>(entity)) {
            const auto& ac = gCoordinator.GetComponent<AnimationComponent>(entity);
            e["animation"] = {
                {"currentClip", ac.currentAnimName},
                {"playbackSpeed", ac.playbackSpeed},
                {"playing", ac.playing},
                {"loop",    ac.loop},
            };
        }

        root["entities"].push_back(std::move(e));
    }

    std::ofstream out(resolved);
    if (!out.is_open()) {
        LOG_ERROR("Failed to open file for writing: ", filepath);
        return false;
    }
    out << root.dump(2);
    LOG_INFO("Scene saved to: ", resolved.string(),
             " (v", kSceneVersion, ", ", root["entities"].size(), " entities)");
    return true;
}

bool SceneSerializer::Load(const std::string& filepath, Coordinator& /*coordinator*/,
                            int& entityCount) {
    const auto sandbox = SceneSandboxRoot();
    std::filesystem::path resolved;
    if (!Mist::PathGuard::is_under(sandbox, filepath, &resolved)) {
        LOG_ERROR("Refusing to load outside scene sandbox: ", filepath);
        return false;
    }

    std::error_code ec;
    const auto fileSize = std::filesystem::file_size(resolved, ec);
    if (ec) {
        LOG_ERROR("Failed to stat scene file: ", filepath);
        return false;
    }
    if (fileSize > kMaxSceneBytes) {
        LOG_ERROR("Scene file exceeds cap (", fileSize, " > ", kMaxSceneBytes, "): ", filepath);
        return false;
    }

    std::ifstream in(resolved);
    if (!in.is_open()) {
        LOG_ERROR("Failed to open file for reading: ", filepath);
        return false;
    }

    json root;
    try { in >> root; }
    catch (const std::exception& e) {
        LOG_ERROR("Scene parse failed: ", e.what());
        return false;
    }

    if (!root.is_object() || !root.contains("entities") || !root["entities"].is_array()) {
        LOG_ERROR("Invalid scene: missing or non-array 'entities'");
        return false;
    }

    // Version warning. Old 0.5 scenes load — they just won't have
    // half the new fields populated, which defaults will cover.
    if (root.contains("version") && root["version"].is_string()) {
        std::string v = root["version"].get<std::string>();
        if (v != kSceneVersion) {
            LOG_WARN("SceneSerializer: loading v", v, " scene into v", kSceneVersion,
                     " engine — missing fields get defaults.");
        }
    }

    const auto* lightProps     = Mist::TypeRegistry::Instance().Get("LightComponent");
    const auto* physicsProps   = Mist::TypeRegistry::Instance().Get("PhysicsComponent");
    const auto* renderProps    = Mist::TypeRegistry::Instance().Get("RenderComponent");

    entityCount = 0;
    for (const auto& e : root["entities"]) {
        Entity entity = gCoordinator.CreateEntity();
        entityCount = std::max(entityCount, static_cast<int>(entity) + 1);

        if (e.contains("transform") && e["transform"].is_object()) {
            TransformComponent t;
            if (e["transform"].contains("pos"))   vec3_from_json(e["transform"]["pos"],   t.position);
            if (e["transform"].contains("rot"))   vec3_from_json(e["transform"]["rot"],   t.rotation);
            if (e["transform"].contains("scale")) vec3_from_json(e["transform"]["scale"], t.scale);
            gCoordinator.AddComponent(entity, t);
        }

        if (e.contains("render") && e["render"].is_object()) {
            RenderComponent r;
            if (renderProps) readReflectedFields(e["render"], &r, *renderProps);
            if (e["render"].contains("mesh")) {
                r.renderable = resolve_mesh_ref(e["render"]["mesh"]);
            }
            gCoordinator.AddComponent(entity, r);
        }

        if (e.contains("physics") && e["physics"].is_object()) {
            PhysicsComponent p;
            if (physicsProps) readReflectedFields(e["physics"], &p, *physicsProps);
            // rigidBody pointer stays null — ECSPhysicsSystem rebuilds
            // it next tick based on the loaded shape/mass params.
            p.rigidBody = nullptr;
            p.shapeHash = 0;
            gCoordinator.AddComponent(entity, p);
        }

        if (e.contains("light") && e["light"].is_object()) {
            LightComponent lc;
            if (e["light"].contains("type") && e["light"]["type"].is_number_integer()) {
                lc.type = static_cast<MistLightType>(e["light"]["type"].get<int>());
            }
            if (lightProps) readReflectedFields(e["light"], &lc, *lightProps);
            gCoordinator.AddComponent(entity, lc);
        }

        if (e.contains("hierarchy") && e["hierarchy"].is_object()) {
            HierarchyComponent h;
            if (e["hierarchy"].contains("parent") && e["hierarchy"]["parent"].is_number_integer()) {
                h.parent = static_cast<Entity>(e["hierarchy"]["parent"].get<int>());
            }
            if (e["hierarchy"].contains("children") && e["hierarchy"]["children"].is_array()) {
                for (const auto& cj : e["hierarchy"]["children"]) {
                    h.children.push_back(static_cast<Entity>(cj.get<int>()));
                }
            }
            gCoordinator.AddComponent(entity, h);
        }

        if (e.contains("animation") && e["animation"].is_object()) {
            AnimationComponent ac;
            ac.currentAnimName = e["animation"].value("currentClip", std::string{});
            ac.playbackSpeed   = e["animation"].value("playbackSpeed", 1.0f);
            ac.playing         = e["animation"].value("playing", false);
            ac.loop            = e["animation"].value("loop",    true);
            // availableClips is not serialised — those come from the
            // re-imported source model, not the scene file.
            gCoordinator.AddComponent(entity, ac);
        }
    }

    LOG_INFO("Scene loaded from: ", resolved.string(),
             " (v", root.value("version", std::string("?")), ", ",
             root["entities"].size(), " entities)");
    return true;
}
