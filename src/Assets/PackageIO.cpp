#include "Assets/PackageIO.h"

#include "Core/Logger.h"
#include "Core/PathGuard.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <random>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace Mist::Assets {

using json = nlohmann::json;

namespace {

// Base64 encode. Standard table, no line-wrap (JSON doesn't care about
// line length, and nlohmann handles embedded long strings fine).
const char kB64Table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const std::vector<std::uint8_t>& in) {
    std::string out;
    out.reserve(((in.size() + 2) / 3) * 4);
    for (std::size_t i = 0; i < in.size(); i += 3) {
        std::uint32_t t = static_cast<std::uint32_t>(in[i]) << 16;
        if (i + 1 < in.size()) t |= static_cast<std::uint32_t>(in[i + 1]) << 8;
        if (i + 2 < in.size()) t |= static_cast<std::uint32_t>(in[i + 2]);

        out.push_back(kB64Table[(t >> 18) & 0x3F]);
        out.push_back(kB64Table[(t >> 12) & 0x3F]);
        out.push_back(i + 1 < in.size() ? kB64Table[(t >> 6)  & 0x3F] : '=');
        out.push_back(i + 2 < in.size() ? kB64Table[(t >> 0)  & 0x3F] : '=');
    }
    return out;
}

std::vector<std::uint8_t> base64Decode(const std::string& in) {
    static std::int8_t rev[256];
    static bool init = false;
    if (!init) {
        for (int i = 0; i < 256; ++i) rev[i] = -1;
        for (int i = 0; i < 64;  ++i) rev[(unsigned char)kB64Table[i]] = static_cast<std::int8_t>(i);
        init = true;
    }
    std::vector<std::uint8_t> out;
    out.reserve((in.size() / 4) * 3);
    std::uint32_t t = 0;
    int n = 0;
    for (char c : in) {
        if (c == '=' || c == '\n' || c == '\r' || c == ' ') {
            if (c == '=') break;
            continue;
        }
        auto v = rev[static_cast<unsigned char>(c)];
        if (v < 0) continue;
        t = (t << 6) | static_cast<std::uint32_t>(v);
        ++n;
        if (n == 4) {
            out.push_back(static_cast<std::uint8_t>((t >> 16) & 0xFF));
            out.push_back(static_cast<std::uint8_t>((t >>  8) & 0xFF));
            out.push_back(static_cast<std::uint8_t>((t >>  0) & 0xFF));
            t = 0;
            n = 0;
        }
    }
    // Tail handling. `t` already holds the 6-bit groups packed MSB-
    // first; no extra shift is needed — we just mask out the data
    // bits and drop the trailing padding zeros.
    if (n == 2) {
        // 12 bits in t = [char1(6)][char2(6)]; top 8 are the byte,
        // bottom 4 are padding.
        out.push_back(static_cast<std::uint8_t>((t >> 4) & 0xFF));
    } else if (n == 3) {
        // 18 bits in t = [char1(6)][char2(6)][char3(6)]; top 8 are
        // byte1, next 8 are byte2, bottom 2 are padding.
        out.push_back(static_cast<std::uint8_t>((t >> 10) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((t >>  2) & 0xFF));
    }
    return out;
}

bool readFileBinary(const std::filesystem::path& p, std::vector<std::uint8_t>& out) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f) return false;
    auto sz = f.tellg();
    f.seekg(0);
    out.resize(static_cast<std::size_t>(sz));
    if (sz > 0) f.read(reinterpret_cast<char*>(out.data()), sz);
    return f.good() || f.eof();
}

bool writeFileBinary(const std::filesystem::path& p,
                     const std::vector<std::uint8_t>& data) {
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    if (!data.empty()) f.write(reinterpret_cast<const char*>(data.data()),
                                static_cast<std::streamsize>(data.size()));
    return f.good();
}

// Collect asset dependency paths from a loaded scene JSON. Walks the
// entity list, harvests RenderComponent.materialPath strings, then
// opens each material to find its texture paths. Best-effort: missing
// files are logged and skipped rather than failing the whole export.
std::unordered_set<std::string> collectDependencies(const json& sceneJson,
                                                     const std::filesystem::path& sceneDir) {
    std::unordered_set<std::string> deps;
    if (!sceneJson.contains("entities")) return deps;
    for (const auto& e : sceneJson["entities"]) {
        // Prefab instances store a reference, not the expanded subtree, so the
        // .mistprefab file itself is a dependency — without it an exported
        // package reconstructs a scene full of instances whose sources are
        // missing, and every one of them silently vanishes on import.
        if (e.contains("prefab") && e["prefab"].is_object()
            && e["prefab"].contains("path") && e["prefab"]["path"].is_string()) {
            const std::string pp = e["prefab"]["path"].get<std::string>();
            if (!pp.empty()) {
                deps.insert(pp);

                // A prefab carries its own RenderComponents, so its material
                // references have to be harvested too. Nested prefabs are out
                // of scope for this cut, so this does not recurse.
                std::ifstream pf(sceneDir / pp);
                if (pf) {
                    try {
                        json pj; pf >> pj;
                        // Walk every "render" object anywhere in the prefab
                        // tree — the shape is recursive, unlike the flat scene
                        // entity list.
                        std::function<void(const json&)> harvest = [&](const json& node) {
                            if (!node.is_object()) return;
                            if (node.contains("render") && node["render"].is_object()) {
                                const auto& r = node["render"];
                                if (r.contains("materialPath") && r["materialPath"].is_string()) {
                                    const auto mp = r["materialPath"].get<std::string>();
                                    if (!mp.empty()) deps.insert(mp);
                                }
                            }
                            if (node.contains("children") && node["children"].is_array()) {
                                for (const auto& c : node["children"]) harvest(c);
                            }
                        };
                        if (pj.contains("root")) harvest(pj["root"]);
                    } catch (...) {}
                }
            }
        }

        if (!e.contains("render")) continue;
        const auto& r = e["render"];
        if (!r.contains("materialPath")) continue;
        std::string mp = r["materialPath"].get<std::string>();
        if (mp.empty()) continue;
        deps.insert(mp);

        // Peek into the material file for texture refs. Paths are
        // stored under albedoTex / normalTex / etc. We just scan for
        // any string field that looks like an image reference.
        std::filesystem::path matAbs = sceneDir / mp;
        std::ifstream mf(matAbs);
        if (!mf) continue;
        try {
            json mj; mf >> mj;
            for (const auto& [key, val] : mj.items()) {
                if (!val.is_string()) continue;
                std::string s = val.get<std::string>();
                if (s.empty()) continue;
                // Heuristic: asset path keys end in "Tex" in our schema.
                if (key.size() >= 3 &&
                    key.compare(key.size() - 3, 3, "Tex") == 0) {
                    deps.insert(s);
                }
            }
        } catch (...) {}
    }
    return deps;
}

// Extraction directory for an imported package.
//
// This used to return a directory under $TMPDIR, which made
// PackageIO::Import structurally unable to succeed: it handed the extracted
// scene path to UIManager::LoadScene, and SceneSerializer::Load enforces
// `is_under(cwd/"scenes", ...)` — a condition no temp-directory path can ever
// satisfy. So "File -> Import Package" always ended in "Refusing to load
// outside scene sandbox". The two sandboxes were designed independently and
// did not compose.
//
// Extracting under `scenes/imported/` satisfies both: the package's assets
// land next to the scene that references them, and the scene itself is inside
// the sandbox the loader requires.
std::filesystem::path makeTempPkgDir() {
    std::random_device rd;
    std::mt19937_64 rng(rd());
    std::stringstream ss;
    ss << "pkg-" << std::hex << rng();

    std::error_code ec;
    auto cwd = std::filesystem::current_path(ec);
    auto base = (ec ? std::filesystem::path{"scenes"} : cwd / "scenes") / "imported";

    auto p = base / ss.str();
    std::filesystem::create_directories(p, ec);
    if (ec) return {};
    return p;
}

} // namespace

bool PackageIO::Export(const std::string& scenePath, const std::string& pkgPath) {
    std::filesystem::path sp(scenePath);
    if (!std::filesystem::exists(sp)) {
        LOG_ERROR("PackageIO::Export: scene not found: ", scenePath);
        return false;
    }

    json sceneJson;
    {
        std::ifstream f(sp);
        if (!f) { LOG_ERROR("PackageIO::Export: cannot open scene: ", scenePath); return false; }
        try { f >> sceneJson; }
        catch (const std::exception& e) {
            LOG_ERROR("PackageIO::Export: scene parse: ", e.what());
            return false;
        }
    }

    auto deps = collectDependencies(sceneJson, sp.parent_path());

    json pkg = {
        {"version", "1.0"},
        {"scene",   sceneJson},
        {"assets",  json::object()},
    };

    std::size_t packed = 0;
    for (const auto& relPath : deps) {
        std::filesystem::path abs = sp.parent_path() / relPath;
        std::vector<std::uint8_t> bytes;
        if (!readFileBinary(abs, bytes)) {
            LOG_WARN("PackageIO::Export: missing dependency '", relPath, "' — skipping");
            continue;
        }
        pkg["assets"][relPath] = base64Encode(bytes);
        ++packed;
    }

    std::ofstream out(pkgPath);
    if (!out) { LOG_ERROR("PackageIO::Export: cannot write pkg: ", pkgPath); return false; }
    out << pkg.dump(2);
    LOG_INFO("PackageIO::Export: ", pkgPath, " (", packed, " assets bundled)");
    return true;
}

bool PackageIO::Import(const std::string& pkgPath, std::string& outScenePath) {
    std::ifstream in(pkgPath);
    if (!in) { LOG_ERROR("PackageIO::Import: cannot open: ", pkgPath); return false; }

    json pkg;
    try { in >> pkg; }
    catch (const std::exception& e) {
        LOG_ERROR("PackageIO::Import: parse: ", e.what());
        return false;
    }
    if (!pkg.contains("scene") || !pkg.contains("assets")) {
        LOG_ERROR("PackageIO::Import: malformed .mistpkg");
        return false;
    }

    auto tempDir = makeTempPkgDir();
    if (tempDir.empty()) {
        LOG_ERROR("PackageIO::Import: could not create extraction directory");
        return false;
    }

    // Extract asset blobs. Each key is a relative path; we preserve the
    // directory structure so the scene's `materialPath`-relative resolution
    // works without path rewriting.
    //
    // SECURITY: every key comes from the package file and is therefore
    // untrusted. `tempDir / relPath` does NOT contain traversal —
    // std::filesystem::path concatenation happily produces
    // "/etc/cron.d/evil" from "../../../etc/cron.d/evil", and an absolute
    // key replaces the base entirely. That made opening an untrusted
    // .mistpkg an arbitrary-file-write primitive. Every destination now goes
    // through PathGuard, the same boundary already applied to scenes,
    // exports and module loading.
    std::size_t extracted = 0, rejected = 0;
    for (const auto& [relPath, b64] : pkg["assets"].items()) {
        if (!b64.is_string()) continue;

        const std::filesystem::path rel(relPath);
        if (rel.is_absolute() || rel.has_root_name()) {
            LOG_WARN("PackageIO::Import: rejecting absolute asset path: ", relPath);
            ++rejected;
            continue;
        }

        // Resolve and confirm the result is still inside tempDir after `..`
        // and symlink resolution.
        const auto dest = Mist::PathGuard::resolve_under(tempDir, tempDir / rel);
        if (dest.empty()) {
            LOG_WARN("PackageIO::Import: rejecting asset path that escapes the "
                     "extraction directory: ", relPath);
            ++rejected;
            continue;
        }

        auto bytes = base64Decode(b64.get<std::string>());
        if (!writeFileBinary(dest, bytes)) {
            LOG_WARN("PackageIO::Import: failed to write: ", relPath);
            continue;
        }
        ++extracted;
    }
    if (rejected > 0) {
        LOG_ERROR("PackageIO::Import: rejected ", rejected,
                  " asset path(s) that tried to escape the extraction "
                  "directory — treat this package as hostile");
    }

    // Drop the scene alongside its assets so relative paths work.
    auto scenePath = tempDir / "scene.mist";
    {
        std::ofstream sout(scenePath);
        if (!sout) { LOG_ERROR("PackageIO::Import: cannot write scene"); return false; }
        sout << pkg["scene"].dump(2);
    }

    outScenePath = scenePath.string();
    LOG_INFO("PackageIO::Import: extracted ", extracted, " assets into ",
             tempDir.string(), "; scene at ", outScenePath);
    return true;
}

} // namespace Mist::Assets
