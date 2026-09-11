#pragma once
#ifndef MIST_PREFAB_SERIALIZER_H
#define MIST_PREFAB_SERIALIZER_H

// `.mistprefab` — the engine's first reuse unit.
//
// Per docs/okf/decisions/0001-prefab-asset-model.md, a prefab is a SEPARATE
// asset type holding an entity subtree, not Godot's "every scene is also
// instantiable" model. That duality was rejected because it needs stable
// per-node identity, instance-level override tracking, and propagation rules
// that survive the source gaining or losing nodes — and MistEngine had no
// stable entity identity at all until NameComponent landed.
//
// The format difference from a scene is addressing, and only addressing:
//
//   - A scene stores integer ids and remaps them through an old->new table on
//     load, because CreateEntity does not hand back the ids that were saved.
//   - A prefab stores **sibling-unique name paths** ("Turret/Barrel"). No ids
//     appear anywhere, so a prefab is position-independent and ids can never
//     leak between assets.
//
// The component payload is identical and comes from Scene/ComponentBlocks.h,
// shared with the scene serializer, so adding a component updates one writer
// rather than two.
//
// Out of scope for this first cut, deliberately:
//   - Nested prefabs (a prefab containing an instance of another).
//   - Structural overrides — adding or removing a child on an instance. This
//     is where prefab systems genuinely rot; overrides are scalar and vector
//     FIELDS only.

#include "ECS/Coordinator.h"

#include <string>

namespace Mist::Assets {

struct PrefabSerializer {
    // Serializes the subtree rooted at `root` to a `.mistprefab` file.
    //
    // Fails, rather than writing a broken asset, when two siblings share a
    // name: a duplicate makes every override addressed at that path ambiguous,
    // and the failure would otherwise surface much later as an override
    // landing on the wrong child. Godot enforces the same rule.
    static bool Save(Entity root, Coordinator& coord, const std::string& path);

    // Instantiates the prefab additively and returns the new subtree's root,
    // or Entity(-1) on failure.
    //
    // Note this is additive, unlike SceneSerializer::Load which destroys the
    // world first. Instancing into a live scene is the entire point.
    static Entity Instantiate(const std::string& path, Coordinator& coord);

    // String forms, for tests and for callers that already hold the text.
    // Neither is path-guarded, because neither touches a path.
    static std::string SaveToString(Entity root, Coordinator& coord, bool* ok = nullptr);
    static Entity InstantiateFromString(const std::string& text, Coordinator& coord,
                                        const std::string& sourcePath = {});
};

} // namespace Mist::Assets

#endif // MIST_PREFAB_SERIALIZER_H
