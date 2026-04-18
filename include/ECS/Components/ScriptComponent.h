#ifndef SCRIPTCOMPONENT_H
#define SCRIPTCOMPONENT_H

#include "Core/Reflection.h"
#include "Script/ScriptLanguage.h"

#include <memory>
#include <string>

// Attach a script to an entity via its source path. The concrete script
// instance is owned here via shared_ptr so two components pointing at
// the same ScriptInstance share lifetime — AssetRegistry is the
// place-of-truth cache but the component keeps a strong ref so scene
// teardown order doesn't race the registry shutdown.
struct ScriptComponent {
    std::string                                 path;      // "res://scripts/spinner.lua"
    std::shared_ptr<Mist::Script::IScriptInstance> instance;
    bool                                        readyFired = false;
};

// Only `path` is reflected — the compiled instance is engine-internal.
MIST_REFLECT(ScriptComponent)
    MIST_FIELD(ScriptComponent, path, ::Mist::PropertyHint::None, "")
MIST_REFLECT_END(ScriptComponent)

#endif // SCRIPTCOMPONENT_H
