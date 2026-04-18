#include "Audio/AudioClip.h"

#include <catch2/catch_all.hpp>

// AudioClip is a POD; this test just freezes its default-init invariants so a
// future refactor of the struct doesn't silently change what PlaySound sees.
TEST_CASE("AudioClip defaults are safe to use before loading", "[audio]") {
    AudioClip c;
    REQUIRE(c.loaded == false);
    REQUIRE(c.name.empty());
    REQUIRE(c.filePath.empty());
}
