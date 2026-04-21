#include <catch2/catch_all.hpp>

#include "Core/Reflection.h"
#include "ECS/Components/LightComponent.h"

#include <set>
#include <string>

// G.1 — physical photometric units for LightComponent. The new
// lumens / candela / lux fields are reflected so the Inspector sees
// them, and defaults to 0 so pre-existing scenes (which drive lights
// via raw `energy`) stay unchanged. Value->energy conversion lives
// in LightSystem and can't be unit-tested headless, but the pinning
// tests here catch regressions in the authoring surface.

TEST_CASE("LightComponent exposes physical units", "[light][units]") {
    const auto* props = Mist::TypeRegistry::Instance().Get("LightComponent");
    REQUIRE(props != nullptr);

    std::set<std::string> names;
    for (const auto& p : *props) names.emplace(p.name);

    REQUIRE(names.count("lumens")  == 1);
    REQUIRE(names.count("candela") == 1);
    REQUIRE(names.count("lux")     == 1);
}

TEST_CASE("LightComponent physical units default to 0 (disabled)", "[light][units]") {
    LightComponent lc;
    REQUIRE(lc.lumens  == Catch::Approx(0.0f));
    REQUIRE(lc.candela == Catch::Approx(0.0f));
    REQUIRE(lc.lux     == Catch::Approx(0.0f));
    // And `energy` stays at 1.0 so raw-energy scenes still light up.
    REQUIRE(lc.energy  == Catch::Approx(1.0f));
}
