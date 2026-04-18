
#ifndef SCENE_H
#define SCENE_H

#include <memory>
#include <utility>
#include <vector>
#include <glm/glm.hpp>

#include "Renderable.h"
#include "Orb.h"
#include "PhysicsSystem.h" // Include PhysicsSystem and its types

// Non-owning handle to a physics body and its corresponding renderable.
// The `body` pointer is owned by PhysicsSystem; the `renderable` pointer is
// owned by this Scene (see Scene::~Scene). Mixing the two ownerships here
// previously caused a leak (body never freed) plus a latent double-free if
// ever cleaned up by both sides.
struct PhysicsRenderable {
    btRigidBody* body;      // non-owning; PhysicsSystem deletes
    Renderable* renderable; // owned by Scene
    glm::mat4 modelMatrix;
};


class Scene {
public:
    Scene() {}
    ~Scene(); // Will need to delete allocated objects

    void AddRenderable(Renderable* renderable) {
        renderables.push_back(renderable);
    }

    void AddOrb(Orb* orb) {
        orbs.push_back(orb);
    }

    void AddPhysicsRenderable(btRigidBody* body, Renderable* renderable) {
        physicsRenderables.push_back({body, renderable, glm::mat4(1.0f)});
    }


    const std::vector<Renderable*>& getRenderables() const {
        return renderables;
    }

    const std::vector<Orb*>& getOrbs() const {
        return orbs;
    }

    std::vector<PhysicsRenderable>& getPhysicsRenderables() {
        return physicsRenderables;
    }

    // Owning sink for Renderables that are referenced by ECS RenderComponents.
    // ECS components hold non-owning pointers; this keeps the Renderable alive
    // until the Scene is torn down. Returns the raw pointer for the component
    // to store.
    template <typename T, typename... Args>
    T* CreateOwnedRenderable(Args&&... args) {
        auto up = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = up.get();
        m_OwnedRenderables.emplace_back(std::move(up));
        return raw;
    }


private:
    std::vector<Renderable*> renderables; // For non-physics renderables
    std::vector<Orb*> orbs; // Orbs might have different rendering needs (glow)
    std::vector<PhysicsRenderable> physicsRenderables; // For physics-enabled renderables
    std::vector<std::unique_ptr<Renderable>> m_OwnedRenderables;
};

#endif // SCENE_H
