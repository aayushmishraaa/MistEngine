#include "Scene/ComponentBlocks.h"

#include "Core/Logger.h"
#include "Mesh.h"
#include "Renderable.h"
#include "Resources/AssetRegistry.h"
#include "Resources/Ref.h"

#include <glm/vec3.hpp>

#include <string>

using json = nlohmann::json;

namespace Mist::Scene {

namespace {

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

const Mist::PropertyList* props_for(const char* type) {
    return Mist::TypeRegistry::Instance().Get(type);
}

} // namespace

json MeshRefFor(const RenderComponent& r) {
    // Empty meshPath records absence rather than guessing. This used to be a
    // function that ignored its argument and unconditionally returned
    // {"builtin":"cube"} — there was no way to recover what a bare
    // Renderable* pointed at — so every plane, sphere and imported model came
    // back as a cube.
    if (r.meshPath.empty()) return json::object();

    static constexpr const char* kBuiltinPrefix = "builtin://";
    if (r.meshPath.rfind(kBuiltinPrefix, 0) == 0) {
        return json{{"builtin", r.meshPath.substr(std::char_traits<char>::length(kBuiltinPrefix))}};
    }
    return json{{"ext", r.meshPath}};
}

Renderable* ResolveMeshRef(const json& meshJson) {
    if (!meshJson.is_object()) return nullptr;
    auto& registry = Mist::Assets::AssetRegistry::Instance();

    if (meshJson.contains("builtin") && meshJson["builtin"].is_string()) {
        const std::string path = "builtin://" + meshJson["builtin"].get<std::string>();
        auto ref = LoadRef(registry.meshes(), path);
        return ref.get();
    }
    if (meshJson.contains("ext") && meshJson["ext"].is_string()) {
        // On-disk models are re-imported through SceneImporter, which spawns
        // its own entity tree — so a serialized reference cannot restore one
        // from here without duplicating that tree. Known gap; it at least
        // reports the actual path instead of silently substituting a cube.
        const std::string& uri = meshJson["ext"].get_ref<const std::string&>();
        LOG_WARN("ComponentBlocks: '", uri, "' is an imported model; re-import "
                 "it via File -> Import Model. Entity will have no mesh.");
        return nullptr;
    }
    return nullptr;
}

void WriteComponentBlocks(json& out, Entity entity, Coordinator& coord) {
    if (coord.HasComponent<TransformComponent>(entity)) {
        const auto& t = coord.GetComponent<TransformComponent>(entity);
        out["transform"] = {
            {"pos",   vec3_to_json(t.position)},
            {"rot",   vec3_to_json(t.rotation)},
            {"scale", vec3_to_json(t.scale)},
        };
    }

    if (coord.HasComponent<CameraComponent>(entity)) {
        const auto& c = coord.GetComponent<CameraComponent>(entity);
        json jc = json::object();
        if (const auto* p = props_for("CameraComponent")) Reflect::WriteFields(jc, &c, *p);
        out["camera"] = jc;
    }

    if (coord.HasComponent<RenderComponent>(entity)) {
        const auto& r = coord.GetComponent<RenderComponent>(entity);
        json jr = {{"mesh", MeshRefFor(r)}};
        if (const auto* p = props_for("RenderComponent")) Reflect::WriteFields(jr, &r, *p);
        out["render"] = jr;
    }

    if (coord.HasComponent<PhysicsComponent>(entity)) {
        const auto& ph = coord.GetComponent<PhysicsComponent>(entity);
        json jp = json::object();
        if (const auto* p = props_for("PhysicsComponent")) Reflect::WriteFields(jp, &ph, *p);
        out["physics"] = jp;
    }

    if (coord.HasComponent<LightComponent>(entity)) {
        const auto& lc = coord.GetComponent<LightComponent>(entity);
        // `type` is written explicitly as well as reflected: the reflected
        // Enum path stores the same ordinal, but the explicit key is what the
        // documented format shows and what a hand-edited file is likely to set.
        json jl = {{"type", static_cast<int>(lc.type)}};
        if (const auto* p = props_for("LightComponent")) Reflect::WriteFields(jl, &lc, *p);
        out["light"] = jl;
    }

    if (coord.HasComponent<AnimationComponent>(entity)) {
        const auto& ac = coord.GetComponent<AnimationComponent>(entity);
        out["animation"] = {
            {"currentClip",   ac.currentAnimName},
            {"playbackSpeed", ac.playbackSpeed},
            {"playing",       ac.playing},
            {"loop",          ac.loop},
        };
    }
}

void ReadComponentBlocks(const json& in, Entity entity, Coordinator& coord) {
    if (!in.is_object()) return;

    if (in.contains("transform") && in["transform"].is_object()) {
        TransformComponent t;
        const auto& jt = in["transform"];
        if (jt.contains("pos"))   vec3_from_json(jt["pos"],   t.position);
        if (jt.contains("rot"))   vec3_from_json(jt["rot"],   t.rotation);
        if (jt.contains("scale")) vec3_from_json(jt["scale"], t.scale);
        coord.AddComponent(entity, t);
    }

    if (in.contains("camera") && in["camera"].is_object()) {
        CameraComponent c;
        if (const auto* p = props_for("CameraComponent")) Reflect::ReadFields(in["camera"], &c, *p);
        coord.AddComponent(entity, c);
    }

    if (in.contains("render") && in["render"].is_object()) {
        RenderComponent r;
        if (const auto* p = props_for("RenderComponent")) Reflect::ReadFields(in["render"], &r, *p);
        if (in["render"].contains("mesh")) r.renderable = ResolveMeshRef(in["render"]["mesh"]);
        coord.AddComponent(entity, r);
    }

    if (in.contains("physics") && in["physics"].is_object()) {
        PhysicsComponent ph;
        if (const auto* p = props_for("PhysicsComponent")) Reflect::ReadFields(in["physics"], &ph, *p);
        // The rigid body pointer stays null: ECSPhysicsSystem rebuilds it next
        // tick from the loaded shape and mass. A deserialized Bullet pointer
        // would be a dangling read.
        ph.rigidBody = nullptr;
        ph.shapeHash = 0;
        coord.AddComponent(entity, ph);
    }

    if (in.contains("light") && in["light"].is_object()) {
        LightComponent lc;
        if (in["light"].contains("type") && in["light"]["type"].is_number_integer()) {
            lc.type = static_cast<MistLightType>(in["light"]["type"].get<int>());
        }
        if (const auto* p = props_for("LightComponent")) Reflect::ReadFields(in["light"], &lc, *p);
        coord.AddComponent(entity, lc);
    }

    if (in.contains("animation") && in["animation"].is_object()) {
        AnimationComponent ac;
        const auto& ja = in["animation"];
        ac.currentAnimName = ja.value("currentClip",   std::string{});
        ac.playbackSpeed   = ja.value("playbackSpeed", 1.0f);
        ac.playing         = ja.value("playing",       false);
        ac.loop            = ja.value("loop",          true);
        // availableClips is not serialized: clips come from the imported
        // model, not from the scene file.
        coord.AddComponent(entity, ac);
    }
}

} // namespace Mist::Scene
