#pragma once
#ifndef MIST_PASS_THROUGH_IMPORTER_H
#define MIST_PASS_THROUGH_IMPORTER_H

#include "Import/AssetImporter.h"

#include <vector>

namespace Mist::Import {

// Minimum-viable importer: copies `source` verbatim into the output
// directory. Claims `".*"` so every asset goes through it by default
// before a format-specific importer is registered. Real importers
// (texture compression, mesh optimization) subclass IAssetImporter and
// claim specific extensions, bumping this off the responsibility list.
class PassThroughImporter : public IAssetImporter {
public:
    explicit PassThroughImporter(std::vector<std::string> extensions);

    std::vector<std::string_view> GetExtensions() const override;
    std::string_view              GetName()       const override { return "PassThrough"; }

    std::filesystem::path Import(const std::filesystem::path& source,
                                 const std::filesystem::path& outputDir,
                                 const ImportSettings&         settings) override;

private:
    std::vector<std::string> m_Extensions;
};

} // namespace Mist::Import

#endif // MIST_PASS_THROUGH_IMPORTER_H
