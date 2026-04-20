#ifndef LIGHTCOMPONENT_H
#define LIGHTCOMPONENT_H

#include "Core/Reflection.h"

#include <glm/glm.hpp>
#include <cstdint>

// Godot-shaped Light3D data, attached to any entity to make it a
// light source. LightSystem reads {Transform, LightComponent} each
// frame and syncs into LightManager (which uploads to the GPU SSBOs
// the PBR shader reads). Light *position* + *direction* come from
// the entity's TransformComponent; this struct only carries the
// light's own parameters.
//
// Type encodes the Godot Light3D hierarchy. Area lights deferred to
// a future cycle (Heitz LTC BRDF).
enum class MistLightType : uint8_t {
    Directional = 0,
    Omni        = 1,
    Spot        = 2,
};

struct LightComponent {
    MistLightType type     = MistLightType::Omni;
    glm::vec3     color    = {1.0f, 1.0f, 1.0f}; // linear RGB
    float         energy   = 1.0f;               // Godot's light_energy; 1.0 = baseline
    float         specular = 0.5f;               // specular contribution (v1 unused in shader)
    float         range    = 10.0f;              // world-units; omni/spot only
    // Spot cone angles in degrees — CPU-side converts to cos on
    // upload so the shader can `dot(-L, spotDir) vs innerCos` cheaply.
    float innerCone = 25.0f;
    float outerCone = 35.0f;

    // Shadow knobs (Phase E wires these into per-light shader paths).
    bool  shadowEnabled   = true;
    float shadowBias      = 0.005f;
    float shadowNormalBias= 1.0f;
    float shadowBlur      = 1.0f;
    float shadowOpacity   = 1.0f;

    // Color temperature in Kelvin (1000..15000). 0 disables the tint
    // and uses `color` as-is. Applied CPU-side before upload.
    float temperatureK = 0.0f;
};

MIST_REFLECT(LightComponent)
    MIST_FIELD(LightComponent, color,    ::Mist::PropertyHint::Color, "")
    MIST_FIELD(LightComponent, energy,   ::Mist::PropertyHint::Range, "0.0,20.0")
    MIST_FIELD(LightComponent, range,    ::Mist::PropertyHint::Range, "0.1,200.0")
    MIST_FIELD(LightComponent, innerCone,::Mist::PropertyHint::Range, "0,90")
    MIST_FIELD(LightComponent, outerCone,::Mist::PropertyHint::Range, "0,90")
    MIST_FIELD(LightComponent, shadowEnabled,   ::Mist::PropertyHint::None, "")
    MIST_FIELD(LightComponent, shadowBias,      ::Mist::PropertyHint::Range, "0,0.1")
    MIST_FIELD(LightComponent, shadowNormalBias,::Mist::PropertyHint::Range, "0,5")
    MIST_FIELD(LightComponent, shadowBlur,      ::Mist::PropertyHint::Range, "0,5")
    MIST_FIELD(LightComponent, shadowOpacity,   ::Mist::PropertyHint::Range, "0,1")
    MIST_FIELD(LightComponent, temperatureK,    ::Mist::PropertyHint::Range, "0,15000")
MIST_REFLECT_END(LightComponent)

#endif // LIGHTCOMPONENT_H
