#include <catch2/catch_all.hpp>

#include "Assets/MaterialSerializer.h"
#include "Core/Reflection.h"
#include "Material.h"

#include <filesystem>
#include <fstream>
#include <set>
#include <string>

// PBRMaterial is the engine's first reflected *asset* (distinct from
// ECS components). These tests pin (1) the full authoring field list
// in the TypeRegistry so an Inspector regression is caught, and (2)
// a round-trip through .mistmat JSON so every field survives save
// + load with no silent drops.

TEST_CASE("PBRMaterial reflects all authoring fields", "[material][reflection]") {
    const auto* props = Mist::TypeRegistry::Instance().Get("PBRMaterial");
    REQUIRE(props != nullptr);

    std::set<std::string> names;
    for (const auto& p : *props) names.emplace(p.name);

    // 6 scalars + 5 texture path refs = 11 authoring fields. Runtime
    // shared_ptr<Texture> handles are intentionally NOT reflected.
    REQUIRE(names.count("albedo")               == 1);
    REQUIRE(names.count("metallic")             == 1);
    REQUIRE(names.count("roughness")            == 1);
    REQUIRE(names.count("ao")                   == 1);
    REQUIRE(names.count("emissive")             == 1);
    REQUIRE(names.count("emissiveIntensity")    == 1);
    REQUIRE(names.count("albedoTex")            == 1);
    REQUIRE(names.count("normalTex")            == 1);
    REQUIRE(names.count("metallicTex")          == 1);
    REQUIRE(names.count("roughnessTex")         == 1);
    REQUIRE(names.count("aoTex")                == 1);
    REQUIRE(names.count("emissiveTex")          == 1);

    REQUIRE(names.count("albedoMap")   == 0);
    REQUIRE(names.count("normalMap")   == 0);
}

TEST_CASE("MaterialSerializer round-trips every reflected field", "[material][serializer]") {
    PBRMaterial src;
    src.albedo            = {0.25f, 0.5f, 0.75f};
    src.metallic          = 0.87f;
    src.roughness         = 0.33f;
    src.ao                = 0.9f;
    src.emissive          = {0.1f, 0.2f, 0.3f};
    src.emissiveIntensity = 4.2f;
    src.albedoTex    = "res://textures/albedo.png";
    src.normalTex    = "res://textures/normal.png";
    src.metallicTex  = "res://textures/metallic.png";
    src.roughnessTex = "res://textures/roughness.png";
    src.aoTex        = "res://textures/ao.png";
    src.emissiveTex  = "res://textures/emi.png";

    auto tmp = std::filesystem::temp_directory_path() / "mist_test_material.mistmat";
    REQUIRE(Mist::Assets::MaterialSerializer::Save(src, tmp.string()));

    PBRMaterial dst;
    REQUIRE(Mist::Assets::MaterialSerializer::Load(tmp.string(), dst));

    REQUIRE(dst.albedo.r              == Catch::Approx(0.25f));
    REQUIRE(dst.albedo.g              == Catch::Approx(0.5f));
    REQUIRE(dst.albedo.b              == Catch::Approx(0.75f));
    REQUIRE(dst.metallic              == Catch::Approx(0.87f));
    REQUIRE(dst.roughness             == Catch::Approx(0.33f));
    REQUIRE(dst.ao                    == Catch::Approx(0.9f));
    REQUIRE(dst.emissive.r            == Catch::Approx(0.1f));
    REQUIRE(dst.emissiveIntensity     == Catch::Approx(4.2f));
    REQUIRE(dst.albedoTex    == "res://textures/albedo.png");
    REQUIRE(dst.normalTex    == "res://textures/normal.png");
    REQUIRE(dst.metallicTex  == "res://textures/metallic.png");
    REQUIRE(dst.roughnessTex == "res://textures/roughness.png");
    REQUIRE(dst.aoTex        == "res://textures/ao.png");
    REQUIRE(dst.emissiveTex  == "res://textures/emi.png");

    std::error_code ec;
    std::filesystem::remove(tmp, ec);
}

TEST_CASE("MaterialSerializer defaults fill gracefully when fields missing",
          "[material][serializer]") {
    // Hand-craft a minimal JSON missing half the fields; Load should
    // leave those at their struct defaults without throwing.
    auto tmp = std::filesystem::temp_directory_path() / "mist_test_partial.mistmat";
    {
        std::ofstream f(tmp);
        f << R"({"type":"PBRMaterial","version":"1.0","albedo":[1,0,0]})";
    }

    PBRMaterial dst;
    REQUIRE(Mist::Assets::MaterialSerializer::Load(tmp.string(), dst));
    REQUIRE(dst.albedo.r      == Catch::Approx(1.0f));
    REQUIRE(dst.albedo.g      == Catch::Approx(0.0f));
    REQUIRE(dst.roughness     == Catch::Approx(0.5f));   // default
    REQUIRE(dst.metallic      == Catch::Approx(0.0f));   // default
    REQUIRE(dst.emissiveIntensity == Catch::Approx(1.0f));
    REQUIRE(dst.albedoTex.empty());

    std::error_code ec;
    std::filesystem::remove(tmp, ec);
}
