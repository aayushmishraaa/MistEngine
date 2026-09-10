#include <catch2/catch_all.hpp>

#include "Core/Reflection.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/TransformComponent.h"

#include <cstdint>
#include <cstring>
#include <string>

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

// --- Enum reflection -------------------------------------------------------

namespace {
enum class ReflectedShape : std::uint8_t { Box = 0, Sphere = 1, Capsule = 2 };
enum class WideEnum : std::uint32_t { A = 0, B = 7 };
} // namespace

TEST_CASE("Scoped enums map to PropertyType::Enum, not Unknown", "[reflection][enum]") {
    // `std::is_integral_v` is FALSE for a scoped enum, so the original
    // mist_type_tag fell through to Unknown for every enum field. That is how
    // PhysicsComponent::shape ended up un-editable in the Inspector (it
    // rendered a greyed "unreflected type" label) and silently absent from both
    // the scene and material serializers.
    REQUIRE(Mist::mist_type_tag<ReflectedShape>() == Mist::PropertyType::Enum);
    REQUIRE(Mist::mist_type_tag<WideEnum>()       == Mist::PropertyType::Enum);

    // Must not have been swallowed by the integral branch.
    REQUIRE(Mist::mist_type_tag<int>()  == Mist::PropertyType::Int);
    REQUIRE(Mist::mist_type_tag<bool>() == Mist::PropertyType::Bool);
}

TEST_CASE("enum_value respects the underlying width", "[reflection][enum]") {
    // A uint8_t-backed enum read through an `int*` would pull in three bytes of
    // whatever follows it in the struct, so the accessors are width-aware.
    ReflectedShape s = ReflectedShape::Capsule;
    REQUIRE(Mist::enum_value(&s, sizeof(s)) == 2);

    Mist::set_enum_value(&s, sizeof(s), 1);
    REQUIRE(s == ReflectedShape::Sphere);

    WideEnum w = WideEnum::B;
    REQUIRE(Mist::enum_value(&w, sizeof(w)) == 7);
    Mist::set_enum_value(&w, sizeof(w), 0);
    REQUIRE(w == WideEnum::A);
}

TEST_CASE("Neighbouring fields survive an enum write", "[reflection][enum]") {
    // The concrete hazard of treating a 1-byte enum as an int.
    struct Packed {
        ReflectedShape shape = ReflectedShape::Box;
        std::uint8_t   a = 0xAA, b = 0xBB, c = 0xCC;
    } p;
    Mist::set_enum_value(&p.shape, sizeof(p.shape), 2);
    REQUIRE(p.shape == ReflectedShape::Capsule);
    REQUIRE(p.a == 0xAA);
    REQUIRE(p.b == 0xBB);
    REQUIRE(p.c == 0xCC);
}

TEST_CASE("parse_enum_hint splits labels in declaration order", "[reflection][enum]") {
    auto labels = Mist::parse_enum_hint("Box,Sphere,Capsule,StaticPlane");
    REQUIRE(labels.size() == 4);
    REQUIRE(labels[0] == "Box");
    REQUIRE(labels[3] == "StaticPlane");

    // Spaces after commas are tolerated.
    auto spaced = Mist::parse_enum_hint("Directional, Omni, Spot");
    REQUIRE(spaced.size() == 3);
    REQUIRE(spaced[1] == "Omni");

    REQUIRE(Mist::parse_enum_hint("").empty());
}

TEST_CASE("PhysicsComponent::shape is reflected and editable", "[reflection][enum][physics]") {
    // The field scripts/showcase.lua tells the user to change in the Inspector.
    const auto* props = Mist::TypeRegistry::Instance().Get("PhysicsComponent");
    REQUIRE(props != nullptr);

    const Mist::PropertyInfo* shape = nullptr;
    for (const auto& p : *props) {
        if (std::string(p.name) == "shape") { shape = &p; break; }
    }
    REQUIRE(shape != nullptr);
    REQUIRE(shape->type == Mist::PropertyType::Enum);
    REQUIRE(shape->hint == Mist::PropertyHint::Enum);
    // Labels must cover every CollisionShape enumerator.
    REQUIRE(Mist::parse_enum_hint(shape->hintString).size() == 4);
}
