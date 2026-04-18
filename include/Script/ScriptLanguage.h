#pragma once
#ifndef MIST_SCRIPT_LANGUAGE_H
#define MIST_SCRIPT_LANGUAGE_H

#include <memory>
#include <string>
#include <string_view>

namespace Mist::Script {

// A running script instance attached to an entity/asset. Concrete
// implementations (Lua, Wren, GDScript-like) subclass this.
class IScriptInstance {
public:
    virtual ~IScriptInstance() = default;

    // Call a named entry point on the script. Arguments are packed via
    // a backend-specific adapter in the concrete subclass; this base
    // interface stays varargs-free so future backends without variadic
    // marshalling (e.g. AOT-compiled scripts) can implement it.
    virtual void CallVoid(std::string_view method) = 0;

    // Optional getter/setter for exposed properties. Default returns
    // false for backends that don't support runtime introspection; the
    // editor's reflection system (G1) falls back to the C++ side of the
    // component in that case.
    virtual bool GetString(std::string_view /*name*/, std::string& /*out*/) { return false; }
    virtual bool SetString(std::string_view /*name*/, std::string_view /*val*/) { return false; }
};

// Language backend. Registered into ScriptRegistry (see below) keyed on
// file extension; ScriptComponent::Load routes a path through the
// matching backend.
class IScriptLanguage {
public:
    virtual ~IScriptLanguage() = default;

    virtual std::string_view GetName()      const = 0;
    virtual std::string_view GetExtension() const = 0;  // ".lua", ".gds", …

    // Compile + instantiate. Returns nullptr on parse/compile error; the
    // concrete backend is responsible for surfacing diagnostics via the
    // engine logger.
    virtual std::unique_ptr<IScriptInstance> Compile(std::string_view source) = 0;

    // Process-wide init/shutdown — useful for VMs that hold a global
    // interpreter state (Lua's lua_State, Wren's WrenVM, etc.). Called
    // by ScriptRegistry on registration and at engine shutdown.
    virtual void Init()     {}
    virtual void Shutdown() {}
};

} // namespace Mist::Script

#endif // MIST_SCRIPT_LANGUAGE_H
