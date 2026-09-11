#pragma once
#ifndef MIST_INPUT_MAP_SERIALIZER_H
#define MIST_INPUT_MAP_SERIALIZER_H

// Persistence for an InputContextMap — Godot's Project Settings > Input Map.
//
// `InputSystem::RebindAction` has existed since the system was written, but the
// binding table was hardcoded in `CreateEditorContext()` / `CreateGameplayContext()`
// and nothing wrote it anywhere. So a rebind survived until the process exited
// and not one second longer, which is the same as not having rebinding.
//
// Guarded under the project root like every other asset path.

#include "Input/InputContext.h"

#include <string>

namespace Mist::Input {

// Writes every action and binding in `ctx` as JSON. Returns false on a path
// guard rejection or an unwritable file.
bool SaveInputMap(const InputContextMap& ctx, const std::string& path);

// Merges the file at `path` into `ctx`: actions present in the file replace
// their in-memory bindings, actions absent from it keep the built-in defaults.
//
// A merge rather than a replace on purpose — if a new engine version adds an
// action, a user's saved map from the previous version must not leave that
// action unbound.
bool LoadInputMap(InputContextMap& ctx, const std::string& path);

} // namespace Mist::Input

#endif // MIST_INPUT_MAP_SERIALIZER_H
