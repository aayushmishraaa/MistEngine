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
// File shape (v1.0):
// {
//   "version": "1.0",
//   "entities": [
//     {
//       "name": "Ground",            // NameComponent; omitted when unnamed
//       "transform": {"pos":[0,-0.01,0], "rot":[0,0,0], "scale":[20,1,20]},
//       "render":    {"mesh": {"builtin": "plane"}, "visible": true},
//       "physics":   {"hasRigidBody": false, "syncTransform": true},
//       "light":     {"type": 0, "color": [1,1,1]},
//       "hierarchy": {"parent": 3},
//       "animation": {"currentClip": "Idle", "playing": true}
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

    // In-memory variants, sharing the exact same walk as the file paths.
    //
    // Play mode needs to snapshot the scene on Play and restore it on Stop;
    // routing that through a temp file would make an editor feature depend on
    // the scene sandbox and on disk being writable. These also make the
    // serializer testable headlessly, which is why tests/test_scene_serializer
    // could only exercise nlohmann's behaviour before.
    //
    // Neither is path-guarded, because neither touches a path.
    static std::string SaveToString(Coordinator& coordinator);
    static bool LoadFromString(const std::string& text, Coordinator& coordinator,
                               int& entityCount);
};

#endif // MIST_SCENE_SERIALIZER_H
