#include <catch2/catch_all.hpp>

#include <algorithm>

// Replicates the accumulator used inside MistEngine's main loop so we can
// unit-test the timing behaviour without standing up a full engine.
//
// Contract:
//   - Physics advances in fixed `step` chunks no matter the frame delta.
//   - A very long frame (e.g. a 1s GC stall) is clamped to `maxFrameDelta`
//     so we never fall into the "spiral of death" where we try to catch up
//     with thousands of physics steps and just fall further behind.
namespace {
struct Accumulator {
    float acc = 0.0f;
    int   Tick(float frameDelta, float step, float maxFrameDelta) {
        frameDelta = std::min(frameDelta, maxFrameDelta);
        acc += frameDelta;
        int steps = 0;
        while (acc >= step) {
            acc -= step;
            ++steps;
        }
        return steps;
    }
};
} // namespace

TEST_CASE("Fixed-timestep accumulator matches physics rate", "[fixed_timestep]") {
    constexpr float step = 1.0f / 60.0f;
    constexpr float cap  = 0.25f;

    Accumulator acc;

    // A 1/30 frame should run physics twice.
    REQUIRE(acc.Tick(1.0f / 30.0f, step, cap) == 2);

    // A 1/120 frame shouldn't run any physics (accumulator rolls over).
    int first = acc.Tick(1.0f / 120.0f, step, cap);
    REQUIRE(first == 0);

    // Two 1/120 frames in a row cross one physics boundary total (still
    // with the leftover from the previous 1/30 frame partially absorbed).
    int second = acc.Tick(1.0f / 120.0f, step, cap);
    // Should be exactly 1 step because the cumulative time is 2/120 = 1/60.
    REQUIRE(second == 1);
}

TEST_CASE("Fixed-timestep clamps catastrophic frame deltas", "[fixed_timestep][security]") {
    constexpr float step = 1.0f / 60.0f;
    constexpr float cap  = 0.25f;

    Accumulator acc;

    // 1 full second of "stalled" time — without the clamp we'd try to run
    // 60 physics steps in one frame, digging us deeper. With the clamp the
    // accumulator only gets 0.25s worth, i.e. at most 15 steps.
    int steps = acc.Tick(1.0f, step, cap);
    REQUIRE(steps <= 15);
    REQUIRE(steps >= 14);
}

TEST_CASE("Fixed-timestep converges to the same step count over time", "[fixed_timestep]") {
    // Run 300 frames at each of 30/60/144 Hz, confirm total physics steps
    // is within a step or two of 60 * (300 * frame_period) for each.
    constexpr float step = 1.0f / 60.0f;
    constexpr float cap  = 0.25f;

    auto run = [&](float frameDelta, int frames) {
        Accumulator acc;
        int total = 0;
        for (int i = 0; i < frames; ++i) total += acc.Tick(frameDelta, step, cap);
        return total;
    };

    // 5 seconds of wall time at each rate:
    //   30 Hz:  150 frames * 1/30 = 5.0s  -> expect ~300 physics steps
    //   60 Hz:  300 frames * 1/60 = 5.0s  -> expect ~300
    //   144 Hz: 720 frames * 1/144 = 5.0s -> expect ~300
    REQUIRE(run(1.0f / 30.0f,  150) == 300);
    REQUIRE(run(1.0f / 60.0f,  300) == 300);
    // 144 Hz is not an integer multiple of 60 Hz; allow a ± 1 tolerance.
    int highRate = run(1.0f / 144.0f, 720);
    REQUIRE(highRate >= 299);
    REQUIRE(highRate <= 301);
}
