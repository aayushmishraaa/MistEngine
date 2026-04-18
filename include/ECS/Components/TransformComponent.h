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

    glm::mat4 GetModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        return model;
    }
};

MIST_REFLECT(TransformComponent)
    MIST_FIELD(TransformComponent, position, ::Mist::PropertyHint::None, "")
    MIST_FIELD(TransformComponent, rotation, ::Mist::PropertyHint::None, "")
    MIST_FIELD(TransformComponent, scale,    ::Mist::PropertyHint::None, "")
MIST_REFLECT_END(TransformComponent)

#endif // TRANSFORMCOMPONENT_H
