#if MIST_ENABLE_SCRIPTING

#include <catch2/catch_all.hpp>

#include "ECS/Components/ScriptComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/HierarchyComponent.h"
#include "ECS/Coordinator.h"
#include "ECS/Systems/HierarchySystem.h"
#include "ECS/Systems/ScriptSystem.h"
#include "Script/LuaScriptLanguage.h"
#include "Script/ScriptRegistry.h"

#include <memory>
#include <string>

extern Coordinator gCoordinator;

// Each test builds a LuaScriptLanguage fresh; singleton ScriptRegistry
// tolerates re-registration (last-writer wins) so tests stay isolated.
namespace {

std::shared_ptr<Mist::Script::LuaScriptLanguage> makeLua() {
    auto l = std::make_shared<Mist::Script::LuaScriptLanguage>();
    Mist::Script::ScriptRegistry::Instance().Register(l);
    return l;
}

} // namespace

TEST_CASE("LuaScriptLanguage compiles + runs a trivial script", "[lua][script]") {
    auto lua = makeLua();
    auto inst = lua->Compile("message = 'hi from lua'");
    REQUIRE(inst != nullptr);

    std::string out;
    REQUIRE(inst->GetString("message", out));
    REQUIRE(out == "hi from lua");
}

TEST_CASE("LuaScriptLanguage compile reports syntax errors without crashing", "[lua]") {
    auto lua = makeLua();
    auto inst = lua->Compile("function oops( -- unterminated");
    REQUIRE(inst == nullptr);
}

TEST_CASE("LuaScriptLanguage CallVoid on missing function is safe", "[lua]") {
    auto lua = makeLua();
    auto inst = lua->Compile("x = 1");
    REQUIRE(inst != nullptr);
    // Should not throw or log an error — just no-op when the function
    // doesn't exist.
    inst->CallVoid("_ready");
    SUCCEED();
}

TEST_CASE("LuaScriptLanguage entity_id binding returns thread-local entity", "[lua][binding]") {
    auto lua = makeLua();
    // Store the captured id as a string so GetString can read it.
    // Lua's tostring(42) yields "42" (no trailing ".0" because it's an
    // integer subtype).
    auto inst = lua->Compile(R"(
        captured = ""
        function _ready()
            captured = tostring(entity_id())
        end
    )");
    REQUIRE(inst != nullptr);

    Mist::Script::LuaScriptLanguage::SetCurrentEntity(static_cast<Entity>(42));
    inst->CallVoid("_ready");
    Mist::Script::LuaScriptLanguage::SetCurrentEntity(static_cast<Entity>(-1));

    std::string out;
    REQUIRE(inst->GetString("captured", out));
    REQUIRE(out == "42");
}

TEST_CASE("Two script instances have isolated environments", "[lua]") {
    auto lua = makeLua();
    auto a = lua->Compile("only_in_a = 'yes'");
    auto b = lua->Compile("only_in_b = 'yes'");
    REQUIRE(a);
    REQUIRE(b);

    std::string out;
    REQUIRE(a->GetString("only_in_a", out));
    REQUIRE_FALSE(a->GetString("only_in_b", out));
    REQUIRE(b->GetString("only_in_b", out));
    REQUIRE_FALSE(b->GetString("only_in_a", out));
}

// --- Gameplay bindings --------------------------------------------------
//
// spawn_cube/spawn_plane would hit GL during Mesh construction
// (glCreateBuffers etc.) which crashes without a live GL context.
// Those paths are covered by the runtime smoke test where launching the
// editor spawns a 5-cube orbit via bootstrap.lua — if spawn_cube were
// broken you'd see no cubes, with the `[lua] bootstrap: assembling...`
// line missing from stdout. destroy_entity and attach_script are also
// exercised along the same path.
//
// Headless test coverage below validates the *binding registration* and
// error surface (non-GL paths) so a regression in the binding setup
// still trips CI even without software GL.

TEST_CASE("spawn_cube is callable from Lua (binding registered)", "[lua][bindings]") {
    auto lua = makeLua();
    auto inst = lua->Compile(R"(
        ok = (type(spawn_cube) == 'function')
            and (type(spawn_plane) == 'function')
            and (type(destroy_entity) == 'function')
            and (type(attach_script) == 'function')
            and (type(run_script) == 'function')
        has_ok = tostring(ok)
    )");
    REQUIRE(inst != nullptr);

    std::string out;
    REQUIRE(inst->GetString("has_ok", out));
    REQUIRE(out == "true");
}

TEST_CASE("destroy_entity tolerates negative ids without crashing", "[lua][bindings]") {
    auto lua = makeLua();
    // Should log a warning but return cleanly. No GL required.
    auto inst = lua->Compile("destroy_entity(-1)");
    REQUIRE(inst != nullptr);
    SUCCEED();
}

#endif // MIST_ENABLE_SCRIPTING
