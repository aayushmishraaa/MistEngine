#include "Assets/PrefabOverrides.h"

#include "Core/Logger.h"
#include "Core/ReflectionJson.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/LightComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/PrefabComponents.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/TransformComponent.h"

#include <nlohmann/json.hpp>

#include <string>

using json = nlohmann::json;

namespace Mist::Assets {

namespace {

json parse_or_empty(const std::string& text) {
    if (text.empty()) return json::array();
    try {
        json j = json::parse(text);
        if (j.is_array()) return j;
    } catch (const std::exception& e) {
        LOG_WARN("PrefabOverrides: malformed override table (", e.what(),
                 "); treating as empty");
    }
    return json::array();
}

// Finds the entity at `target` within the instance rooted at `instanceRoot`.
//
// Walks PrefabMemberComponent::localPath rather than re-walking the hierarchy
// by name, because the membership marker is written at instantiate time from
// the prefab's own addressing. Re-deriving the path from live names would
// break the moment a user renames an instanced child — the override should
// follow the slot it was authored against, not the label.
Entity ResolveTarget(Entity instanceRoot, Coordinator& coord, const std::string& target) {
    if (target.empty()) return instanceRoot;

    for (Entity e : coord.GetLivingEntities()) {
        if (!coord.HasComponent<PrefabMemberComponent>(e)) continue;
        const auto& m = coord.GetComponent<PrefabMemberComponent>(e);
        if (m.instanceRoot == instanceRoot && m.localPath == target) return e;
    }
    return static_cast<Entity>(-1);
}

// Applies a partial reflected object to whichever component `type` names.
//
// The type switch is unavoidable: reflection gives field offsets within a
// type, but the ECS stores components in type-keyed arrays, so something has
// to turn a string into a GetComponent<T>() call. Kept to the components a
// prefab can actually carry.
bool ApplyToComponent(Entity e, Coordinator& coord, const std::string& type,
                      const json& fields) {
    const auto* props = Mist::TypeRegistry::Instance().Get(type.c_str());
    if (!props) {
        LOG_WARN("PrefabOverrides: unknown component type '", type, "'; skipping");
        return false;
    }

    auto apply = [&](auto* component) {
        if (!component) return false;
        Mist::Reflect::ReadFields(fields, component, *props);
        return true;
    };

    if (type == "TransformComponent" && coord.HasComponent<TransformComponent>(e)) {
        auto& t = coord.GetComponent<TransformComponent>(e);
        const bool done = apply(&t);
        // An overridden position must recompose the subtree, or the override
        // lands in the component and never reaches cachedGlobal.
        if (done) t.dirty = true;
        return done;
    }
    if (type == "RenderComponent" && coord.HasComponent<RenderComponent>(e)) {
        return apply(&coord.GetComponent<RenderComponent>(e));
    }
    if (type == "PhysicsComponent" && coord.HasComponent<PhysicsComponent>(e)) {
        auto& p = coord.GetComponent<PhysicsComponent>(e);
        const bool done = apply(&p);
        // Force a body rebuild: an overridden shape or mass is not applied to
        // an already-built Bullet body.
        if (done) { p.rigidBody = nullptr; p.shapeHash = 0; }
        return done;
    }
    if (type == "LightComponent" && coord.HasComponent<LightComponent>(e)) {
        return apply(&coord.GetComponent<LightComponent>(e));
    }
    if (type == "CameraComponent" && coord.HasComponent<CameraComponent>(e)) {
        return apply(&coord.GetComponent<CameraComponent>(e));
    }

    LOG_WARN("PrefabOverrides: entity has no ", type, "; override skipped");
    return false;
}

} // namespace

int ApplyPrefabOverrides(Entity instanceRoot, Coordinator& coord,
                         const std::string& overridesJson) {
    const json table = parse_or_empty(overridesJson);
    int applied = 0;

    for (const auto& entry : table) {
        if (!entry.is_object()) continue;
        const std::string target = entry.value("target",    std::string{});
        const std::string type   = entry.value("component", std::string{});
        if (type.empty() || !entry.contains("fields") || !entry["fields"].is_object()) continue;

        const Entity e = ResolveTarget(instanceRoot, coord, target);
        if (e == static_cast<Entity>(-1)) {
            // The prefab source lost this child. Dropping the override is the
            // right call — refusing to load the scene over a stale override
            // would make editing a prefab a scene-breaking operation.
            LOG_WARN("PrefabOverrides: target '", target, "' not found in instance ",
                     instanceRoot, "; override dropped");
            continue;
        }
        if (ApplyToComponent(e, coord, type, entry["fields"])) ++applied;
    }
    return applied;
}

std::string RecordPrefabOverride(const std::string& overridesJson,
                                 const std::string& target,
                                 const std::string& componentType,
                                 const std::string& fieldName,
                                 const void* componentInstance) {
    const auto* props = Mist::TypeRegistry::Instance().Get(componentType.c_str());
    if (!props || !componentInstance) return overridesJson;

    const Mist::PropertyInfo* info = nullptr;
    for (const auto& p : *props) {
        if (fieldName == p.name) { info = &p; break; }
    }
    if (!info) {
        LOG_WARN("PrefabOverrides: '", componentType, "' has no field '", fieldName, "'");
        return overridesJson;
    }

    json table = parse_or_empty(overridesJson);

    // Merge into the existing group for this (target, component) so repeated
    // edits to different fields of the same component do not accumulate one
    // entry each.
    for (auto& entry : table) {
        if (!entry.is_object()) continue;
        if (entry.value("target", std::string{}) != target)           continue;
        if (entry.value("component", std::string{}) != componentType) continue;
        if (!entry.contains("fields") || !entry["fields"].is_object()) {
            entry["fields"] = json::object();
        }
        Mist::Reflect::WriteField(entry["fields"], componentInstance, *info);
        return table.dump();
    }

    json fields = json::object();
    Mist::Reflect::WriteField(fields, componentInstance, *info);
    table.push_back(json{
        {"target",    target},
        {"component", componentType},
        {"fields",    std::move(fields)},
    });
    return table.dump();
}

std::string ClearPrefabOverride(const std::string& overridesJson,
                                const std::string& target,
                                const std::string& componentType,
                                const std::string& fieldName) {
    json table = parse_or_empty(overridesJson);
    json out   = json::array();

    for (auto& entry : table) {
        if (!entry.is_object()) continue;
        const bool match = entry.value("target", std::string{}) == target
                        && entry.value("component", std::string{}) == componentType;

        if (match && entry.contains("fields") && entry["fields"].is_object()) {
            entry["fields"].erase(fieldName);
            // Prune the group once its last field is reverted, so the stored
            // table does not accumulate empty husks.
            if (entry["fields"].empty()) continue;
        }
        out.push_back(entry);
    }
    return out.dump();
}

std::vector<PrefabOverrideEntry> ListPrefabOverrides(const std::string& overridesJson) {
    std::vector<PrefabOverrideEntry> list;
    const json table = parse_or_empty(overridesJson);

    for (const auto& entry : table) {
        if (!entry.is_object()) continue;
        if (!entry.contains("fields") || !entry["fields"].is_object()) continue;

        const std::string target = entry.value("target",    std::string{});
        const std::string type   = entry.value("component", std::string{});
        for (auto it = entry["fields"].begin(); it != entry["fields"].end(); ++it) {
            list.push_back(PrefabOverrideEntry{target, type, it.key()});
        }
    }
    return list;
}

bool PrefabLocalPath(Entity entity, Coordinator& coord, std::string& out) {
    if (!coord.HasComponent<PrefabMemberComponent>(entity)) return false;
    out = coord.GetComponent<PrefabMemberComponent>(entity).localPath;
    return true;
}

} // namespace Mist::Assets
