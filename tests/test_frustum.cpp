#include <catch2/catch_all.hpp>

#include "Scene/AABB.h"
#include "Scene/Frustum.h"

#include <glm/gtc/matrix_transform.hpp>

// Frustum culling maths.
//
// Why these exist: the implementation in include/Scene/Frustum.h predates this
// work and had only ever been exercised by src/Scene/SceneGraph.cpp, which was
// unreachable from main() and has since been deleted. So it was written,
// plausible, and never actually run until these cases were added — deliberately
// written before wiring it into the render passes. Plane extraction is also the one place where a column-major/row-major
// mix-up produces planes that are wrong but not obviously wrong — geometry
// disappears at the screen edge rather than everywhere, which is exactly the
// kind of bug that ships.
//
// ExtractFromVP implements Gribb-Hartmann. For a glm matrix, `vp[c][r]` is
// column c row r, so row i of the matrix is
// (vp[0][i], vp[1][i], vp[2][i], vp[3][i]) and the left plane is row3 + row0.
// These cases pin that reading against real matrices.

namespace {

// A camera at +Z looking at the origin down -Z, matching the engine's
// convention (Camera::WorldForward is -Z).
Frustum makeCameraFrustum(float nearP = 0.1f, float farP = 100.0f,
                          float fovDeg = 45.0f, float aspect = 16.0f / 9.0f,
                          const glm::vec3& eye = {0.0f, 0.0f, 10.0f}) {
    const glm::mat4 view = glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0, 1, 0));
    const glm::mat4 proj = glm::perspective(glm::radians(fovDeg), aspect, nearP, farP);
    Frustum f;
    f.ExtractFromVP(proj * view);
    return f;
}

AABB unitBoxAt(const glm::vec3& c, float halfExtent = 0.5f) {
    AABB b;
    b.min = c - glm::vec3(halfExtent);
    b.max = c + glm::vec3(halfExtent);
    return b;
}

} // namespace

TEST_CASE("Frustum planes point inward", "[frustum]") {
    // The sign convention the whole cull depends on: DistanceToPoint is
    // positive inside. If the planes came out flipped, Intersects would reject
    // everything visible and accept everything behind the camera — and the
    // p-vertex test in Intersects would still "work", just inverted.
    const Frustum f = makeCameraFrustum();

    const glm::vec3 origin(0.0f);  // dead centre of the view
    for (int i = 0; i < 6; ++i) {
        INFO("plane index " << i);
        REQUIRE(f.planes[i].DistanceToPoint(origin) > 0.0f);
    }
}

TEST_CASE("Frustum planes are normalized", "[frustum]") {
    // ExtractFromVP divides by the normal's length, so DistanceToPoint returns
    // a true signed distance rather than a scaled one. Nothing here depends on
    // the magnitude today, but a future "how far outside" test would.
    const Frustum f = makeCameraFrustum();
    for (int i = 0; i < 6; ++i) {
        INFO("plane index " << i);
        REQUIRE(glm::length(f.planes[i].normal) == Catch::Approx(1.0f).margin(1e-4));
    }
}

TEST_CASE("Frustum accepts a box in front of the camera", "[frustum]") {
    const Frustum f = makeCameraFrustum();
    REQUIRE(f.Intersects(unitBoxAt({0.0f, 0.0f, 0.0f})));
}

TEST_CASE("Frustum rejects a box behind the camera", "[frustum]") {
    // Camera sits at z=+10 looking toward -Z, so z=+50 is behind it.
    const Frustum f = makeCameraFrustum();
    REQUIRE_FALSE(f.Intersects(unitBoxAt({0.0f, 0.0f, 50.0f})));
}

TEST_CASE("Frustum rejects a box beyond the far plane", "[frustum]") {
    const Frustum f = makeCameraFrustum(0.1f, 100.0f);
    // Camera at z=10 looking down -Z: the far plane is at z = 10 - 100 = -90.
    REQUIRE_FALSE(f.Intersects(unitBoxAt({0.0f, 0.0f, -200.0f})));
}

TEST_CASE("Frustum rejects a box far off to the side", "[frustum]") {
    const Frustum f = makeCameraFrustum();
    REQUIRE_FALSE(f.Intersects(unitBoxAt({500.0f, 0.0f, 0.0f})));
    REQUIRE_FALSE(f.Intersects(unitBoxAt({0.0f, 500.0f, 0.0f})));
}

TEST_CASE("Frustum accepts a box straddling a plane", "[frustum]") {
    // The case that matters most in practice. A cull that only accepted fully
    // contained boxes would pop geometry at the screen edge the moment its
    // centre left the frustum — the classic symptom of a bad cull, and the
    // thing to watch for when checking this in the editor.
    const Frustum f = makeCameraFrustum();

    // A very wide box centred far to the right still overlaps the view volume.
    AABB straddling;
    straddling.min = {-1.0f, -1.0f, -1.0f};
    straddling.max = {500.0f, 1.0f, 1.0f};
    REQUIRE(f.Intersects(straddling));
}

TEST_CASE("Frustum handles an aspect ratio change", "[frustum]") {
    // A point outside a narrow view can be inside a wide one. This is also the
    // invariant ShadowSystem::CalculateCascades violated by hardcoding 1.6
    // while the projection used the real viewport ratio.
    const glm::vec3 offAxis{6.0f, 0.0f, 0.0f};

    const Frustum narrow = makeCameraFrustum(0.1f, 100.0f, 45.0f, 0.5f);
    const Frustum wide   = makeCameraFrustum(0.1f, 100.0f, 45.0f, 4.0f);

    REQUIRE_FALSE(narrow.Intersects(unitBoxAt(offAxis)));
    REQUIRE(wide.Intersects(unitBoxAt(offAxis)));
}

TEST_CASE("An orthographic frustum culls correctly", "[frustum]") {
    // Shadow cascades are fitted with orthographic projections, so the cull
    // used for CSM passes goes through this path rather than the perspective
    // one. Gribb-Hartmann is projection-agnostic; this proves it.
    const glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 10), glm::vec3(0), glm::vec3(0, 1, 0));
    const glm::mat4 proj = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 50.0f);
    Frustum f;
    f.ExtractFromVP(proj * view);

    REQUIRE(f.Intersects(unitBoxAt({0.0f, 0.0f, 0.0f})));
    REQUIRE(f.Intersects(unitBoxAt({4.0f, 4.0f, 0.0f})));
    // Outside the ortho box laterally: a perspective frustum would still catch
    // this at that depth, an orthographic one must not.
    REQUIRE_FALSE(f.Intersects(unitBoxAt({20.0f, 0.0f, 0.0f})));
}

TEST_CASE("AABB::Transform produces a world box that contains the rotated corners",
          "[frustum][aabb]") {
    // Culling transforms a mesh's local AABB by its world matrix each frame.
    // A rotated box's transformed bounds must enclose it — being conservative
    // is fine, being tight is not required, being too small drops geometry.
    AABB local = unitBoxAt({0.0f, 0.0f, 0.0f}, 1.0f);

    const glm::mat4 m =
        glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0, 0, 1));

    const AABB world = local.Transform(m);
    REQUIRE(world.IsValid());

    // Centre moved with the translation.
    REQUIRE(world.Center().x == Catch::Approx(10.0f).margin(1e-4));

    // A 45-degree rotation grows the footprint to sqrt(2) per side.
    REQUIRE(world.Extents().x == Catch::Approx(std::sqrt(2.0f)).margin(1e-4));

    // Every transformed corner is inside the result.
    for (float sx : {-1.0f, 1.0f})
    for (float sy : {-1.0f, 1.0f})
    for (float sz : {-1.0f, 1.0f}) {
        const glm::vec4 c = m * glm::vec4(sx, sy, sz, 1.0f);
        REQUIRE(c.x >= world.min.x - 1e-4f);
        REQUIRE(c.x <= world.max.x + 1e-4f);
        REQUIRE(c.y >= world.min.y - 1e-4f);
        REQUIRE(c.y <= world.max.y + 1e-4f);
    }
}

TEST_CASE("A default-constructed AABB is invalid", "[frustum][aabb]") {
    // min = FLT_MAX, max = lowest. Feeding one to Frustum::Intersects picks
    // nonsense p-vertices, so every cull site has to check IsValid() first and
    // treat "no bounds" as "always draw" — dropping an object because its
    // bounds were never computed is a far worse failure than not culling it.
    AABB fresh;
    REQUIRE_FALSE(fresh.IsValid());

    fresh.Merge(glm::vec3(1.0f));
    REQUIRE(fresh.IsValid());
}


// --- Cull policy ------------------------------------------------------------
//
// RenderSystem::PassesCull is private and every pass needs a live GL context
// to reach, so these cases pin the *policy* it implements rather than calling
// it. The policy is the part that silently loses geometry when it is wrong.

namespace {
// Mirrors RenderSystem::PassesCull exactly. If one changes without the other,
// that is the thing to notice.
bool cullPolicy(const AABB& worldBounds, const Frustum* cull) {
    if (!cull) return true;
    if (!worldBounds.IsValid()) return true;
    return cull->Intersects(worldBounds);
}
} // namespace

TEST_CASE("A null frustum draws everything", "[frustum][cull]") {
    // Passes that do not cull (legacy paths) pass nullptr, and must be
    // unaffected.
    REQUIRE(cullPolicy(unitBoxAt({9999.0f, 0.0f, 0.0f}), nullptr));
}

TEST_CASE("Unknown bounds always draw", "[frustum][cull]") {
    // The single most important case here. A renderable that cannot describe
    // its extent — Orb, or a Mesh with no vertices — has invalid world bounds.
    // Culling it would make it vanish permanently, which is far worse than the
    // draw call not culling saves.
    const Frustum f = makeCameraFrustum();
    AABB unknown;  // default: min = FLT_MAX, max = lowest
    REQUIRE_FALSE(unknown.IsValid());
    REQUIRE(cullPolicy(unknown, &f));
}

TEST_CASE("Known bounds outside the frustum are culled", "[frustum][cull]") {
    const Frustum f = makeCameraFrustum();
    REQUIRE_FALSE(cullPolicy(unitBoxAt({0.0f, 0.0f, 500.0f}), &f));
}

TEST_CASE("A shadow cascade culls against the light, not the camera",
          "[frustum][cull]") {
    // The failure this guards: culling a shadow pass against the camera
    // frustum drops casters that are behind the viewer but still throw light
    // into the view, so shadows disappear as you turn away from the caster.
    //
    // Camera looks down -Z from z=+10. The caster sits behind it at z=+40.
    // A light looking down from above sees the caster; the camera does not.
    const AABB caster = unitBoxAt({0.0f, 0.0f, 40.0f}, 2.0f);

    const Frustum cameraFrustum = makeCameraFrustum();
    REQUIRE_FALSE(cullPolicy(caster, &cameraFrustum));

    const glm::mat4 lightView = glm::lookAt(glm::vec3(0.0f, 60.0f, 40.0f),
                                            glm::vec3(0.0f, 0.0f, 40.0f),
                                            glm::vec3(0, 0, -1));
    const glm::mat4 lightProj = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, 0.1f, 200.0f);
    Frustum lightFrustum;
    lightFrustum.ExtractFromVP(lightProj * lightView);

    REQUIRE(cullPolicy(caster, &lightFrustum));
}
