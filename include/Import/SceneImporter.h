#pragma once
#ifndef MIST_SCENE_IMPORTER_H
#define MIST_SCENE_IMPORTER_H

#include "ECS/Entity.h"

#include <string>
#include <string_view>
#include <unordered_map>

class Coordinator;

namespace Mist::Import {

// Imports an on-disk model file (.obj / .fbx / .gltf / .glb) into the
// scene as a tree of entities. Each aiNode becomes an entity with
// TransformComponent + HierarchyComponent; meshes attached to that
// node get a RenderComponent. Textures on each aiMaterial are loaded
// and attached via an inline PBRMaterial. The Mesh objects are kept
// alive by a process-wide store so RenderComponent's non-owning
// pointer remains valid for the lifetime of the process.
//
// Returns the root Entity (the aiScene's root node), or
// `static_cast<Entity>(-1)` on failure (bad path, Assimp parse error,
// unsupported format).
//
// This is deliberately the SINGLE entry point for "turn a model file
// on disk into ECS state." Callers (UIManager drag-drop, File -> Import
// menu, tests) should go through here rather than talking to Assimp
// directly.
struct SceneImporter {
    // Returns true if the extension is one we know how to handle.
    static bool IsSupportedPath(std::string_view path);

    // Imports the model at `path` into `coord`. If `outNames` is
    // non-null, populates it with <entity, display name> pairs drawn
    // from the aiNode / aiMesh names so the Hierarchy panel shows
    // meaningful labels instead of "Entity N".
    static Entity ImportToScene(const std::string& path,
                                Coordinator& coord,
                                std::unordered_map<Entity, std::string>* outNames = nullptr);
};

} // namespace Mist::Import

#endif // MIST_SCENE_IMPORTER_H
