#include "Resources/AssetRegistry.h"

#include "Mesh.h"
#include "ShapeGenerator.h"
#include "Texture.h"

#include <memory>
#include <string>

namespace Mist::Assets {

AssetRegistry& AssetRegistry::Instance() {
    static AssetRegistry inst;
    return inst;
}

namespace {
// Built-in mesh loader. Recognises three "procedural" schemes:
//   builtin://cube
//   builtin://plane
//   builtin://sphere
// Paths starting with res:// are reserved for future on-disk mesh files
// (a minimal `.mesh` format lands with the scene serializer in G3).
std::shared_ptr<Mesh> defaultMeshLoader(const std::string& path) {
    std::vector<Vertex>       verts;
    std::vector<unsigned int> idx;

    if (path == "builtin://cube") {
        generateCubeMesh(verts, idx);
    } else if (path == "builtin://plane") {
        generatePlaneMesh(verts, idx);
    } else if (path == "builtin://sphere") {
        generateSphereMesh(verts, idx);
    } else {
        // TODO(G3): parse .mesh files once we have an on-disk mesh format.
        return nullptr;
    }

    return std::make_shared<Mesh>(verts, idx, std::vector<Texture>{});
}
} // namespace

void AssetRegistry::RegisterDefaultLoaders() {
    if (m_LoadersRegistered) {
        return;
    }
    m_LoadersRegistered = true;

    m_Meshes.SetLoader(&defaultMeshLoader);
    // Texture, AudioClip, Shader loaders wired in later phases — the
    // ResourceManagers exist immediately so callers can probe the cache
    // without crashing, they just return invalid handles until a loader
    // is installed.
}

} // namespace Mist::Assets
