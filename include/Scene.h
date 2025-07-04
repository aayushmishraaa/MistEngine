
#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <glm/glm.hpp>

#include "Renderable.h"
#include "Orb.h"
#include "PhysicsSystem.h" // Include PhysicsSystem and its types

// Define a struct to hold a physics body and its corresponding renderable
struct PhysicsRenderable {
    btRigidBody* body;
    Renderable* renderable;
    glm::mat4 modelMatrix; // To store the updated model matrix from physics
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


private:
    std::vector<Renderable*> renderables; // For non-physics renderables
    std::vector<Orb*> orbs; // Orbs might have different rendering needs (glow)
    std::vector<PhysicsRenderable> physicsRenderables; // For physics-enabled renderables
};

#endif // SCENE_H
