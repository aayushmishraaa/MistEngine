#pragma once
#ifndef MIST_MATERIAL_H
#define MIST_MATERIAL_H

#include "Core/Reflection.h"
#include "Shader.h"
#include "Texture.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>

// PBRMaterial is authored in two halves:
//   * Reflected "authoring" fields — scalars + texture paths. These are
//     what the Inspector edits, what .mistmat JSON round-trips, and
//     what the SceneSerializer captures per RenderComponent override.
//   * Runtime texture handles (shared_ptr<Texture>) — lazily populated
//     from the paths on the first Bind call. The importer can also
//     inject resolved textures directly without touching paths, for
//     cases where the texture came from an Assimp scene rather than a
//     standalone file.
//
// When the Inspector writes to a *Tex string field, call Refresh()
// so the runtime handle catches up. Otherwise the next Bind keeps
// showing the old texture.
struct PBRMaterial {
    // --- Authoring fields (reflected + serialised) ---
    glm::vec3 albedo            = glm::vec3(1.0f);
    float     metallic          = 0.0f;
    float     roughness         = 0.5f;
    float     ao                = 1.0f;
    glm::vec3 emissive          = glm::vec3(0.0f);
    float     emissiveIntensity = 1.0f;

    std::string albedoTex;     // absolute or res:// path
    std::string normalTex;
    std::string metallicTex;
    std::string roughnessTex;
    std::string aoTex;
    std::string emissiveTex;

    // --- Runtime texture handles (resolved from paths on demand) ---
    std::shared_ptr<Texture> albedoMap;
    std::shared_ptr<Texture> normalMap;
    std::shared_ptr<Texture> metallicMap;
    std::shared_ptr<Texture> roughnessMap;
    std::shared_ptr<Texture> aoMap;
    std::shared_ptr<Texture> emissiveMap;

    // Walks the six *Tex path fields and loads each into the matching
    // runtime handle. Idempotent — if the path/handle pair is already
    // in sync we skip. sRGB decoding follows the Godot convention:
    // albedo + emissive are sRGB, everything else is linear.
    void Refresh() {
        auto pull = [](const std::string& path, bool sRGB,
                       std::shared_ptr<Texture>& out) {
            if (path.empty()) { out.reset(); return; }
            if (out && out->path == path) return;
            auto tex = std::make_shared<Texture>();
            if (tex->LoadFromFile(path, sRGB)) out = tex;
        };
        pull(albedoTex,    true,  albedoMap);
        pull(normalTex,    false, normalMap);
        pull(metallicTex,  false, metallicMap);
        pull(roughnessTex, false, roughnessMap);
        pull(aoTex,        false, aoMap);
        pull(emissiveTex,  true,  emissiveMap);
    }

    void Bind(Shader& shader, int startUnit = 1) const {
        // Unit 0 reserved for shadow map.

        shader.setBool("material.hasAlbedoMap", albedoMap != nullptr);
        if (albedoMap) {
            albedoMap->Bind(startUnit + 0);
            shader.setInt("material.albedoMap", startUnit + 0);
        }
        shader.setVec3("material.albedoColor", albedo);

        shader.setBool("material.hasNormalMap", normalMap != nullptr);
        if (normalMap) {
            normalMap->Bind(startUnit + 1);
            shader.setInt("material.normalMap", startUnit + 1);
        }

        shader.setBool("material.hasMetallicMap", metallicMap != nullptr);
        if (metallicMap) {
            metallicMap->Bind(startUnit + 2);
            shader.setInt("material.metallicMap", startUnit + 2);
        }
        shader.setFloat("material.metallicValue", metallic);

        shader.setBool("material.hasRoughnessMap", roughnessMap != nullptr);
        if (roughnessMap) {
            roughnessMap->Bind(startUnit + 3);
            shader.setInt("material.roughnessMap", startUnit + 3);
        }
        shader.setFloat("material.roughnessValue", roughness);

        shader.setBool("material.hasAOMap", aoMap != nullptr);
        if (aoMap) {
            aoMap->Bind(startUnit + 4);
            shader.setInt("material.aoMap", startUnit + 4);
        }
        shader.setFloat("material.aoValue", ao);

        shader.setBool("material.hasEmissiveMap", emissiveMap != nullptr);
        if (emissiveMap) {
            emissiveMap->Bind(startUnit + 5);
            shader.setInt("material.emissiveMap", startUnit + 5);
        }
        shader.setVec3("material.emissiveColor", emissive * emissiveIntensity);
    }
};

// PBRMaterial is reflected as a top-level asset, not an ECS component.
// Same macro — the Inspector still auto-generates its panel, and the
// Material editor reuses DrawReflectedProperties.
MIST_REFLECT(PBRMaterial)
    MIST_FIELD(PBRMaterial, albedo,            ::Mist::PropertyHint::Color,       "")
    MIST_FIELD(PBRMaterial, metallic,          ::Mist::PropertyHint::Range,       "0,1")
    MIST_FIELD(PBRMaterial, roughness,         ::Mist::PropertyHint::Range,       "0,1")
    MIST_FIELD(PBRMaterial, ao,                ::Mist::PropertyHint::Range,       "0,1")
    MIST_FIELD(PBRMaterial, emissive,          ::Mist::PropertyHint::Color,       "")
    MIST_FIELD(PBRMaterial, emissiveIntensity, ::Mist::PropertyHint::Range,       "0,20")
    MIST_FIELD(PBRMaterial, albedoTex,         ::Mist::PropertyHint::ResourceRef, "Texture")
    MIST_FIELD(PBRMaterial, normalTex,         ::Mist::PropertyHint::ResourceRef, "Texture")
    MIST_FIELD(PBRMaterial, metallicTex,       ::Mist::PropertyHint::ResourceRef, "Texture")
    MIST_FIELD(PBRMaterial, roughnessTex,      ::Mist::PropertyHint::ResourceRef, "Texture")
    MIST_FIELD(PBRMaterial, aoTex,             ::Mist::PropertyHint::ResourceRef, "Texture")
    MIST_FIELD(PBRMaterial, emissiveTex,       ::Mist::PropertyHint::ResourceRef, "Texture")
MIST_REFLECT_END(PBRMaterial)

#endif // MIST_MATERIAL_H
