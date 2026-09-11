#pragma once
#ifndef MIST_PREFAB_OVERRIDES_H
#define MIST_PREFAB_OVERRIDES_H

// Per-instance property overrides for a `.mistprefab` instance.
//
// An override is keyed on (local name path, component type, field name). The
// first comes from the prefab's addressing scheme; the last two the reflection
// layer already gives by name, so nothing new had to be invented to identify a
// property.
//
// Scope, deliberately narrow: scalar and vector FIELDS only. Structural
// overrides — adding or removing a child on an instance — are where prefab
// systems genuinely rot, because they force propagation rules that survive the
// source gaining or losing nodes. Explicitly out of scope rather than
// half-implemented.
//
// The storage shape is a partial reflected object per (target, component):
//
//   [{"target": "Barrel", "component": "RenderComponent",
//     "fields": {"materialPath": "materials/rust.mistmat"}}]
//
// That is exactly what Mist::Reflect::ReadFields consumes, where a missing key
// leaves the field untouched. "Apply only the overridden fields" and "tolerate
// fields an older file lacks" are the same operation, so this reuses it rather
// than reimplementing the same switch a fourth time.

#include "ECS/Coordinator.h"

#include <string>
#include <vector>

namespace Mist::Assets {

// One overridden field, flattened for the editor's benefit.
struct PrefabOverrideEntry {
    std::string target;     // local name path; "" is the instance root
    std::string component;  // reflected type name, e.g. "TransformComponent"
    std::string field;      // reflected property name
};

// Applies `overridesJson` to an already-spawned instance rooted at
// `instanceRoot`. Unresolvable targets are logged and skipped rather than
// aborting: a prefab that lost a child should drop that child's overrides, not
// refuse to load the scene.
//
// Returns the number of (target, component) groups successfully applied.
int ApplyPrefabOverrides(Entity instanceRoot, Coordinator& coord,
                         const std::string& overridesJson);

// Records one field override into `overridesJson`, merging into an existing
// entry for the same (target, component) rather than appending a duplicate.
// `valueSource` is the live component the current value is read from.
//
// Returns the updated JSON text.
std::string RecordPrefabOverride(const std::string& overridesJson,
                                 const std::string& target,
                                 const std::string& componentType,
                                 const std::string& fieldName,
                                 const void* componentInstance);

// Removes one field override ("revert to prefab"). Empty groups are pruned so
// the stored JSON does not accumulate husks.
std::string ClearPrefabOverride(const std::string& overridesJson,
                                const std::string& target,
                                const std::string& componentType,
                                const std::string& fieldName);

// Flattened list, for the Inspector's overridden-field markers.
std::vector<PrefabOverrideEntry> ListPrefabOverrides(const std::string& overridesJson);

// The local name path of `entity` within its prefab instance, or "" when it is
// the root. Returns false when the entity is not part of an instance.
bool PrefabLocalPath(Entity entity, Coordinator& coord, std::string& out);

} // namespace Mist::Assets

#endif // MIST_PREFAB_OVERRIDES_H
