#include "Scene/SceneSerializer.h"

#include "Assets/PrefabOverrides.h"
#include "Assets/PrefabSerializer.h"
#include "ECS/Components/PrefabComponents.h"
#include "ECS/EntityName.h"
#include "Scene/ComponentBlocks.h"

#include "Core/Logger.h"
#include "Core/PathGuard.h"
#include "Core/Reflection.h"
#include "Core/ReflectionJson.h"
#include "Environment.h"
#include "ECS/Components/AnimationComponent.h"
#include "ECS/Components/HierarchyComponent.h"
#include "ECS/Components/LightComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/NameComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Coordinator.h"
#include "ECS/Systems/HierarchySystem.h"
#include "Mesh.h"
#include "Renderable.h"
#include "Resources/AssetRegistry.h"
#include "Resources/Ref.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

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



// Reflection-driven component blocks delegate to the shared codec in
// Core/ReflectionJson.h. These two names are kept as thin aliases because
// the Save/Load bodies below read better with them than with the fully
// qualified calls.
inline void writeReflectedFields(json& j, const void* obj, const Mist::PropertyList& props) {
    Mist::Reflect::WriteFields(j, obj, props);
}

inline void readReflectedFields(const json& j, void* obj, const Mist::PropertyList& props) {
    Mist::Reflect::ReadFields(j, obj, props);
}

} // namespace

namespace {

bool ApplySceneJson(const json& root, int& entityCount, Environment* env);

// Serialise the whole living world to JSON. Split out of Save() so the same
// walk backs both the on-disk path and SaveToString(), which play mode uses
// to snapshot the scene without touching the filesystem.
json BuildSceneJson(const Environment* env) {
    json root = {
        {"version",  kSceneVersion},
        {"engine",   "MistEngine"},
        {"entities", json::array()},
    };


    // Iterate the authoritative living-entity set, not a dense 0..entityCount
    // range.
    //
    // `entityCount` is UIManager's m_EntityCounter, which only the UI creation
    // paths and SetEntityName ever bump — Lua's spawn_cube / spawn_plane /
    // spawn_light and SceneImporter never touched it. So saving the default
    // showcase scene, which is entirely Lua-authored, wrote almost nothing.
    // GetLivingEntities() is what the Hierarchy panel already uses for exactly
    // this reason.
    //
    // Sorted so scene files are stable and diffable across saves (the living
    // set is an unordered_set).
    std::vector<Entity> living(gCoordinator.GetLivingEntities().begin(),
                               gCoordinator.GetLivingEntities().end());
    std::sort(living.begin(), living.end());

    for (const Entity entity : living) {
        json e = json::object();
        e["id"] = static_cast<int>(entity);

        // Transform is the gatekeeper — any entity without one is skipped.
        if (!gCoordinator.HasComponent<TransformComponent>(entity)) continue;

        // Prefab instances are stored as a REFERENCE plus overrides, never as
        // the expanded subtree. That is the whole payoff: edit the prefab
        // source, reload the scene, and every instance picks up the change.
        // Flattening would make a prefab a one-time copy, which "Duplicate"
        // already was.
        //
        // So: skip every entity the instance spawned except its root, and
        // skip the root's component blocks too — its transform is carried as
        // an override if the user moved it.
        if (gCoordinator.HasComponent<PrefabMemberComponent>(entity)) {
            const auto& member = gCoordinator.GetComponent<PrefabMemberComponent>(entity);
            const bool isRoot = (member.instanceRoot == entity);
            if (!isRoot) continue;

            if (gCoordinator.HasComponent<PrefabInstanceComponent>(entity)) {
                const auto& inst = gCoordinator.GetComponent<PrefabInstanceComponent>(entity);
                json jp = {
                    {"path", inst.prefabPath},
                };
                // Stored as parsed JSON rather than an escaped string so the
                // scene file stays readable and diffable.
                try {
                    jp["overrides"] = inst.overridesJson.empty()
                                    ? json::array()
                                    : json::parse(inst.overridesJson);
                } catch (const std::exception&) {
                    LOG_WARN("SceneSerializer: instance ", entity,
                             " has a malformed override table; writing an empty one");
                    jp["overrides"] = json::array();
                }
                e["prefab"] = std::move(jp);

                if (gCoordinator.HasComponent<NameComponent>(entity)) {
                    const auto& n = gCoordinator.GetComponent<NameComponent>(entity);
                    if (!n.name.empty()) e["name"] = n.name;
                }
                // Hierarchy is still written: an instance can be parented to
                // something in the scene, and that link is scene state, not
                // prefab state.
                if (gCoordinator.HasComponent<HierarchyComponent>(entity)) {
                    const auto& h = gCoordinator.GetComponent<HierarchyComponent>(entity);
                    json jh = json::object();
                    if (h.parent != HierarchyComponent::kNoParent) {
                        jh["parent"] = static_cast<int>(h.parent);
                    }
                    e["hierarchy"] = jh;
                }
                root["entities"].push_back(std::move(e));
            }
            continue;
        }

        // Name. Written as a bare string rather than a reflected block: the
        // component has exactly one field and `"name": "Ground"` is what
        // SceneSerializer.h has documented as the format since v0.5.
        if (gCoordinator.HasComponent<NameComponent>(entity)) {
            const auto& n = gCoordinator.GetComponent<NameComponent>(entity);
            if (!n.name.empty()) e["name"] = n.name;
        }

        // Component payload. Shared with the prefab serializer so adding a
        // component updates one writer, not two.
        Mist::Scene::WriteComponentBlocks(e, entity, gCoordinator);

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

    // Environment. One per scene, reflection-driven like the component blocks
    // — adding a field to Environment.h serialises with no change here.
    if (env) {
        if (const auto* envProps = Mist::TypeRegistry::Instance().Get("Environment")) {
            json je = json::object();
            writeReflectedFields(je, env, *envProps);
            root["environment"] = je;
        }
    }

    return root;
}

} // namespace

bool SceneSerializer::Save(const std::string& filepath, Coordinator& /*coordinator*/,
                            int /*entityCount*/, const Environment* env) {
    const auto sandbox = SceneSandboxRoot();
    std::filesystem::path resolved;
    if (!Mist::PathGuard::is_under(sandbox, filepath, &resolved)) {
        LOG_ERROR("Refusing to save outside scene sandbox: ", filepath);
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(resolved.parent_path(), ec);

    const json root = BuildSceneJson(env);

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
                            int& entityCount, Environment* env) {
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

    return ApplySceneJson(root, entityCount, env);
}

std::string SceneSerializer::SaveToString(Coordinator& coordinator, const Environment* env) {
    (void)coordinator;
    return BuildSceneJson(env).dump();
}

bool SceneSerializer::LoadFromString(const std::string& text, Coordinator& /*coordinator*/,
                                     int& entityCount, Environment* env) {
    if (text.size() > kMaxSceneBytes) {
        LOG_ERROR("Scene text exceeds cap (", text.size(), " > ", kMaxSceneBytes, ")");
        return false;
    }
    json root;
    try { root = json::parse(text); }
    catch (const std::exception& e) {
        LOG_ERROR("Scene parse failed: ", e.what());
        return false;
    }
    return ApplySceneJson(root, entityCount, env);
}

namespace {

// Rebuild the world from parsed scene JSON. Split out of Load() so play
// mode's snapshot restore shares exactly one code path with file loading —
// a second implementation is how "Stop" and "Open Scene" drift apart.
bool ApplySceneJson(const json& root, int& entityCount, Environment* env) {
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


    // Clear the existing world first. Load used to add on top of whatever was
    // already there, so opening a scene duplicated the current one instead of
    // replacing it. Snapshot the id list before destroying — DestroyEntity
    // mutates the living set we'd otherwise be iterating.
    {
        std::vector<Entity> doomed(gCoordinator.GetLivingEntities().begin(),
                                   gCoordinator.GetLivingEntities().end());
        for (Entity e : doomed) gCoordinator.DestroyEntity(e);
        LOG_INFO("SceneSerializer: cleared ", doomed.size(), " existing entities");
    }

    // Saved ids are not the ids CreateEntity will hand back, so hierarchy links
    // have to be remapped. Pass 1 creates entities and records old->new; pass 2
    // wires parents once every id is known (a child can reference a parent that
    // appears later in the file).
    std::unordered_map<Entity, Entity> idRemap;
    std::vector<std::pair<Entity, Entity>> pendingParents;  // {child(new), parent(old)}

    entityCount = 0;
    for (const auto& e : root["entities"]) {
        // Prefab instance: re-instantiate from the source asset and apply the
        // stored overrides, rather than reading component blocks that were
        // never written. This is where propagation happens — the subtree comes
        // from the prefab as it is on disk NOW, so edits to the source reach
        // every instance.
        const bool isPrefabRef = e.contains("prefab") && e["prefab"].is_object()
                              && e["prefab"].contains("path")
                              && e["prefab"]["path"].is_string();

        Entity entity;
        if (isPrefabRef) {
            const std::string prefabPath = e["prefab"]["path"].get<std::string>();
            entity = Mist::Assets::PrefabSerializer::Instantiate(prefabPath, gCoordinator);
            if (entity == static_cast<Entity>(-1)) {
                // A missing or broken prefab must not take the rest of the
                // scene with it. Skip this entity and carry on; the warning
                // from Instantiate already names the path.
                LOG_WARN("SceneSerializer: prefab '", prefabPath,
                         "' failed to instantiate; skipping that instance");
                continue;
            }

            std::string overrides = "[]";
            if (e["prefab"].contains("overrides")) {
                overrides = e["prefab"]["overrides"].dump();
            }
            Mist::Assets::ApplyPrefabOverrides(entity, gCoordinator, overrides);
            if (gCoordinator.HasComponent<PrefabInstanceComponent>(entity)) {
                gCoordinator.GetComponent<PrefabInstanceComponent>(entity).overridesJson =
                    overrides;
            }
        } else {
            entity = gCoordinator.CreateEntity();
        }

        entityCount = std::max(entityCount, static_cast<int>(entity) + 1);
        if (e.contains("id") && e["id"].is_number_integer()) {
            idRemap[static_cast<Entity>(e["id"].get<int>())] = entity;
        }

        if (e.contains("name") && e["name"].is_string()) {
            Mist::SetEntityName(gCoordinator, entity, e["name"].get<std::string>());
        }

        if (!isPrefabRef) {
            Mist::Scene::ReadComponentBlocks(e, entity, gCoordinator);
        }

        if (e.contains("hierarchy") && e["hierarchy"].is_object()) {
            // Add an empty HierarchyComponent now; the parent link is applied
            // in pass 2 via HierarchySystem::Attach, which keeps both sides of
            // the relationship consistent. `children` is deliberately NOT read
            // back from the file — it is derived from the parent links, and
            // trusting both would let a hand-edited scene desynchronise them.
            if (!gCoordinator.HasComponent<HierarchyComponent>(entity)) {
                gCoordinator.AddComponent(entity, HierarchyComponent{});
            }
            if (e["hierarchy"].contains("parent") && e["hierarchy"]["parent"].is_number_integer()) {
                pendingParents.emplace_back(
                    entity, static_cast<Entity>(e["hierarchy"]["parent"].get<int>()));
            }
        }

    }

    // Environment. Absent block leaves `env` untouched, so a pre-Environment
    // scene keeps whatever the caller had rather than snapping to defaults.
    if (env && root.contains("environment")) {
        if (const auto* envProps = Mist::TypeRegistry::Instance().Get("Environment")) {
            readReflectedFields(root["environment"], env, *envProps);
        }
    }

    // Pass 2: wire hierarchy through the id remap.
    std::size_t attached = 0, orphaned = 0;
    for (const auto& [child, oldParent] : pendingParents) {
        auto it = idRemap.find(oldParent);
        if (it == idRemap.end()) {
            ++orphaned;  // parent id not present in the file; child stays a root
            continue;
        }
        if (HierarchySystem::Attach(gCoordinator, it->second, child)) ++attached;
    }
    if (orphaned > 0) {
        LOG_WARN("SceneSerializer: ", orphaned, " entity/entities referenced a "
                 "parent id not present in the file; loaded as roots");
    }

    LOG_INFO("Scene loaded (v", root.value("version", std::string("?")), ", ",
             root["entities"].size(), " entities, ", attached, " parented)");
    return true;
}

} // namespace
