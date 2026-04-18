#include <catch2/catch_all.hpp>

#include "Renderer/RenderingDevice.h"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>

// Headless contract tests. `GLRenderingDevice` itself needs a live GL
// context and is covered under MIST_TEST_GL; here we validate the
// interface contract any future backend (Vulkan, D3D12, software stub)
// must honour.

namespace {

class MockDevice : public Mist::GPU::RenderingDevice {
public:
    // Every Create* returns a unique RID that Destroy later removes. A
    // monotonically-increasing counter mirrors GLRenderingDevice's
    // `m_NextId` so the uniqueness guarantee is part of the contract,
    // not just an implementation accident.
    RID CreateTexture(const Mist::GPU::TextureDesc&)           override { return alloc(); }
    RID CreateTextureArray(const Mist::GPU::TextureArrayDesc&) override { return alloc(); }
    RID CreateBuffer(const Mist::GPU::BufferDesc&)             override { return alloc(); }
    RID CreateShader(const Mist::GPU::ShaderDesc&)             override { return alloc(); }
    RID CreateShaderProgram(const Mist::GPU::ProgramDesc&)     override { return alloc(); }

    void Destroy(RID r) override {
        // Invalid RIDs are a no-op, per interface contract.
        if (!r.IsValid()) return;
        m_Live.erase(r.id);
    }

    const char* GetBackendName() const override { return "MockDevice"; }

    bool IsLive(RID r) const { return r.IsValid() && m_Live.count(r.id) != 0; }
    std::size_t LiveCount() const { return m_Live.size(); }

private:
    RID alloc() {
        RID r{++m_Next};
        m_Live.insert(r.id);
        return r;
    }
    std::uint64_t                m_Next = 0;
    std::unordered_set<std::uint64_t> m_Live;
};

} // namespace

TEST_CASE("RenderingDevice returns unique RIDs across create calls", "[rid][device]") {
    MockDevice dev;

    RID a = dev.CreateTexture({});
    RID b = dev.CreateTexture({});
    RID c = dev.CreateBuffer({});
    RID d = dev.CreateShader({});
    RID e = dev.CreateShaderProgram({});

    // A freshly created RID must be valid (non-zero id).
    REQUIRE(a.IsValid());
    REQUIRE(b.IsValid());
    REQUIRE(c.IsValid());
    REQUIRE(d.IsValid());
    REQUIRE(e.IsValid());

    // Uniqueness is the core guarantee: if any two calls returned the same
    // id, Destroy on one would silently free the other.
    std::unordered_set<RID> seen;
    for (RID r : {a, b, c, d, e}) seen.insert(r);
    REQUIRE(seen.size() == 5);
}

TEST_CASE("Destroy removes the resource and is idempotent", "[rid][device]") {
    MockDevice dev;
    RID r = dev.CreateTexture({});
    REQUIRE(dev.IsLive(r));

    dev.Destroy(r);
    REQUIRE_FALSE(dev.IsLive(r));

    // Destroying an already-destroyed RID must not crash and must not
    // resurrect phantom entries.
    dev.Destroy(r);
    REQUIRE(dev.LiveCount() == 0);
}

TEST_CASE("Destroy on an invalid RID is a no-op", "[rid][device]") {
    MockDevice dev;
    RID live = dev.CreateBuffer({});
    REQUIRE(dev.LiveCount() == 1);

    // Default-constructed RID is invalid; Destroy must not touch the live
    // set (it could, for instance, accidentally wipe the whole map on a
    // 0-key lookup).
    dev.Destroy(RID{});
    REQUIRE(dev.LiveCount() == 1);
    REQUIRE(dev.IsLive(live));
}

TEST_CASE("GetBackendName is never null", "[device]") {
    MockDevice dev;
    const char* name = dev.GetBackendName();
    REQUIRE(name != nullptr);
    REQUIRE(std::string(name).size() > 0);
}

TEST_CASE("Device()/SetDevice() round-trip", "[device]") {
    MockDevice dev;

    // Default state is nullptr — Renderer::Init is the only place that
    // should set this, so tests that don't want the global state dirtied
    // must reset afterwards (which this test does).
    Mist::GPU::SetDevice(&dev);
    REQUIRE(Mist::GPU::Device() == &dev);

    Mist::GPU::SetDevice(nullptr);
    REQUIRE(Mist::GPU::Device() == nullptr);
}
