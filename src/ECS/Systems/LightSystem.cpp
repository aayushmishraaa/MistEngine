#include "ECS/Systems/LightSystem.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/LightComponent.h"
#include "LightManager.h"
#include "Light.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace {

// Planckian locus fit (McCamy approximation, then Kim et al. fit to
// sRGB). Good enough for 1000..15000 K — the user's expected range.
// Used when a LightComponent has temperatureK > 0.
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

// Build a forward direction from euler rotations stored in degrees.
// Same convention as `TransformComponent::GetModelMatrix`.
glm::vec3 directionFromEuler(const glm::vec3& eulerDeg) {
    glm::mat4 R = glm::mat4(1.0f);
    R = glm::rotate(R, glm::radians(eulerDeg.x), glm::vec3(1, 0, 0));
    R = glm::rotate(R, glm::radians(eulerDeg.y), glm::vec3(0, 1, 0));
    R = glm::rotate(R, glm::radians(eulerDeg.z), glm::vec3(0, 0, 1));
    // Light shines "forward" along -Z in local space, same as Godot.
    return glm::normalize(glm::vec3(R * glm::vec4(0, 0, -1, 0)));
}

} // namespace

void LightSystem::Update(Coordinator& coord, LightManager& lights) {
    // Full-rebuild strategy. Simplest correct behaviour; the flicker-
    // free incremental path lands with shadow atlas in Phase H where
    // we track light stability for atlas LRU.
    while (lights.GetLightCount() > 0) {
        lights.RemoveLight(lights.GetLightCount() - 1);
    }

    // Assign omni shadow atlas indices. First N shadow-enabled omni
    // lights get slots 0..N-1; extras get -1 (unshadowed). Matches
    // Godot's "shadow_atlas_size + light importance" heuristic but
    // simplified to "first come first served" for v1.
    constexpr int MAX_OMNI_SHADOWS = 4;   // matches ShadowSystem
    int omniShadowSlot = 0;

    for (Entity e : m_Entities) {
        auto& t  = coord.GetComponent<TransformComponent>(e);
        auto& lc = coord.GetComponent<LightComponent>(e);

        Light L;
        L.position.x = t.position.x;
        L.position.y = t.position.y;
        L.position.z = t.position.z;
        L.position.w = static_cast<float>(static_cast<int>(lc.type));

        glm::vec3 dir = directionFromEuler(t.rotation);
        L.direction.x = dir.x;
        L.direction.y = dir.y;
        L.direction.z = dir.z;
        L.direction.w = std::cos(glm::radians(lc.innerCone));

        glm::vec3 c = lc.color;
        if (lc.temperatureK > 0.0f) c *= kelvinToRGB(lc.temperatureK);
        L.color.x = c.x;
        L.color.y = c.y;
        L.color.z = c.z;
        L.color.w = lc.energy;

        L.params.x = lc.range;
        L.params.y = std::cos(glm::radians(lc.outerCone));

        // Shadow index: only omni lights with shadowEnabled get a
        // slot in the omni atlas; fill 0..N-1 then stop. Directional
        // uses CSM (separate path); spot shadows deferred to a later
        // atlas. -1 = no shadow.
        L.params.z = -1.0f;
        if (lc.type == MistLightType::Omni && lc.shadowEnabled
            && omniShadowSlot < MAX_OMNI_SHADOWS) {
            L.params.z = static_cast<float>(omniShadowSlot++);
        }
        L.params.w = 0.0f;

        lights.AddLight(L);
    }

    lights.MarkLightsDirty();
}
