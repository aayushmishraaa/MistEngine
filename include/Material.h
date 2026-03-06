#pragma once
#ifndef MIST_MATERIAL_H
#define MIST_MATERIAL_H

#include <glm/glm.hpp>
#include <memory>
#include "Shader.h"
#include "Texture.h"

struct PBRMaterial {
    // Texture maps (nullptr = use scalar fallback)
    std::shared_ptr<Texture> albedoMap;
    std::shared_ptr<Texture> normalMap;
    std::shared_ptr<Texture> metallicMap;
    std::shared_ptr<Texture> roughnessMap;
    std::shared_ptr<Texture> aoMap;
    std::shared_ptr<Texture> emissiveMap;

    // Scalar fallbacks
    glm::vec3 albedo       = glm::vec3(0.8f);
    float     metallic     = 0.0f;
    float     roughness    = 0.5f;
    float     ao           = 1.0f;
    glm::vec3 emissive     = glm::vec3(0.0f);

    void Bind(Shader& shader, int startUnit = 1) const {
        // Unit 0 is reserved for shadow map

        // Albedo (unit startUnit + 0)
        shader.setBool("material.hasAlbedoMap", albedoMap != nullptr);
        if (albedoMap) {
            albedoMap->Bind(startUnit + 0);
            shader.setInt("material.albedoMap", startUnit + 0);
        }
        shader.setVec3("material.albedoColor", albedo);

        // Normal (unit startUnit + 1)
        shader.setBool("material.hasNormalMap", normalMap != nullptr);
        if (normalMap) {
            normalMap->Bind(startUnit + 1);
            shader.setInt("material.normalMap", startUnit + 1);
        }

        // Metallic (unit startUnit + 2)
        shader.setBool("material.hasMetallicMap", metallicMap != nullptr);
        if (metallicMap) {
            metallicMap->Bind(startUnit + 2);
            shader.setInt("material.metallicMap", startUnit + 2);
        }
        shader.setFloat("material.metallicValue", metallic);

        // Roughness (unit startUnit + 3)
        shader.setBool("material.hasRoughnessMap", roughnessMap != nullptr);
        if (roughnessMap) {
            roughnessMap->Bind(startUnit + 3);
            shader.setInt("material.roughnessMap", startUnit + 3);
        }
        shader.setFloat("material.roughnessValue", roughness);

        // AO (unit startUnit + 4)
        shader.setBool("material.hasAOMap", aoMap != nullptr);
        if (aoMap) {
            aoMap->Bind(startUnit + 4);
            shader.setInt("material.aoMap", startUnit + 4);
        }
        shader.setFloat("material.aoValue", ao);

        // Emissive (unit startUnit + 5)
        shader.setBool("material.hasEmissiveMap", emissiveMap != nullptr);
        if (emissiveMap) {
            emissiveMap->Bind(startUnit + 5);
            shader.setInt("material.emissiveMap", startUnit + 5);
        }
        shader.setVec3("material.emissiveColor", emissive);
    }
};

#endif // MIST_MATERIAL_H
