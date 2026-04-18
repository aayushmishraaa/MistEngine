#pragma once
#ifndef MIST_SCENE_SERIALIZER_H
#define MIST_SCENE_SERIALIZER_H

#include <cstdint>
#include <string>

class Coordinator;

using Entity = uint32_t;

// Scene serializer v2 — stores entities as JSON with ExtResource references
// to shared assets. Supersedes the hand-rolled JsonValue/JsonWriter pair
// that previously lived here; those inlined every mesh into the scene file
// and couldn't express asset reuse.
//
// File shape (v0.5):
// {
//   "version": "0.5",
//   "entities": [
//     {
//       "name": "Ground",
//       "transform": {"pos":[0,-0.01,0], "rot":[0,0,0], "scale":[20,1,20]},
//       "render":    {"mesh": {"builtin": "plane"}, "visible": true},
//       "physics":   {"hasRigidBody": false, "syncTransform": true}
//     }
//   ]
// }
//
// `mesh` is either `{"builtin": "cube|plane|sphere"}` for built-in primitives
// or `{"ext": "res://meshes/foo.mesh"}` for on-disk assets. Load path routes
// both through AssetRegistry::meshes() so two entities referencing the same
// path share one underlying Mesh.
class SceneSerializer {
public:
    static bool Save(const std::string& filepath, Coordinator& coordinator, int entityCount);
    static bool Load(const std::string& filepath, Coordinator& coordinator, int& entityCount);
};

#endif // MIST_SCENE_SERIALIZER_H
