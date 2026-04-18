#include <catch2/catch_all.hpp>

#include "Renderer/RID.h"

#include <unordered_set>

TEST_CASE("RID default is invalid", "[rid]") {
    RID r;
    REQUIRE_FALSE(r.IsValid());
}

TEST_CASE("RID equality + hash", "[rid]") {
    RID a{7};
    RID b{7};
    RID c{42};

    REQUIRE(a == b);
    REQUIRE(a != c);

    // Hashable for unordered_set / map use — GLRenderingDevice leans on
    // this to track live resources.
    std::unordered_set<RID> s;
    s.insert(a);
    s.insert(c);
    REQUIRE(s.count(b) == 1); // b has same id as a
    REQUIRE(s.count(c) == 1);
    REQUIRE(s.size() == 2);
}
