#include "Version.h"

#include <catch2/catch_all.hpp>
#include <string_view>

TEST_CASE("Version numbers are in lockstep", "[version]") {
    // Catches the kind of drift we had pre-0.4.2 where the README, CMakeLists,
    // and Version.h all advertised different versions. Tests here are cheap
    // and stop that particular paper-cut from returning.
    REQUIRE(MIST_ENGINE_VERSION_MAJOR == 0);
    REQUIRE(MIST_ENGINE_VERSION_MINOR == 5);
    REQUIRE(MIST_ENGINE_VERSION_PATCH == 0);
    REQUIRE(std::string_view(MIST_ENGINE_VERSION_STRING) == "0.5.0-prealpha");
}
