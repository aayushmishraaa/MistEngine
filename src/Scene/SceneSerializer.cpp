#include "Scene/SceneSerializer.h"

#include "Core/Logger.h"
#include "Core/PathGuard.h"
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
#include <filesystem>
#include <fstream>
#include <string>

extern Coordinator gCoordinator;

using json = nlohmann::json;

namespace {

// Cap scene files to 64 MiB. Legitimate scenes are well under a megabyte;
// anything above this is almost certainly a mistake or an attempt to exhaust
// memory by forcing the whole contents into RAM.
constexpr std::uintmax_t kMaxSceneBytes = 64ull * 1024ull * 1024ull;

std::filesystem::path SceneSandboxRoot() {
    std::error_code ec;
    auto cwd = std::filesystem::current_path(ec);
    if (ec) return std::filesystem::path{"scenes"};
    return cwd / "scenes";
}

// Serialize a glm::vec3 as a 3-element JSON array.
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

// Best-effort classification for a Renderable* so we can write a useful
// mesh reference. Today we only track meshes loaded via AssetRegistry;
// anything else (Scene::CreateOwnedRenderable, dynamically-generated meshes)
// falls back to `{"builtin": "cube"}` so the entity at least has geometry
// on reload instead of silently disappearing.
json mesh_ref_for(Renderable* /*r*/) {
    // A future phase will walk AssetRegistry::meshes() to find the path for
    // the given renderable pointer. For now we emit the safe default.
    return json{{"builtin", "cube"}};
}

// Resolve a `"mesh"` JSON object into a Renderable* via AssetRegistry.
// Returns nullptr on bad input; the caller is responsible for deciding
// whether to skip the component or fail the load.
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
        // On-disk mesh loader lands with G4's later extension; for now emit
        // a clear warning and fall through to a placeholder cube so the
        // entity still renders.
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
        {"version",  "0.5"},
        {"engine",   "MistEngine"},
        {"entities", json::array()},
    };

    for (int i = 0; i < entityCount; ++i) {
        const Entity entity = static_cast<Entity>(i);

        json e = json::object();
        e["id"] = static_cast<int>(entity);

        // Transform is the gatekeeper — any entity without one is skipped.
        TransformComponent transform;
        try {
            transform = gCoordinator.GetComponent<TransformComponent>(entity);
        } catch (...) {
            continue;
        }
        e["transform"] = {
            {"pos",   vec3_to_json(transform.position)},
            {"rot",   vec3_to_json(transform.rotation)},
            {"scale", vec3_to_json(transform.scale)},
        };

        try {
            auto& render = gCoordinator.GetComponent<RenderComponent>(entity);
            e["render"] = {
                {"mesh",    mesh_ref_for(render.renderable)},
                {"visible", render.visible},
            };
        } catch (...) {
            // No render component — skip.
        }

        try {
            auto& physics = gCoordinator.GetComponent<PhysicsComponent>(entity);
            e["physics"] = {
                {"hasRigidBody",  physics.rigidBody != nullptr},
                {"syncTransform", physics.syncTransform},
            };
        } catch (...) {
            // No physics component — skip.
        }

        root["entities"].push_back(std::move(e));
    }

    std::ofstream out(resolved);
    if (!out.is_open()) {
        LOG_ERROR("Failed to open file for writing: ", filepath);
        return false;
    }
    out << root.dump(2);
    LOG_INFO("Scene saved to: ", resolved.string());
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
    try {
        in >> root;
    } catch (const std::exception& e) {
        LOG_ERROR("Scene parse failed: ", e.what());
        return false;
    }

    if (!root.is_object() || !root.contains("entities") || !root["entities"].is_array()) {
        LOG_ERROR("Invalid scene: missing or non-array 'entities'");
        return false;
    }

    entityCount = 0;
    for (const auto& e : root["entities"]) {
        Entity entity = gCoordinator.CreateEntity();
        entityCount = std::max(entityCount, static_cast<int>(entity) + 1);

        // Transform — required.
        if (e.contains("transform") && e["transform"].is_object()) {
            TransformComponent t;
            if (e["transform"].contains("pos"))
                vec3_from_json(e["transform"]["pos"], t.position);
            if (e["transform"].contains("rot"))
                vec3_from_json(e["transform"]["rot"], t.rotation);
            if (e["transform"].contains("scale"))
                vec3_from_json(e["transform"]["scale"], t.scale);
            gCoordinator.AddComponent(entity, t);
        }

        // Render — optional; resolves mesh via AssetRegistry so two entities
        // referencing the same path share the cached Mesh.
        if (e.contains("render") && e["render"].is_object()) {
            RenderComponent r;
            r.visible = e["render"].value("visible", true);
            if (e["render"].contains("mesh")) {
                r.renderable = resolve_mesh_ref(e["render"]["mesh"]);
            }
            gCoordinator.AddComponent(entity, r);
        }

        // Physics — optional; we keep the flag only. Reconstruction of the
        // rigid body is caller-driven (PhysicsSystem::CreateCube/etc).
        if (e.contains("physics") && e["physics"].is_object()) {
            PhysicsComponent p;
            p.syncTransform = e["physics"].value("syncTransform", true);
            gCoordinator.AddComponent(entity, p);
        }
    }

    LOG_INFO("Scene loaded from: ", resolved.string(),
             " (", root["entities"].size(), " entities)");
    return true;
}
