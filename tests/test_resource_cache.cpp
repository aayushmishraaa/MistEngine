#include <catch2/catch_all.hpp>

#include "Resources/ResourceHandle.h"
#include "Resources/ResourceManager.h"

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>

// A stand-in for a real asset type — keeps the test independent of Mesh/Shader
// so it stays green even if those classes evolve.
namespace {
struct FakeAsset {
    int counter;
    std::string origin;
};
} // namespace

TEST_CASE("ResourceManager dedupes loads by path", "[resource][cache]") {
    ResourceManager<FakeAsset> mgr;
    int loader_calls = 0;
    mgr.SetLoader([&](const std::string& path) {
        ++loader_calls;
        return std::make_shared<FakeAsset>(FakeAsset{loader_calls, path});
    });

    auto h1 = mgr.Load("res://fake/a");
    auto h2 = mgr.Load("res://fake/a");
    auto h3 = mgr.Load("res://fake/b");

    REQUIRE(h1.IsValid());
    REQUIRE(h1 == h2);           // same path → same handle
    REQUIRE(h1 != h3);           // different path → different handle
    REQUIRE(loader_calls == 2);  // loader ran for /a once, /b once
}

TEST_CASE("ResourceManager returns nullptr for missing loader", "[resource]") {
    ResourceManager<FakeAsset> mgr;
    // No SetLoader call.
    auto h = mgr.Load("res://anything");
    REQUIRE_FALSE(h.IsValid());
}

TEST_CASE("ResourceManager::LoadAsync runs concurrently", "[resource][async]") {
    // 10 parallel loads against a loader that sleeps 20ms each. If the
    // scheduler really ran them sequentially we'd need ~200ms; in parallel
    // it should be ~20-40ms. Generous bound so slow CI doesn't flake.
    ResourceManager<FakeAsset> mgr;
    mgr.SetLoader([](const std::string& path) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        return std::make_shared<FakeAsset>(FakeAsset{0, path});
    });

    const auto t0 = std::chrono::steady_clock::now();
    std::vector<std::future<ResourceHandle<FakeAsset>>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(mgr.LoadAsync("res://async-" + std::to_string(i)));
    }
    for (auto& f : futures) {
        auto h = f.get();
        REQUIRE(h.IsValid());
    }
    const auto elapsed = std::chrono::steady_clock::now() - t0;
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    REQUIRE(ms < 150); // single-threaded would be ~200ms
    REQUIRE(mgr.Count() == 10);
}

TEST_CASE("ResourceManager::LoadAsync surfaces failure status", "[resource][async]") {
    ResourceManager<FakeAsset> mgr;
    mgr.SetLoader([](const std::string&) -> std::shared_ptr<FakeAsset> {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return nullptr;
    });

    auto fut    = mgr.LoadAsync("res://nope");
    auto handle = fut.get();
    REQUIRE_FALSE(handle.IsValid());
    REQUIRE(mgr.GetStatus("res://nope") == ResourceManager<FakeAsset>::LoadStatus::Failed);
}

TEST_CASE("ResourceManager release removes the cache entry", "[resource][cache]") {
    ResourceManager<FakeAsset> mgr;
    int loader_calls = 0;
    mgr.SetLoader([&](const std::string& path) {
        ++loader_calls;
        return std::make_shared<FakeAsset>(FakeAsset{loader_calls, path});
    });

    auto h1 = mgr.Load("res://x");
    REQUIRE(h1.IsValid());
    REQUIRE(mgr.Count() == 1);

    mgr.Release(h1);
    REQUIRE(mgr.Count() == 0);

    // Re-loading produces a *new* handle + a fresh loader invocation,
    // confirming the prior entry was evicted rather than kept silently.
    auto h2 = mgr.Load("res://x");
    REQUIRE(h2.IsValid());
    REQUIRE(h2 != h1);
    REQUIRE(loader_calls == 2);
}
