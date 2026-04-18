#include <catch2/catch_all.hpp>

#include "Import/AssetImporter.h"
#include "Import/PassThroughImporter.h"

#include <filesystem>
#include <fstream>
#include <memory>

namespace fs = std::filesystem;

namespace {
fs::path uniqueTempDir(const char* label) {
    auto base = fs::temp_directory_path() / (std::string("mist-importer-") + label);
    fs::remove_all(base);
    fs::create_directories(base);
    return base;
}
} // namespace

TEST_CASE("ImporterRegistry resolves by extension", "[importer]") {
    Mist::Import::ImporterRegistry::Instance().Clear();

    auto imp = std::make_shared<Mist::Import::PassThroughImporter>(
        std::vector<std::string>{".png", ".jpg"});
    Mist::Import::ImporterRegistry::Instance().Register(imp);

    REQUIRE(Mist::Import::ImporterRegistry::Instance().Get(".png") != nullptr);
    REQUIRE(Mist::Import::ImporterRegistry::Instance().Get(".jpg") != nullptr);
    // Case-insensitive.
    REQUIRE(Mist::Import::ImporterRegistry::Instance().Get(".PNG") != nullptr);
    REQUIRE(Mist::Import::ImporterRegistry::Instance().Get(".other") == nullptr);
}

TEST_CASE("PassThroughImporter copies source into output directory", "[importer]") {
    auto srcDir = uniqueTempDir("src");
    auto outDir = uniqueTempDir("out");

    auto src = srcDir / "hello.png";
    std::ofstream(src) << "pretend this is a png";

    Mist::Import::PassThroughImporter imp({".png"});
    Mist::Import::ImportSettings settings;
    auto result = imp.Import(src, outDir, settings);

    REQUIRE(!result.empty());
    REQUIRE(fs::exists(result));
    REQUIRE(fs::file_size(result) == fs::file_size(src));
}

TEST_CASE("ImportSettings round-trips values", "[importer]") {
    Mist::Import::ImportSettings s;
    REQUIRE(s.Empty());
    s.Set("compress", "bc7");
    s.Set("mipmaps",  "true");

    REQUIRE(s.GetOr("compress")          == "bc7");
    REQUIRE(s.GetOr("mipmaps")           == "true");
    REQUIRE(s.GetOr("missing", "fallback") == "fallback");
}
