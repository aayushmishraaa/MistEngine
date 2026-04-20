#if MIST_ENABLE_SCRIPTING

#include <catch2/catch_all.hpp>

#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Coordinator.h"
#include "Script/LuaScriptLanguage.h"
#include "Script/ScriptRegistry.h"

#include <memory>
#include <string>

// Physics binding existence checks. The functions are registered at
// LuaScriptLanguage::Init time; these tests run headless (no Bullet
// world stepping, no GL) and only assert the bindings are callable
// from Lua and behave like no-ops on entities without PhysicsComponent.
// The "actual physics happens" coverage lives in the runtime smoke
// checklist because a real PhysicsSystem + ECSPhysicsSystem tick
// requires a live world.

extern Coordinator gCoordinator;

namespace {
std::shared_ptr<Mist::Script::LuaScriptLanguage> makeLua() {
    auto l = std::make_shared<Mist::Script::LuaScriptLanguage>();
    Mist::Script::ScriptRegistry::Instance().Register(l);
    return l;
}

// The Lua bindings hit `gCoordinator.HasComponent<PhysicsComponent>(e)`,
// which dereferences the per-type ComponentArray. If PhysicsComponent
// was never registered, that pointer is null and we segfault. Engine
// startup registers it; the test harness doesn't, so we do it here.
// Idempotent: ComponentManager::RegisterComponent uses insert(), so
// a second call is a no-op key collision.
void ensurePhysicsRegistered() {
    static bool initialized = false;
    if (!initialized) {
        gCoordinator.Init();
        gCoordinator.RegisterComponent<PhysicsComponent>();
        initialized = true;
    }
}
} // namespace

TEST_CASE("Physics Lua bindings are all registered", "[lua][physics][bindings]") {
    auto lua = makeLua();
    auto inst = lua->Compile(R"(
        ok = (type(apply_force)   == 'function')
            and (type(apply_impulse) == 'function')
            and (type(set_velocity)  == 'function')
            and (type(set_mass)      == 'function')
            and (type(set_kinematic) == 'function')
        ok_str = tostring(ok)
    )");
    REQUIRE(inst != nullptr);

    std::string out;
    REQUIRE(inst->GetString("ok_str", out));
    REQUIRE(out == "true");
}

TEST_CASE("Physics bindings silently no-op on entities without PhysicsComponent",
          "[lua][physics][bindings]") {
    ensurePhysicsRegistered();

    // id 999999 is effectively guaranteed not to have PhysicsComponent
    // in the test process. The bindings should swallow this without
    // throwing / erroring — that's the "tolerant-of-stale-ids"
    // contract callers rely on.
    auto lua = makeLua();
    auto inst = lua->Compile(R"(
        reached_end = false
        apply_force(999999, 0, 0, 0)
        apply_impulse(999999, 0, 0, 0)
        set_velocity(999999, 0, 0, 0)
        set_mass(999999, 1)
        set_kinematic(999999, true)
        reached_end = true
        reached_str = tostring(reached_end)
    )");
    REQUIRE(inst != nullptr);

    std::string out;
    REQUIRE(inst->GetString("reached_str", out));
    REQUIRE(out == "true");
}

#endif // MIST_ENABLE_SCRIPTING
