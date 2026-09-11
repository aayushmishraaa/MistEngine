#include "Assets/PrefabSerializer.h"

#include "Core/Logger.h"
#include "Core/PathGuard.h"
#include "Core/ReflectionJson.h"
#include "ECS/Components/HierarchyComponent.h"
#include "ECS/Components/NameComponent.h"
#include "ECS/Components/PrefabComponents.h"
#include "ECS/EntityName.h"
#include "ECS/Systems/HierarchySystem.h"
#include "Scene/ComponentBlocks.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

using json = nlohmann::json;

namespace Mist::Assets {

namespace {

constexpr const char* kPrefabType    = "MistPrefab";
constexpr const char* kPrefabVersion = "1.0";

// Same cap the scene serializer uses. A legitimate prefab is far smaller.
constexpr std::uintmax_t kMaxPrefabBytes = 64ull * 1024ull * 1024ull;

// Prefabs are project assets, so they live under the project root — the same
// boundary PathGuard enforces for materials, scenes and packages.
//
// Guarded from the first commit rather than retrofitted: materials and
// packages both had to be fixed after the fact at commit 04b00b4, and both
// paths were reachable from untrusted input in the meantime. A prefab path
// arrives from a free-text field and from asset drops, so it is the same
// exposure.
std::filesystem::path ResolvePrefabPath(const std::string& path) {
    return Mist::PathGuard::resolve_under(Mist::PathGuard::project_root(), path);
}

const std::vector<Entity>* ChildrenOf(Coordinator& coord, Entity e) {
    if (!coord.HasComponent<HierarchyComponent>(e)) return nullptr;
    return &coord.GetComponent<HierarchyComponent>(e).children;
}

// Serializes one entity and its descendants.
//
// Returns false when two siblings share a name. That has to be fatal: an
// override is addressed by name path, so a duplicate makes "Turret/Barrel"
// ambiguous and the override would silently land on whichever child happened
// to come first in iteration order. Failing at save time turns a mystery into
// an error message.
bool WriteSubtree(json& out, Entity e, Coordinator& coord, const std::string& pathSoFar) {
    // An unnamed entity gets a stable synthesized name rather than being
    // rejected: most entities are never named, and refusing to save a subtree
    // because a child is untitled would make the feature unusable.
    const std::string name = Mist::HasEntityName(coord, e)
                           ? Mist::EntityName(coord, e)
                           : ("Node" + std::to_string(e));
    out["name"] = name;

    Mist::Scene::WriteComponentBlocks(out, e, coord);

    const auto* kids = ChildrenOf(coord, e);
    if (!kids || kids->empty()) return true;

    out["children"] = json::array();
    std::unordered_set<std::string> seen;

    for (Entity child : *kids) {
        json jc = json::object();
        const std::string childName = Mist::HasEntityName(coord, child)
                                    ? Mist::EntityName(coord, child)
                                    : ("Node" + std::to_string(child));

        if (!seen.insert(childName).second) {
            const std::string where = pathSoFar.empty() ? name : pathSoFar + "/" + name;
            LOG_ERROR("PrefabSerializer: '", where, "' has two children named '", childName,
                      "'. Sibling names must be unique or overrides addressed at that "
                      "path are ambiguous. Rename one and save again.");
            return false;
        }

        const std::string childPath = pathSoFar.empty() ? childName : pathSoFar + "/" + childName;
        if (!WriteSubtree(jc, child, coord, childPath)) return false;
        out["children"].push_back(std::move(jc));
    }
    return true;
}

// Rebuilds a subtree. `localPath` is relative to the instance root, so the
// root itself gets "".
Entity ReadSubtree(const json& in, Coordinator& coord, Entity instanceRoot,
                   const std::string& localPath) {
    if (!in.is_object()) return Mist::kInvalidEntity;

    Entity e = coord.CreateEntity();

    if (in.contains("name") && in["name"].is_string()) {
        Mist::SetEntityName(coord, e, in["name"].get<std::string>());
    }

    Mist::Scene::ReadComponentBlocks(in, e, coord);

    // Every spawned entity participates in the hierarchy — a prefab is a tree
    // by definition, and the root needs a HierarchyComponent so it can be
    // reparented into the scene.
    if (!coord.HasComponent<HierarchyComponent>(e)) {
        coord.AddComponent(e, HierarchyComponent{});
    }

    // The root's own membership marker is stamped by the caller, which is the
    // only place that knows its own id.
    if (instanceRoot != Mist::kInvalidEntity) {
        coord.AddComponent(e, PrefabMemberComponent{instanceRoot, localPath});
    }

    if (in.contains("children") && in["children"].is_array()) {
        for (const auto& jc : in["children"]) {
            if (!jc.is_object() || !jc.contains("name")) continue;
            const std::string childName = jc["name"].get<std::string>();
            const std::string childPath = localPath.empty()
                                        ? childName
                                        : localPath + "/" + childName;

            Entity child = ReadSubtree(jc, coord,
                                       instanceRoot == Mist::kInvalidEntity ? e : instanceRoot,
                                       childPath);
            if (child == Mist::kInvalidEntity) continue;
            HierarchySystem::Attach(coord, e, child);
        }
    }
    return e;
}

} // namespace

std::string PrefabSerializer::SaveToString(Entity root, Coordinator& coord, bool* ok) {
    json doc = {
        {"type",    kPrefabType},
        {"version", kPrefabVersion},
    };

    json jroot = json::object();
    const bool wrote = WriteSubtree(jroot, root, coord, {});
    if (ok) *ok = wrote;
    if (!wrote) return {};

    doc["root"] = std::move(jroot);
    return doc.dump(2);
}

bool PrefabSerializer::Save(Entity root, Coordinator& coord, const std::string& path) {
    if (!coord.GetLivingEntities().count(root)) {
        LOG_ERROR("PrefabSerializer: entity ", root, " is not alive; nothing to save");
        return false;
    }

    const auto resolved = ResolvePrefabPath(path);
    if (resolved.empty()) {
        LOG_ERROR("PrefabSerializer: refusing to write outside the project root: ", path);
        return false;
    }

    bool ok = false;
    const std::string text = SaveToString(root, coord, &ok);
    if (!ok) return false;   // WriteSubtree already logged why

    std::error_code ec;
    std::filesystem::create_directories(resolved.parent_path(), ec);

    std::ofstream out(resolved, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        LOG_ERROR("PrefabSerializer: failed to open for writing: ", resolved.string());
        return false;
    }
    out << text;

    LOG_INFO("Prefab saved: ", resolved.string());
    return true;
}

Entity PrefabSerializer::InstantiateFromString(const std::string& text, Coordinator& coord,
                                               const std::string& sourcePath) {
    if (text.size() > kMaxPrefabBytes) {
        LOG_ERROR("PrefabSerializer: prefab text exceeds cap (", text.size(), " > ",
                  kMaxPrefabBytes, ")");
        return Mist::kInvalidEntity;
    }

    json doc;
    try { doc = json::parse(text); }
    catch (const std::exception& e) {
        LOG_ERROR("PrefabSerializer: parse failed: ", e.what());
        return Mist::kInvalidEntity;
    }

    if (!doc.is_object() || !doc.contains("root") || !doc["root"].is_object()) {
        LOG_ERROR("PrefabSerializer: invalid prefab — missing 'root' object");
        return Mist::kInvalidEntity;
    }
    if (doc.value("type", std::string{}) != kPrefabType) {
        LOG_ERROR("PrefabSerializer: not a ", kPrefabType, " document");
        return Mist::kInvalidEntity;
    }
    if (doc.contains("version") && doc["version"].is_string()) {
        const auto v = doc["version"].get<std::string>();
        if (v != kPrefabVersion) {
            LOG_WARN("PrefabSerializer: loading v", v, " prefab into v", kPrefabVersion,
                     " engine — missing fields get defaults.");
        }
    }

    // kInvalidEntity as the instanceRoot tells ReadSubtree "you are the root";
    // it stamps descendants and leaves the root to us, since only we know the
    // id CreateEntity returned.
    Entity root = ReadSubtree(doc["root"], coord, Mist::kInvalidEntity, {});
    if (root == Mist::kInvalidEntity) return root;

    coord.AddComponent(root, PrefabMemberComponent{root, std::string{}});
    coord.AddComponent(root, PrefabInstanceComponent{sourcePath, "[]"});
    return root;
}

Entity PrefabSerializer::Instantiate(const std::string& path, Coordinator& coord) {
    const auto resolved = ResolvePrefabPath(path);
    if (resolved.empty()) {
        LOG_ERROR("PrefabSerializer: refusing to read outside the project root: ", path);
        return Mist::kInvalidEntity;
    }

    std::error_code ec;
    const auto size = std::filesystem::file_size(resolved, ec);
    if (ec) {
        LOG_ERROR("PrefabSerializer: failed to stat prefab: ", resolved.string());
        return Mist::kInvalidEntity;
    }
    if (size > kMaxPrefabBytes) {
        LOG_ERROR("PrefabSerializer: prefab exceeds cap (", size, " > ", kMaxPrefabBytes, "): ",
                  resolved.string());
        return Mist::kInvalidEntity;
    }

    std::ifstream in(resolved, std::ios::binary);
    if (!in.is_open()) {
        LOG_ERROR("PrefabSerializer: failed to open for reading: ", resolved.string());
        return Mist::kInvalidEntity;
    }
    const std::string text((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());

    // The path stored on the instance is the caller's, not the canonicalised
    // one: a scene should reference "prefabs/turret.mistprefab", not an
    // absolute path that breaks on another machine.
    return InstantiateFromString(text, coord, path);
}

} // namespace Mist::Assets
