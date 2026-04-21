#include <catch2/catch_all.hpp>

#include "Assets/PackageIO.h"

#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// .mistpkg single-file archive round-trip. The format is a JSON
// document with base64-embedded asset blobs; these tests construct a
// minimal synthetic scene + one texture, Export to .mistpkg, then
// Import and assert the extracted scene + asset bytes match byte-for-
// byte. Catches regressions in base64 encoding, dependency walking,
// and temp-dir extraction.

namespace {

std::filesystem::path uniqueTempDir(const std::string& tag) {
    std::random_device rd;
    std::mt19937_64 rng(rd());
    std::stringstream ss;
    ss << "mist-test-" << tag << "-" << std::hex << rng();
    auto p = std::filesystem::temp_directory_path() / ss.str();
    std::filesystem::create_directories(p);
    return p;
}

void writeText(const std::filesystem::path& p, const std::string& s) {
    std::filesystem::create_directories(p.parent_path());
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    f << s;
}

std::vector<std::uint8_t> readBinary(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    auto sz = f.tellg(); f.seekg(0);
    std::vector<std::uint8_t> out(static_cast<std::size_t>(sz));
    if (sz > 0) f.read(reinterpret_cast<char*>(out.data()), sz);
    return out;
}

} // namespace

TEST_CASE(".mistpkg exports a scene + dependency bundle", "[mistpkg]") {
    auto dir = uniqueTempDir("pkg-export");

    // Synthesize a 4-byte "PNG-ish" texture blob. Content doesn't
    // matter — the test only checks round-trip integrity.
    std::vector<std::uint8_t> texBytes = {0x89, 'P', 'N', 'G'};
    {
        std::ofstream f(dir / "albedo.png", std::ios::binary | std::ios::trunc);
        f.write(reinterpret_cast<const char*>(texBytes.data()),
                static_cast<std::streamsize>(texBytes.size()));
    }
    writeText(dir / "red.mistmat",
              R"({"type":"PBRMaterial","version":"1.0","albedoTex":"albedo.png"})");

    // Scene JSON with one entity referencing the material.
    writeText(dir / "scene.mist",
              R"({
                "version": "1.0",
                "engine":  "MistEngine",
                "entities": [
                  { "id": 1,
                    "transform": { "pos":[0,0,0], "rot":[0,0,0], "scale":[1,1,1] },
                    "render":    { "mesh":{"builtin":"cube"}, "visible":true,
                                   "materialPath":"red.mistmat" } }
                ]
              })");

    auto pkg = dir / "bundle.mistpkg";
    REQUIRE(Mist::Assets::PackageIO::Export((dir / "scene.mist").string(),
                                             pkg.string()));
    REQUIRE(std::filesystem::exists(pkg));
    REQUIRE(std::filesystem::file_size(pkg) > 0);
}

TEST_CASE(".mistpkg import extracts assets byte-exact to a temp dir", "[mistpkg]") {
    auto dir = uniqueTempDir("pkg-import");

    std::vector<std::uint8_t> origBytes = {0x42, 0x43, 0x44, 0x45, 0x46};
    {
        std::ofstream f(dir / "data.bin", std::ios::binary | std::ios::trunc);
        f.write(reinterpret_cast<const char*>(origBytes.data()),
                static_cast<std::streamsize>(origBytes.size()));
    }
    writeText(dir / "m.mistmat",
              R"({"type":"PBRMaterial","version":"1.0","albedoTex":"data.bin"})");
    writeText(dir / "scene.mist",
              R"({"version":"1.0","engine":"MistEngine","entities":[
                {"id":0,
                 "transform":{"pos":[0,0,0],"rot":[0,0,0],"scale":[1,1,1]},
                 "render":{"mesh":{"builtin":"cube"},"visible":true,
                           "materialPath":"m.mistmat"}}
               ]})");

    auto pkg = dir / "out.mistpkg";
    REQUIRE(Mist::Assets::PackageIO::Export((dir / "scene.mist").string(),
                                             pkg.string()));

    std::string extractedScene;
    REQUIRE(Mist::Assets::PackageIO::Import(pkg.string(), extractedScene));
    REQUIRE(std::filesystem::exists(extractedScene));

    auto extractedDir = std::filesystem::path(extractedScene).parent_path();
    auto extracted = readBinary(extractedDir / "data.bin");
    REQUIRE(extracted == origBytes);
}
