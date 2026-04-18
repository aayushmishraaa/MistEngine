#include <catch2/catch_all.hpp>

#include "Core/Reflection.h"
#include "ECS/Components/TransformComponent.h"

#include <cstring>

TEST_CASE("TypeRegistry exposes TransformComponent fields", "[reflection]") {
    const auto* props = Mist::TypeRegistry::Instance().Get("TransformComponent");
    REQUIRE(props != nullptr);
    REQUIRE(props->size() == 3);

    // Fields are stored in registration order: position, rotation, scale.
    REQUIRE(std::strcmp((*props)[0].name, "position") == 0);
    REQUIRE((*props)[0].type == Mist::PropertyType::Vec3);

    REQUIRE(std::strcmp((*props)[1].name, "rotation") == 0);
    REQUIRE(std::strcmp((*props)[2].name, "scale")    == 0);

    // Offsets should match offsetof exactly.
    REQUIRE((*props)[0].offset == offsetof(TransformComponent, position));
    REQUIRE((*props)[1].offset == offsetof(TransformComponent, rotation));
    REQUIRE((*props)[2].offset == offsetof(TransformComponent, scale));
}

TEST_CASE("Reflection offsets allow generic read/write", "[reflection]") {
    // The inspector will use offset+type to read/write fields generically.
    // This test emulates that path so we catch layout drift (eg. someone adds
    // a member in between and forgets to re-order MIST_FIELD declarations).
    const auto* props = Mist::TypeRegistry::Instance().Get("TransformComponent");
    REQUIRE(props != nullptr);

    TransformComponent t;
    t.position = {0, 0, 0};

    // Write via offset
    void* field = reinterpret_cast<char*>(&t) + (*props)[0].offset;
    *reinterpret_cast<glm::vec3*>(field) = glm::vec3{1.0f, 2.0f, 3.0f};

    REQUIRE(t.position.x == Catch::Approx(1.0f));
    REQUIRE(t.position.y == Catch::Approx(2.0f));
    REQUIRE(t.position.z == Catch::Approx(3.0f));
}

TEST_CASE("parse_range_hint handles min,max and min,max,step", "[reflection]") {
    float lo, hi, step;
    REQUIRE(Mist::parse_range_hint("0,100", lo, hi, step));
    REQUIRE(lo == Catch::Approx(0.0f));
    REQUIRE(hi == Catch::Approx(100.0f));
    REQUIRE(step == Catch::Approx(0.01f)); // default step

    REQUIRE(Mist::parse_range_hint("-1.5,1.5,0.1", lo, hi, step));
    REQUIRE(lo == Catch::Approx(-1.5f));
    REQUIRE(hi == Catch::Approx(1.5f));
    REQUIRE(step == Catch::Approx(0.1f));

    REQUIRE_FALSE(Mist::parse_range_hint("garbage", lo, hi, step));
    REQUIRE_FALSE(Mist::parse_range_hint("", lo, hi, step));
}
