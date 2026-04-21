#if MIST_ENABLE_SCRIPTING

#include "Script/LuaScriptLanguage.h"

#include "Core/Logger.h"
#include "Core/PathGuard.h"
#include "Core/ServiceLocator.h"
#include "PhysicsSystem.h"
#include "ECS/Components/HierarchyComponent.h"
#include "ECS/Components/AnimationComponent.h"
#include "ECS/Components/LightComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/ScriptComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Coordinator.h"
#include "Mesh.h"
#include "Resources/AssetRegistry.h"
#include "Resources/Ref.h"
#include "Script/ScriptRegistry.h"

#include <sol/sol.hpp>

#include <fstream>
#include <sstream>

extern Coordinator gCoordinator;

namespace Mist::Script {

// PIMPL definitions — kept in the .cpp so sol2's heavy templates don't
// bleed into every translation unit that touches scripting.
struct LuaStatePimpl {
    sol::state state;
};

struct LuaEnvPimpl {
    sol::environment env;
};

namespace {
// Thread-local bridges — Lua callbacks are called synchronously on a
// single thread today, but using thread_local lets future multi-VM
// setups (one sol::state per thread) share this pattern without rewrite.
thread_local Entity g_current_entity = static_cast<Entity>(-1);
thread_local float  g_last_dt        = 0.0f;
} // namespace

Entity LuaScriptLanguage::CurrentEntity()            { return g_current_entity; }
void   LuaScriptLanguage::SetCurrentEntity(Entity e) { g_current_entity = e; }
float  LuaScriptLanguage::LastDeltaTime()            { return g_last_dt; }
void   LuaScriptLanguage::SetLastDeltaTime(float dt) { g_last_dt = dt; }

// --- LuaScriptLanguage ---------------------------------------------------

LuaScriptLanguage::LuaScriptLanguage()  = default;
LuaScriptLanguage::~LuaScriptLanguage() = default;

void LuaScriptLanguage::Init() {
    if (m_State) return;
    m_State = std::make_unique<LuaStatePimpl>();
    auto& state = m_State->state;

    state.open_libraries(sol::lib::base, sol::lib::string,
                         sol::lib::math, sol::lib::table);

    // print → engine logger so Lua output lands in the console dock.
    state["print"] = [](sol::variadic_args args) {
        std::string joined;
        for (auto v : args) {
            if (!joined.empty()) joined += '\t';
            joined += v.as<std::string>();
        }
        LOG_INFO("[lua] ", joined);
    };

    state["entity_id"] = []() -> int {
        return static_cast<int>(CurrentEntity());
    };

    state["get_delta_time"] = []() -> float { return LastDeltaTime(); };

    // Transform accessors — flat table form. A sol::usertype<glm::vec3>
    // can land later without breaking the binding surface.
    state["get_transform"] = [](sol::this_state ts) -> sol::object {
        sol::state_view lua(ts);
        Entity e = CurrentEntity();
        if (e == static_cast<Entity>(-1) ||
            !gCoordinator.HasComponent<TransformComponent>(e)) {
            return sol::make_object(lua, sol::lua_nil);
        }
        auto& t = gCoordinator.GetComponent<TransformComponent>(e);
        sol::table tbl = lua.create_table();
        tbl["x"]  = t.position.x; tbl["y"]  = t.position.y; tbl["z"]  = t.position.z;
        tbl["rx"] = t.rotation.x; tbl["ry"] = t.rotation.y; tbl["rz"] = t.rotation.z;
        tbl["sx"] = t.scale.x;    tbl["sy"] = t.scale.y;    tbl["sz"] = t.scale.z;
        return sol::make_object(lua, tbl);
    };

    state["set_transform"] = [](sol::table tbl) {
        Entity e = CurrentEntity();
        if (e == static_cast<Entity>(-1) ||
            !gCoordinator.HasComponent<TransformComponent>(e)) return;
        auto& t = gCoordinator.GetComponent<TransformComponent>(e);
        t.position.x = tbl.get_or("x",  t.position.x);
        t.position.y = tbl.get_or("y",  t.position.y);
        t.position.z = tbl.get_or("z",  t.position.z);
        t.rotation.x = tbl.get_or("rx", t.rotation.x);
        t.rotation.y = tbl.get_or("ry", t.rotation.y);
        t.rotation.z = tbl.get_or("rz", t.rotation.z);
        t.scale.x    = tbl.get_or("sx", t.scale.x);
        t.scale.y    = tbl.get_or("sy", t.scale.y);
        t.scale.z    = tbl.get_or("sz", t.scale.z);
        t.dirty = true;  // HierarchySystem rebuilds cachedGlobal next frame
    };

    // --- Gameplay bindings (this cycle) ---

    // spawn_cube(x, y, z) — returns new entity id or -1 on failure.
    // Uses the shared "builtin://cube" mesh from AssetRegistry so N cubes
    // allocate exactly one Mesh, not N. Optional sx/sy/sz lets scripts
    // make pillars / slabs without attaching a follow-up set_transform.
    state["spawn_cube"] = [](float x, float y, float z,
                             sol::optional<float> sx,
                             sol::optional<float> sy,
                             sol::optional<float> sz) -> int {
        auto& meshes = Mist::Assets::AssetRegistry::Instance().meshes();
        auto ref = LoadRef(meshes, std::string("builtin://cube"));
        if (!ref) {
            LOG_ERROR("spawn_cube: failed to load builtin://cube");
            return -1;
        }
        Entity e = gCoordinator.CreateEntity();
        TransformComponent t;
        t.position = {x, y, z};
        t.scale    = {sx.value_or(1.0f), sy.value_or(1.0f), sz.value_or(1.0f)};
        gCoordinator.AddComponent(e, t);

        RenderComponent r;
        r.renderable = ref.get();   // non-owning; registry keeps it alive
        r.visible    = true;
        gCoordinator.AddComponent(e, r);

        gCoordinator.AddComponent(e, HierarchyComponent{});
        return static_cast<int>(e);
    };

    // spawn_plane(x, y, z, sx, sy, sz) — convenience for the ground plate.
    // Scale defaults are a big flat 20×0.2×20 slab matching the old
    // hardcoded default scene.
    state["spawn_plane"] = [](float x, float y, float z,
                              sol::optional<float> sx,
                              sol::optional<float> sy,
                              sol::optional<float> sz) -> int {
        auto& meshes = Mist::Assets::AssetRegistry::Instance().meshes();
        auto ref = LoadRef(meshes, std::string("builtin://plane"));
        if (!ref) {
            LOG_ERROR("spawn_plane: failed to load builtin://plane");
            return -1;
        }
        Entity e = gCoordinator.CreateEntity();
        TransformComponent t;
        t.position = {x, y, z};
        t.scale    = {sx.value_or(20.0f), sy.value_or(1.0f), sz.value_or(20.0f)};
        gCoordinator.AddComponent(e, t);

        RenderComponent r;
        r.renderable = ref.get();
        r.visible    = true;
        gCoordinator.AddComponent(e, r);

        gCoordinator.AddComponent(e, HierarchyComponent{});
        return static_cast<int>(e);
    };

    // destroy_entity(id) — removes the entity from every system. Safe
    // to call on an already-dead id; the coordinator logs but doesn't
    // throw.
    state["destroy_entity"] = [](int id) {
        if (id < 0) {
            LOG_WARN("destroy_entity: ignoring negative id");
            return;
        }
        gCoordinator.DestroyEntity(static_cast<Entity>(id));
    };

    // run_script(path) — compile + run a .lua file's top-level once,
    // without attaching to any entity. Handy for bootstrap scripts that
    // should spawn things into the world but don't want a lingering
    // ScriptComponent.
    state["run_script"] = [](const std::string& path) -> bool {
        auto abs = Mist::PathGuard::resolve_res_path(path);
        if (abs.empty()) {
            LOG_ERROR("run_script: path escapes sandbox: ", path);
            return false;
        }
        std::ifstream in(abs);
        if (!in.is_open()) {
            LOG_ERROR("run_script: cannot open ", path);
            return false;
        }
        std::stringstream ss; ss << in.rdbuf();
        auto lang = ScriptRegistry::Instance().Get(".lua");
        if (!lang) return false;
        auto inst = lang->Compile(ss.str());
        // Compile runs the top-level body; inst drops immediately.
        return inst != nullptr;
    };

    // attach_script(id, path) — compile a Lua file and bind it as a
    // ScriptComponent on the given entity. Path must be res://-relative
    // and stays within the project root sandbox.
    state["attach_script"] = [](int id, const std::string& path) -> bool {
        if (id < 0) {
            LOG_WARN("attach_script: negative entity id");
            return false;
        }
        auto abs = Mist::PathGuard::resolve_res_path(path);
        if (abs.empty()) {
            LOG_ERROR("attach_script: path escapes sandbox: ", path);
            return false;
        }
        std::ifstream in(abs);
        if (!in.is_open()) {
            LOG_ERROR("attach_script: cannot open ", path);
            return false;
        }
        std::stringstream ss; ss << in.rdbuf();

        auto lang = ScriptRegistry::Instance().Get(".lua");
        if (!lang) {
            LOG_ERROR("attach_script: no Lua backend registered");
            return false;
        }
        auto inst = lang->Compile(ss.str());
        if (!inst) return false;   // Compile already logged the error

        ScriptComponent sc;
        sc.path     = path;
        sc.instance = std::shared_ptr<IScriptInstance>(inst.release());
        gCoordinator.AddComponent(static_cast<Entity>(id), sc);
        return true;
    };

    // --- Lighting bindings (Phase D of Godot-parity lighting cycle) ---

    // spawn_light(type, x, y, z, r, g, b, energy) -> entity id
    //   type in {"directional","omni","spot"}. RGB + energy optional,
    //   defaults to white @ 1.0. Returns new entity id, -1 on bad type.
    state["spawn_light"] = [](const std::string& type,
                              float x, float y, float z,
                              sol::optional<float> r,
                              sol::optional<float> g,
                              sol::optional<float> b,
                              sol::optional<float> energy) -> int {
        MistLightType t;
        if      (type == "directional") t = MistLightType::Directional;
        else if (type == "omni")        t = MistLightType::Omni;
        else if (type == "spot")        t = MistLightType::Spot;
        else {
            LOG_ERROR("spawn_light: unknown type '", type,
                      "' (expected directional/omni/spot)");
            return -1;
        }

        Entity e = gCoordinator.CreateEntity();
        TransformComponent tr;
        tr.position = {x, y, z};
        gCoordinator.AddComponent(e, tr);
        gCoordinator.AddComponent(e, HierarchyComponent{});

        LightComponent lc;
        lc.type   = t;
        lc.color  = { r.value_or(1.0f), g.value_or(1.0f), b.value_or(1.0f) };
        lc.energy = energy.value_or(1.0f);
        // Sensible defaults per type — user tweaks via inspector or
        // the set_light_* bindings below.
        if (t == MistLightType::Directional) lc.range = 0.0f;         // unused
        else if (t == MistLightType::Omni)   lc.range = 10.0f;
        else                                 lc.range = 15.0f;        // spot
        gCoordinator.AddComponent(e, lc);
        return static_cast<int>(e);
    };

    state["set_light_color"] = [](int id, float r, float g, float f) {
        Entity e = static_cast<Entity>(id);
        if (!gCoordinator.HasComponent<LightComponent>(e)) return;
        auto& lc = gCoordinator.GetComponent<LightComponent>(e);
        lc.color = {r, g, f};
    };
    state["set_light_energy"] = [](int id, float energy) {
        Entity e = static_cast<Entity>(id);
        if (!gCoordinator.HasComponent<LightComponent>(e)) return;
        gCoordinator.GetComponent<LightComponent>(e).energy = energy;
    };
    state["set_light_range"] = [](int id, float range) {
        Entity e = static_cast<Entity>(id);
        if (!gCoordinator.HasComponent<LightComponent>(e)) return;
        gCoordinator.GetComponent<LightComponent>(e).range = range;
    };

    // --- Physics bindings (Phase D of physics-upgrade cycle) ---
    //
    // Every binding silently no-ops on entities without a
    // PhysicsComponent — keeps Lua scripts tolerant of stale ids.
    // Force / impulse / velocity apply immediately to the Bullet
    // body; mass / kinematic mutate the component and the next
    // EnsureBody tick picks up the change.

    state["apply_force"] = [](int id, float fx, float fy, float fz) {
        Entity e = static_cast<Entity>(id);
        if (!gCoordinator.HasComponent<PhysicsComponent>(e)) return;
        auto& pc = gCoordinator.GetComponent<PhysicsComponent>(e);
        if (!pc.rigidBody) return;
        pc.rigidBody->activate(true);
        pc.rigidBody->applyCentralForce(btVector3(fx, fy, fz));
    };

    state["apply_impulse"] = [](int id, float ix, float iy, float iz) {
        Entity e = static_cast<Entity>(id);
        if (!gCoordinator.HasComponent<PhysicsComponent>(e)) return;
        auto& pc = gCoordinator.GetComponent<PhysicsComponent>(e);
        if (!pc.rigidBody) return;
        pc.rigidBody->activate(true);
        pc.rigidBody->applyCentralImpulse(btVector3(ix, iy, iz));
    };

    state["set_velocity"] = [](int id, float vx, float vy, float vz) {
        Entity e = static_cast<Entity>(id);
        if (!gCoordinator.HasComponent<PhysicsComponent>(e)) return;
        auto& pc = gCoordinator.GetComponent<PhysicsComponent>(e);
        if (!pc.rigidBody) return;
        pc.rigidBody->activate(true);
        pc.rigidBody->setLinearVelocity(btVector3(vx, vy, vz));
    };

    state["set_mass"] = [](int id, float mass) {
        Entity e = static_cast<Entity>(id);
        if (!gCoordinator.HasComponent<PhysicsComponent>(e)) return;
        // Mass is a shape-hash input — next EnsureBody call rebuilds.
        gCoordinator.GetComponent<PhysicsComponent>(e).mass = mass;
    };

    state["set_kinematic"] = [](int id, bool on) {
        Entity e = static_cast<Entity>(id);
        if (!gCoordinator.HasComponent<PhysicsComponent>(e)) return;
        gCoordinator.GetComponent<PhysicsComponent>(e).kinematic = on;
    };

    // --- Animation bindings (Phase D of assets cycle) ---
    //
    // Silently no-op on entities without AnimationComponent — same
    // "tolerant-of-stale-ids" contract the physics / light bindings
    // use. Clip lookup is O(N) in availableClips, but N is tiny (one
    // per aiAnimation in the source file).

    state["play_animation"] = [](int id, const std::string& name) -> bool {
        Entity e = static_cast<Entity>(id);
        if (!gCoordinator.HasComponent<AnimationComponent>(e)) return false;
        return gCoordinator.GetComponent<AnimationComponent>(e).PlayByName(name);
    };

    state["stop_animation"] = [](int id) {
        Entity e = static_cast<Entity>(id);
        if (!gCoordinator.HasComponent<AnimationComponent>(e)) return;
        gCoordinator.GetComponent<AnimationComponent>(e).Stop();
    };

    state["set_anim_speed"] = [](int id, float v) {
        Entity e = static_cast<Entity>(id);
        if (!gCoordinator.HasComponent<AnimationComponent>(e)) return;
        gCoordinator.GetComponent<AnimationComponent>(e).playbackSpeed = v;
    };

    state["set_anim_loop"] = [](int id, bool loop) {
        Entity e = static_cast<Entity>(id);
        if (!gCoordinator.HasComponent<AnimationComponent>(e)) return;
        gCoordinator.GetComponent<AnimationComponent>(e).loop = loop;
    };

    // --- Physics raycast (Phase G.3 of assets cycle) ---
    //
    //   hit, id, px, py, pz, nx, ny, nz = raycast(ox,oy,oz, dx,dy,dz, maxDist)
    //
    // Returns 8 values; if `hit` is false the rest are zero. `id` is
    // the ECS entity of the body that was struck, tagged via
    // Bullet user-index by ECSPhysicsSystem. Direction is auto-
    // normalised; maxDist is in world units.
    state["raycast"] = [](float ox, float oy, float oz,
                          float dx, float dy, float dz,
                          float maxDist) {
        auto* phys = ServiceLocator::Instance().GetPhysicsSystem();
        if (!phys) {
            return std::make_tuple(false, -1,
                                   0.0f, 0.0f, 0.0f,
                                   0.0f, 0.0f, 0.0f);
        }
        auto h = phys->Raycast({ox, oy, oz}, {dx, dy, dz}, maxDist);
        return std::make_tuple(h.hit,
                               h.hit ? static_cast<int>(h.entity) : -1,
                               h.point.x,  h.point.y,  h.point.z,
                               h.normal.x, h.normal.y, h.normal.z);
    };

    LOG_INFO("LuaScriptLanguage initialized (Lua 5.4 + sol2)");
}

void LuaScriptLanguage::Shutdown() {
    m_State.reset();
}

std::unique_ptr<IScriptInstance> LuaScriptLanguage::Compile(std::string_view source) {
    auto inst = std::make_unique<LuaScriptInstance>(this, std::string(source));
    if (!inst->Compile()) return nullptr;
    return inst;
}

// --- LuaScriptInstance ---------------------------------------------------

LuaScriptInstance::LuaScriptInstance(LuaScriptLanguage* owner, std::string source)
    : m_Owner(owner), m_Source(std::move(source)) {}

LuaScriptInstance::~LuaScriptInstance() = default;

bool LuaScriptInstance::Compile() {
    if (!m_Owner || !m_Owner->StatePimpl()) return false;
    auto& lua = m_Owner->StatePimpl()->state;

    // Each instance gets its own sandboxed environment that falls back
    // to the globals for things we registered in Init (print, entity_id,
    // etc.). Scripts see engine APIs but can't trample each other.
    m_Env = std::make_unique<LuaEnvPimpl>();
    m_Env->env = sol::environment(lua, sol::create, lua.globals());

    sol::protected_function_result res = lua.safe_script(
        m_Source, m_Env->env, sol::script_pass_on_error);
    if (!res.valid()) {
        sol::error err = res;
        LOG_ERROR("Lua compile error: ", err.what());
        m_Env.reset();
        m_Compiled = false;
        return false;
    }
    m_Compiled = true;
    return true;
}

void LuaScriptInstance::CallVoid(std::string_view method) {
    if (!m_Compiled || !m_Env) return;

    sol::protected_function fn = m_Env->env[std::string(method)];
    if (!fn.valid()) return; // script didn't define this callback

    sol::protected_function_result res = fn();
    if (!res.valid()) {
        sol::error err = res;
        LOG_ERROR("Lua '", method, "' runtime error: ", err.what());
    }
}

bool LuaScriptInstance::GetString(std::string_view name, std::string& out) {
    if (!m_Compiled || !m_Env) return false;
    sol::object v = m_Env->env[std::string(name)];
    if (!v.valid() || v.get_type() != sol::type::string) return false;
    out = v.as<std::string>();
    return true;
}

bool LuaScriptInstance::SetString(std::string_view name, std::string_view val) {
    if (!m_Compiled || !m_Env) return false;
    m_Env->env[std::string(name)] = std::string(val);
    return true;
}

} // namespace Mist::Script

#endif // MIST_ENABLE_SCRIPTING
