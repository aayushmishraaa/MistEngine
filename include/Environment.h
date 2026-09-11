#pragma once
#ifndef MIST_ENVIRONMENT_H
#define MIST_ENVIRONMENT_H

// How a scene looks — Godot's Environment resource.
//
// Problem this solves: render settings used to be public fields scattered
// across nine objects (PostProcessStack, Renderer, SkyboxRenderer, SSRRenderer,
// BloomRenderer, SSAORenderer, SSGIRenderer, ShadowSystem, TAARenderer). Every
// one was reachable from the View menu, every one reset to its compile-time
// default on restart, and none of them were serialized. A lighting look could
// not be saved, shared, or version-controlled, so a .mist file was not a
// complete description of anything — reopen a scene and you got the defaults.
//
// Settings also had no owner. Adding a post-process knob meant adding a public
// field to whichever renderer happened to hold the pass, then hand-writing a
// View-menu widget for it. There was no single place that meant "how this
// scene renders".
//
// Shape borrowed from Godot's Environment, with the same reflection trick
// PBRMaterial already proves: one MIST_REFLECT struct gets the Inspector panel
// and the serializer round-trip for free, so a new field costs one line here
// and nothing anywhere else.
//
// Deliberately NOT modelled yet (Godot has all three):
//   - the CameraAttributes split, which separates exposure and DOF out of
//     Environment and adds auto-exposure
//   - the priority chain, Camera3D > WorldEnvironment > editor preview
//   - fog and volumetric fog
// One serializable Environment first.
//
// This header is GL-free on purpose: the sub-renderers include it, but it must
// stay includable from the serializer and from headless tests.

#include "Core/Reflection.h"

#include <glm/vec3.hpp>

#include <cstdint>

// Matches Godot 4.3's flip to AgX as the default.
enum class TonemapOperator : std::uint8_t {
    ACES     = 0,
    Reinhard = 1,
    AgX      = 2,
};

struct Environment {
    // --- Tonemap and exposure ---
    TonemapOperator tonemap  = TonemapOperator::AgX;
    float exposure           = 1.0f;

    // --- Ambient fallback ---
    // Used where no IBL contribution applies. The sun direction and colour are
    // still derived per-frame from the first directional LightComponent when
    // one exists; sunDirection below is the fallback for a scene with no
    // directional light, which previously rendered against a hardcoded vector.
    glm::vec3 ambientColor     {0.03f, 0.03f, 0.03f};
    float     ambientIntensity = 1.0f;

    // --- Bloom ---
    bool  bloomEnabled   = true;
    float bloomThreshold = 1.0f;
    float bloomIntensity = 0.5f;

    // --- SSAO ---
    bool  ssaoEnabled = true;
    float ssaoRadius  = 0.5f;
    float ssaoBias    = 0.025f;

    // --- Screen-space reflections ---
    bool  ssrEnabled         = true;   // on by default: visible win on ground reflections
    float ssrRoughnessCutoff = 0.7f;   // pixels rougher than this skip SSR
    float ssrMaxDistance     = 100.0f; // view-space march distance cap

    // --- Screen-space global illumination ---
    bool  ssgiEnabled   = false;
    float ssgiRadius    = 3.0f;
    float ssgiIntensity = 1.0f;

    // --- Antialiasing ---
    bool taaEnabled  = false;  // off by default; user opts in
    bool fxaaEnabled = true;

    // --- Motion blur ---
    bool  motionBlurEnabled  = false;
    float motionBlurStrength = 0.5f;

    // --- Depth of field (bokeh) ---
    // Off by default because a good focus-target UX is future work; the focus
    // distance is still a raw number with no auto-exposure behind it.
    bool  dofEnabled       = false;
    float dofFocusDistance = 10.0f;  // view-space forward distance
    float dofAperture      = 0.15f;  // scales CoC; 0.15 ~ a cinematic f/2.8
    float dofMaxRadius     = 10.0f;  // max blur radius in pixels

    // --- Shadows ---
    // softness is Godot's "light angular size" surrogate: it scales the PCSS
    // penumbra search and the PCF kernel. 0 disables PCSS (hard shadows).
    // quality 0 = 4/16 taps, 1 = 8/32.
    float shadowSoftness      = 1.0f;
    int   shadowQuality       = 0;
    bool  showCascadeColors   = false;

    // --- Sky (atmospheric scattering) ---
    glm::vec3 sunDirection      {0.5f, 0.3f, 0.8f};
    float     skyRayleigh       = 1.0f;
    float     skyMie            = 0.005f;
    float     skyTurbidity      = 2.0f;

    // --- Renderer toggles ---
    // usePBR is a scene property; the two debug overlays are editor state that
    // rides along here so the whole View menu has exactly one backing object.
    bool usePBR           = true;
    bool showEditorGrid   = true;
    bool showPhysicsDebug = false;
};

MIST_REFLECT(Environment)
    MIST_FIELD(Environment, tonemap,            ::Mist::PropertyHint::Enum,  "ACES,Reinhard,AgX")
    MIST_FIELD(Environment, exposure,           ::Mist::PropertyHint::Range, "0.01,10")

    MIST_FIELD(Environment, ambientColor,       ::Mist::PropertyHint::Color, "")
    MIST_FIELD(Environment, ambientIntensity,   ::Mist::PropertyHint::Range, "0,4")

    MIST_FIELD(Environment, bloomEnabled,       ::Mist::PropertyHint::None,  "")
    MIST_FIELD(Environment, bloomThreshold,     ::Mist::PropertyHint::Range, "0,10")
    MIST_FIELD(Environment, bloomIntensity,     ::Mist::PropertyHint::Range, "0,2")

    MIST_FIELD(Environment, ssaoEnabled,        ::Mist::PropertyHint::None,  "")
    MIST_FIELD(Environment, ssaoRadius,         ::Mist::PropertyHint::Range, "0.01,2")
    MIST_FIELD(Environment, ssaoBias,           ::Mist::PropertyHint::Range, "0,0.2")

    MIST_FIELD(Environment, ssrEnabled,         ::Mist::PropertyHint::None,  "")
    MIST_FIELD(Environment, ssrRoughnessCutoff, ::Mist::PropertyHint::Range, "0,1")
    MIST_FIELD(Environment, ssrMaxDistance,     ::Mist::PropertyHint::Range, "1,500")

    MIST_FIELD(Environment, ssgiEnabled,        ::Mist::PropertyHint::None,  "")
    MIST_FIELD(Environment, ssgiRadius,         ::Mist::PropertyHint::Range, "0.1,10")
    MIST_FIELD(Environment, ssgiIntensity,      ::Mist::PropertyHint::Range, "0,4")

    MIST_FIELD(Environment, taaEnabled,         ::Mist::PropertyHint::None,  "")
    MIST_FIELD(Environment, fxaaEnabled,        ::Mist::PropertyHint::None,  "")

    MIST_FIELD(Environment, motionBlurEnabled,  ::Mist::PropertyHint::None,  "")
    MIST_FIELD(Environment, motionBlurStrength, ::Mist::PropertyHint::Range, "0,2")

    MIST_FIELD(Environment, dofEnabled,         ::Mist::PropertyHint::None,  "")
    MIST_FIELD(Environment, dofFocusDistance,   ::Mist::PropertyHint::Range, "0.1,200")
    MIST_FIELD(Environment, dofAperture,        ::Mist::PropertyHint::Range, "0,1")
    MIST_FIELD(Environment, dofMaxRadius,       ::Mist::PropertyHint::Range, "1,32")

    MIST_FIELD(Environment, shadowSoftness,     ::Mist::PropertyHint::Range, "0,4")
    MIST_FIELD(Environment, shadowQuality,      ::Mist::PropertyHint::Range, "0,1")
    MIST_FIELD(Environment, showCascadeColors,  ::Mist::PropertyHint::None,  "")

    MIST_FIELD(Environment, sunDirection,       ::Mist::PropertyHint::None,  "")
    MIST_FIELD(Environment, skyRayleigh,        ::Mist::PropertyHint::Range, "0,5")
    MIST_FIELD(Environment, skyMie,             ::Mist::PropertyHint::Range, "0,0.1")
    MIST_FIELD(Environment, skyTurbidity,       ::Mist::PropertyHint::Range, "1,10")

    MIST_FIELD(Environment, usePBR,             ::Mist::PropertyHint::None,  "")
    MIST_FIELD(Environment, showEditorGrid,     ::Mist::PropertyHint::None,  "")
    MIST_FIELD(Environment, showPhysicsDebug,   ::Mist::PropertyHint::None,  "")
MIST_REFLECT_END(Environment)

#endif // MIST_ENVIRONMENT_H
