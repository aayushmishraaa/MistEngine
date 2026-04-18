#pragma once
#ifndef MIST_ASSET_REGISTRY_H
#define MIST_ASSET_REGISTRY_H

#include "Resources/ResourceManager.h"

#include <memory>
#include <string>

class Mesh;
class Texture;
struct AudioClip;
class Shader;

namespace Mist::Assets {

// Engine-wide singleton that owns one ResourceManager<T> per asset kind we
// actually load by path. Everything is path-keyed and deduplicated —
// two calls to meshes().Load("res://cube.mesh") return the same handle,
// and the resource only gets constructed once.
//
// The existing ResourceManager<T> template (include/Resources/ResourceManager.h)
// already has the caching + thread-safe load logic; this class just binds
// it to engine-side loaders and exposes named accessors.
class AssetRegistry {
public:
    static AssetRegistry& Instance();

    // Registers built-in loaders for each managed asset type. Safe to call
    // multiple times — subsequent calls are no-ops.
    void RegisterDefaultLoaders();

    ResourceManager<Mesh>&      meshes()   { return m_Meshes; }
    ResourceManager<Texture>&   textures() { return m_Textures; }
    ResourceManager<AudioClip>& audio()    { return m_Audio; }
    ResourceManager<Shader>&    shaders()  { return m_Shaders; }

private:
    AssetRegistry() = default;
    AssetRegistry(const AssetRegistry&)            = delete;
    AssetRegistry& operator=(const AssetRegistry&) = delete;

    bool m_LoadersRegistered = false;

    ResourceManager<Mesh>      m_Meshes;
    ResourceManager<Texture>   m_Textures;
    ResourceManager<AudioClip> m_Audio;
    ResourceManager<Shader>    m_Shaders;
};

} // namespace Mist::Assets

#endif // MIST_ASSET_REGISTRY_H
