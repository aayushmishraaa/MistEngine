#include <catch2/catch_all.hpp>

#include <glm/glm.hpp>
#include <cmath>

// Reference Kelvin -> sRGB (McCamy + Kim fit) implementation. Copy
// of LightSystem's internal function so tests don't need to link
// against the private helper. Kept at anonymous namespace scope so
// it doesn't collide with the real one.
namespace {
glm::vec3 kelvinToRGB(float K) {
    K = glm::clamp(K, 1000.0f, 15000.0f);
    float t = K / 100.0f;
    float r, g, b;
    if (t <= 66.0f) {
        r = 255.0f;
        g = 99.4708025861f * std::log(t) - 161.1195681661f;
        if (t <= 19.0f) b = 0.0f;
        else            b = 138.5177312231f * std::log(t - 10.0f) - 305.0447927307f;
    } else {
        r = 329.698727446f * std::pow(t - 60.0f, -0.1332047592f);
        g = 288.1221695283f * std::pow(t - 60.0f, -0.0755148492f);
        b = 255.0f;
    }
    return glm::vec3(
        glm::clamp(r / 255.0f, 0.0f, 1.0f),
        glm::clamp(g / 255.0f, 0.0f, 1.0f),
        glm::clamp(b / 255.0f, 0.0f, 1.0f));
}
} // namespace

TEST_CASE("Kelvin 6500K is approximately neutral white", "[kelvin]") {
    // Daylight standard (D65) locus sits near 6500K. Tint should be
    // very close to (1, 1, 1) — within a few percent is fine for the
    // McCamy fit.
    glm::vec3 c = kelvinToRGB(6500.0f);
    REQUIRE(c.r == Catch::Approx(1.0f).margin(0.05));
    REQUIRE(c.g >= 0.9f);
    REQUIRE(c.b >= 0.9f);
}

TEST_CASE("Kelvin 3000K is warm (red dominant)", "[kelvin]") {
    // Tungsten lamp ~3000K. Red stays near 1.0, blue drops a lot.
    glm::vec3 c = kelvinToRGB(3000.0f);
    REQUIRE(c.r > c.g);
    REQUIRE(c.g > c.b);
    REQUIRE(c.r == Catch::Approx(1.0f).margin(0.01));
}

TEST_CASE("Kelvin 10000K is cool (blue dominant)", "[kelvin]") {
    // Clear blue sky ~10000K. Blue near 1.0, red drops noticeably.
    glm::vec3 c = kelvinToRGB(10000.0f);
    REQUIRE(c.b == Catch::Approx(1.0f).margin(0.01));
    REQUIRE(c.b > c.r);
}

TEST_CASE("Kelvin is clamped to valid range", "[kelvin]") {
    // Extreme inputs shouldn't blow up — fit clamps to 1000..15000
    // before the polynomial evaluation.
    glm::vec3 lo = kelvinToRGB(100.0f);      // below clamp
    glm::vec3 hi = kelvinToRGB(100000.0f);   // above clamp

    // Below-clamp value should equal the 1000K fit result.
    glm::vec3 at1000 = kelvinToRGB(1000.0f);
    REQUIRE(lo.r == Catch::Approx(at1000.r));
    REQUIRE(lo.g == Catch::Approx(at1000.g));
    REQUIRE(lo.b == Catch::Approx(at1000.b));

    // Above-clamp value should equal the 15000K fit result.
    glm::vec3 at15000 = kelvinToRGB(15000.0f);
    REQUIRE(hi.r == Catch::Approx(at15000.r));
    REQUIRE(hi.g == Catch::Approx(at15000.g));
    REQUIRE(hi.b == Catch::Approx(at15000.b));
}
