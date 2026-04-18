#pragma once
#ifndef MIST_SCRIPT_REGISTRY_H
#define MIST_SCRIPT_REGISTRY_H

#include "Script/ScriptLanguage.h"

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Mist::Script {

// Process-wide registry of script-language backends, keyed by file
// extension. Multiple languages can coexist (e.g., Lua + a custom DSL)
// — each calls `Register` once during static init / early main.
class ScriptRegistry {
public:
    static ScriptRegistry& Instance();

    // Register a backend. Extension comparison is case-insensitive on
    // access; we keep the canonical lowercase form internally.
    void Register(std::shared_ptr<IScriptLanguage> language);

    // Look up a backend by extension (must include the leading dot,
    // e.g. ".lua"). Returns nullptr if no backend is registered for it.
    std::shared_ptr<IScriptLanguage> Get(std::string_view extension) const;

    // Shut down every registered backend. Called from engine teardown.
    void ShutdownAll();

private:
    ScriptRegistry() = default;

    mutable std::mutex                                                  m_Mutex;
    std::unordered_map<std::string, std::shared_ptr<IScriptLanguage>>   m_Languages;
};

} // namespace Mist::Script

#endif // MIST_SCRIPT_REGISTRY_H
