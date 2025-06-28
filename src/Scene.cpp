
#include "Scene.h"
#include "Renderable.h"
#include "Orb.h"

Scene::~Scene() {
    // Delete all non-physics renderable objects
    for (Renderable* renderable : renderables) {
        delete renderable;
    }
    renderables.clear();

    // Delete all orb objects
    for (Orb* orb : orbs) {
        delete orb;
    }
    orbs.clear();

    // Delete renderables associated with physics objects
    for (auto& physicsRenderable : physicsRenderables) {
        delete physicsRenderable.renderable;
    }
    physicsRenderables.clear();
}
