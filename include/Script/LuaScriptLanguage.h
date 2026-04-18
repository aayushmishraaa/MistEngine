#pragma once
#ifndef MIST_LUA_SCRIPT_LANGUAGE_H
#define MIST_LUA_SCRIPT_LANGUAGE_H

#if MIST_ENABLE_SCRIPTING

#include "ECS/Entity.h"
#include "Script/ScriptLanguage.h"

#include <memory>
#include <string>
#include <string_view>

// sol2's types (`sol::state`, `sol::environment`) can't be forward-declared
// — they're using-aliases to templates. To keep the sol2 compile cost out
// of every includer we hide the state + environment behind PIMPLs whose
// definitions live in LuaScriptLanguage.cpp.
namespace Mist::Script {

struct LuaStatePimpl;   // wraps sol::state
struct LuaEnvPimpl;     // wraps sol::environment

class LuaScriptLanguage;

// Per-script sandbox. Shares the owning language's VM but holds its own
// sol::environment so `function foo() ... end` declared in one script
// can't be seen by another. Mirrors Godot's per-script scope model.
class LuaScriptInstance : public IScriptInstance {
public:
    LuaScriptInstance(LuaScriptLanguage* owner, std::string source);
    ~LuaScriptInstance() override;

    // Compile source into this instance's environment. Idempotent — a
    // second compile overwrites the previous environment (hot-reload).
    bool Compile();

    void CallVoid(std::string_view method) override;

    bool GetString(std::string_view name, std::string& out) override;
    bool SetString(std::string_view name, std::string_view val) override;

private:
    LuaScriptLanguage*            m_Owner;   // non-owning
    std::string                   m_Source;
    std::unique_ptr<LuaEnvPimpl>  m_Env;
    bool                          m_Compiled = false;
};

class LuaScriptLanguage : public IScriptLanguage {
public:
    LuaScriptLanguage();
    ~LuaScriptLanguage() override;

    std::string_view GetName()      const override { return "Lua"; }
    std::string_view GetExtension() const override { return ".lua"; }

    std::unique_ptr<IScriptInstance> Compile(std::string_view source) override;

    void Init()     override;
    void Shutdown() override;

    // PIMPL accessor used by LuaScriptInstance. Callers from outside the
    // Lua impl should treat it as opaque.
    LuaStatePimpl* StatePimpl() { return m_State.get(); }

    // Thread-local bridges used by the Lua bindings. Set before each
    // callback so `entity_id()` / `get_delta_time()` resolve correctly.
    static Entity CurrentEntity();
    static void   SetCurrentEntity(Entity);

    static float LastDeltaTime();
    static void  SetLastDeltaTime(float);

private:
    std::unique_ptr<LuaStatePimpl> m_State;
};

} // namespace Mist::Script

#endif // MIST_ENABLE_SCRIPTING
#endif // MIST_LUA_SCRIPT_LANGUAGE_H
