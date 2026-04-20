#pragma once
#ifndef MIST_MATERIAL_CACHE_H
#define MIST_MATERIAL_CACHE_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

struct PBRMaterial;

namespace Mist::Assets {

// Path -> PBRMaterial singleton cache. Populated on-demand by
// RenderSystem when a RenderComponent has a non-empty materialPath.
// Caches loaded PBRMaterial so multiple entities referencing the same
// .mistmat don't re-parse the file every frame.
class MaterialCache {
public:
    static MaterialCache& Instance();

    // Resolve a .mistmat path to a material. Returns nullptr on load
    // failure. Subsequent calls with the same path return the cached
    // shared_ptr.
    std::shared_ptr<PBRMaterial> Get(const std::string& path);

    // Re-read the file from disk (e.g. after the inspector saved it
    // over its source). Returns the new material or nullptr on failure.
    std::shared_ptr<PBRMaterial> Reload(const std::string& path);

    // Drop all cached entries. Used by scene-close / package-unload.
    void Clear();

private:
    MaterialCache() = default;
    std::mutex m_Mu;
    std::unordered_map<std::string, std::shared_ptr<PBRMaterial>> m_ByPath;
};

} // namespace Mist::Assets

#endif // MIST_MATERIAL_CACHE_H
