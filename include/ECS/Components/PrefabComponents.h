#ifndef PREFABCOMPONENTS_H
#define PREFABCOMPONENTS_H

#include "Core/Reflection.h"
#include "ECS/Entity.h"

#include <string>

// Marks an entity as the root of a `.mistprefab` instance.
//
// A scene stores the **reference plus overrides**, never the expanded subtree.
// That is the entire payoff of the design: edit the prefab source, reload the
// scene, and every instance picks up the change for free. Flattening the
// subtree into the scene file would make a prefab a one-time copy — which is
// what "Duplicate" in the hierarchy already was.
struct PrefabInstanceComponent {
    // Path to the source asset, resolved under the project root by PathGuard.
    std::string prefabPath;

    // Per-instance overrides, as a JSON array:
    //
    //   [{"target": "Barrel", "component": "RenderComponent",
    //     "fields": {"materialPath": "materials/rust.mistmat"}}]
    //
    // `target` is a prefab-local name path relative to the instance root ("" is
    // the root itself). `fields` is a PARTIAL reflected object — exactly the
    // shape Mist::Reflect::ReadFields already consumes, where a missing key
    // leaves the field alone. That is not a coincidence: "apply only the fields
    // that were overridden" and "tolerate fields an older file lacks" are the
    // same operation, so overrides reuse it rather than reimplementing it.
    //
    // Held as text rather than a parsed structure so this header stays free of
    // nlohmann/json, and deliberately NOT reflected: a raw JSON blob in the
    // Inspector would invite hand-editing the one field whose syntax errors
    // silently drop overrides. The Inspector marks overridden fields instead.
    std::string overridesJson;
};

MIST_REFLECT(PrefabInstanceComponent)
    MIST_FIELD(PrefabInstanceComponent, prefabPath, ::Mist::PropertyHint::ResourceRef, "Prefab")
MIST_REFLECT_END(PrefabInstanceComponent)

// Runtime-only membership marker, written on every entity a prefab instance
// spawned — including the root.
//
// Two things need it. SceneSerializer::Save uses it to SKIP expanded prefab
// content, writing the instance reference instead. The Inspector uses
// `localPath` to name the override target when a field is edited on a member.
//
// Never serialized: it is rebuilt at instantiate time, and a saved entity id
// (`instanceRoot`) would be meaningless after the load-time id remap anyway.
struct PrefabMemberComponent {
    Entity      instanceRoot = 0;
    std::string localPath;   // "" for the instance root itself
};

#endif // PREFABCOMPONENTS_H
