#pragma once
#ifndef MIST_ASSET_IMPORTER_H
#define MIST_ASSET_IMPORTER_H

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Mist::Import {

// Per-asset import settings — the .import sidecar equivalent. Backed by
// nlohmann::json internally so users can hand-edit; the engine-facing
// API is a typed accessor by key for convenience.
//
// First cut: just a string map. Real importers will extend this with
// typed getters once they exist.
class ImportSettings {
public:
    void        Set(std::string_view key, std::string value);
    std::string GetOr(std::string_view key, std::string_view fallback = "") const;

    bool Empty() const { return m_Values.empty(); }

private:
    std::unordered_map<std::string, std::string> m_Values;
};

// Importer contract. A concrete importer turns a source asset (e.g.,
// `.png`) into an engine-ready cached file (e.g., `.ctex`) plus the
// sidecar `.import` metadata — same pattern as Godot's editor/import/
// subsystem. `PassThroughImporter` below is the minimum viable impl;
// real format-specific importers (texture compression, mesh
// optimisation, audio transcoding) arrive opportunistically.
class IAssetImporter {
public:
    virtual ~IAssetImporter() = default;

    virtual std::vector<std::string_view> GetExtensions() const = 0;
    virtual std::string_view              GetName()       const = 0;

    // Import `source` into `outputDir` with the given settings. Returns
    // the path of the produced cached asset, or an empty path on
    // failure. Must be deterministic — given the same source + settings,
    // the same output path must result (content-addressing via hash is
    // acceptable).
    virtual std::filesystem::path Import(const std::filesystem::path& source,
                                         const std::filesystem::path& outputDir,
                                         const ImportSettings&         settings) = 0;
};

// Process-wide importer registry. Each importer claims one or more
// extensions; the first registered wins on conflict (caller can call
// Clear() + Re-register to override).
class ImporterRegistry {
public:
    static ImporterRegistry& Instance();

    void                             Register(std::shared_ptr<IAssetImporter> importer);
    std::shared_ptr<IAssetImporter>  Get(std::string_view extension) const;
    void                             Clear();

private:
    ImporterRegistry() = default;
    mutable std::mutex                                                 m_Mutex;
    std::unordered_map<std::string, std::shared_ptr<IAssetImporter>>   m_ByExtension;
};

} // namespace Mist::Import

#endif // MIST_ASSET_IMPORTER_H
