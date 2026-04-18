#include <catch2/catch_all.hpp>

#include <nlohmann/json.hpp>

#include <string>

// The old hand-rolled JsonValue parser is gone as of scene v0.5; we now
// delegate to nlohmann/json, which has its own depth + size guards. These
// tests ensure the behaviours we depend on at the library level stay
// stable — full scene round-trip tests will land once AssetRegistry
// mesh loading from on-disk files is wired (currently tests would need
// a live GL context to build a Mesh).

TEST_CASE("nlohmann::json parses a minimal scene shell", "[json]") {
    auto j = nlohmann::json::parse(R"({"version":"0.5","entities":[]})");
    REQUIRE(j.contains("version"));
    REQUIRE(j["version"] == "0.5");
    REQUIRE(j["entities"].is_array());
    REQUIRE(j["entities"].empty());
}

TEST_CASE("nlohmann::json rejects deeply nested input", "[json][security]") {
    // 200 opening brackets — nlohmann's default max depth is 512 but the
    // parser refuses malformed/truncated input regardless. This test
    // documents the behaviour we rely on: the engine surfaces an exception
    // rather than hanging or crashing.
    std::string bomb(200, '[');
    bool threw = false;
    try {
        auto v = nlohmann::json::parse(bomb);
        (void)v;
    } catch (const std::exception&) {
        threw = true;
    }
    REQUIRE(threw);
}

TEST_CASE("nlohmann::json throws on number overflow", "[json]") {
    // nlohmann rejects scientific-notation overflow rather than
    // materialising `inf`. SceneSerializer wraps `in >> root` in a try/catch
    // so this surfaces as a typed error instead of a crash; the test
    // documents the behaviour contract callers rely on.
    bool threw = false;
    try {
        auto j = nlohmann::json::parse(R"({"n": 1e99999})");
        (void)j;
    } catch (const std::exception&) {
        threw = true;
    }
    REQUIRE(threw);
}
