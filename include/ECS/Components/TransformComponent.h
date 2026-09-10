#ifndef TRANSFORMCOMPONENT_H
#define TRANSFORMCOMPONENT_H

#include "Core/Reflection.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct TransformComponent {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};

    // Cached world-space transform populated by HierarchySystem each frame.
    // Callers that want the composed-with-parents matrix read this instead
    // of rebuilding locally. `dirty` signals the cache needs recomputing —
    // flipped by HierarchySystem::Attach/Detach + by editor mutations.
    glm::mat4 cachedGlobal{1.0f};
    bool      dirty{true};

    // LOCAL transform — parent chain not applied. Only HierarchySystem and
    // code that genuinely wants parent-relative space should call this.
    // Everything that draws, lights or queries an entity in world space wants
    // WorldMatrix() below.
    glm::mat4 GetModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        return model;
    }

    // WORLD transform — the one the renderer, shadow passes, light sync and
    // gizmos must use.
    //
    // HierarchySystem only visits entities that carry a HierarchyComponent, so
    // `cachedGlobal` is authoritative exactly when it has been resolved this
    // frame (`!dirty`). Entities with no HierarchyComponent never get visited,
    // leave `dirty` true forever, and correctly fall through to their local
    // matrix — which for a parentless entity *is* its world matrix. That
    // fallback is what lets spawn paths (Lua spawn_cube, AssetRegistry,
    // SceneSerializer) keep working without being forced to attach a
    // HierarchyComponent.
    glm::mat4 WorldMatrix() const {
        return dirty ? GetModelMatrix() : cachedGlobal;
    }

    // World-space basis, extracted from WorldMatrix(). These replace four
    // copy-pasted "rotate XYZ then take -Z" blocks that previously lived in
    // LightSystem and three separate places in Renderer — and unlike those,
    // they follow the parent chain.
    //
    // Forward is -Z in local space, matching Godot's Light3D convention.
    glm::vec3 WorldPosition() const { return glm::vec3(WorldMatrix()[3]); }

    glm::vec3 WorldForward() const {
        const glm::mat4 m = WorldMatrix();
        return glm::normalize(glm::vec3(-m[2]));
    }

    glm::vec3 WorldUp() const {
        const glm::mat4 m = WorldMatrix();
        return glm::normalize(glm::vec3(m[1]));
    }

    glm::vec3 WorldRight() const {
        const glm::mat4 m = WorldMatrix();
        return glm::normalize(glm::vec3(m[0]));
    }
};

MIST_REFLECT(TransformComponent)
    MIST_FIELD(TransformComponent, position, ::Mist::PropertyHint::None, "")
    MIST_FIELD(TransformComponent, rotation, ::Mist::PropertyHint::None, "")
    MIST_FIELD(TransformComponent, scale,    ::Mist::PropertyHint::None, "")
MIST_REFLECT_END(TransformComponent)

#endif // TRANSFORMCOMPONENT_H
