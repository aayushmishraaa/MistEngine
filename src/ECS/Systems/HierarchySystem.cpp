#include "ECS/Systems/HierarchySystem.h"

#include "ECS/Components/HierarchyComponent.h"
#include "ECS/Components/TransformComponent.h"

#include <glm/gtc/matrix_transform.hpp>
#include <stack>
#include <vector>

Mist::Signal<Entity>& HierarchySystem::OnReady() {
    static Mist::Signal<Entity> sig;
    return sig;
}

namespace {

// Returns the HierarchyComponent pointer or nullptr if absent. Using
// GetComponent directly would throw, so we route through HasComponent.
// The indirection is small and keeps the recursion readable.
HierarchyComponent* TryGetHier(Coordinator& c, Entity e) {
    if (!c.HasComponent<HierarchyComponent>(e)) return nullptr;
    return &c.GetComponent<HierarchyComponent>(e);
}

TransformComponent* TryGetTrans(Coordinator& c, Entity e) {
    if (!c.HasComponent<TransformComponent>(e)) return nullptr;
    return &c.GetComponent<TransformComponent>(e);
}

// Recursive compose: if dirty, multiply parent's cachedGlobal with our
// local. If not dirty but parent changed, we still have to recompute, so
// callers should propagate dirty down before invoking this. Returns the
// cachedGlobal that was written, mostly so the caller can feed it into
// children as their parent matrix.
void RecomputeSubtree(Coordinator& c, Entity e, const glm::mat4& parentGlobal, bool parentDirty) {
    auto* t = TryGetTrans(c, e);
    if (!t) return;

    const bool mustRecompute = t->dirty || parentDirty;
    if (mustRecompute) {
        t->cachedGlobal = parentGlobal * t->GetModelMatrix();
        t->dirty        = false;
    }

    auto* h = TryGetHier(c, e);
    if (!h) return;
    for (Entity child : h->children) {
        RecomputeSubtree(c, child, t->cachedGlobal, mustRecompute);
    }
}

// Post-order traversal so a parent's OnReady fires after all children. We
// flag entities whose readyFired is already true so listeners see each
// entity exactly once across the engine's lifetime.
void ReadyPostOrder(Coordinator& c, Entity e, Mist::Signal<Entity>& sig) {
    auto* h = TryGetHier(c, e);
    if (h) {
        for (Entity child : h->children) {
            ReadyPostOrder(c, child, sig);
        }
    }
    if (h && !h->readyFired) {
        h->readyFired = true;
        sig.Emit(e);
    }
}

} // namespace

void HierarchySystem::UpdateTransforms(Coordinator& coord) {
    // Walk every entity with a HierarchyComponent. Skip those with a
    // parent — we only want roots as entry points. RecomputeSubtree then
    // recurses through children and propagates the parent matrix.
    for (Entity e : m_Entities) {
        auto* h = TryGetHier(coord, e);
        if (!h) continue;
        if (h->parent != HierarchyComponent::kNoParent) continue;
        RecomputeSubtree(coord, e, glm::mat4(1.0f), false);
    }
}

void HierarchySystem::FireReadyCallbacks(Coordinator& coord) {
    auto& sig = OnReady();
    for (Entity e : m_Entities) {
        auto* h = TryGetHier(coord, e);
        if (!h) continue;
        if (h->parent != HierarchyComponent::kNoParent) continue;
        ReadyPostOrder(coord, e, sig);
    }
}

bool HierarchySystem::Attach(Coordinator& coord, Entity parent, Entity child) {
    auto* ph = TryGetHier(coord, parent);
    auto* ch = TryGetHier(coord, child);
    if (!ph || !ch) return false;

    // Detach from previous parent if any, to keep invariants consistent.
    if (ch->parent != HierarchyComponent::kNoParent) {
        if (auto* old = TryGetHier(coord, ch->parent)) {
            auto& vec = old->children;
            vec.erase(std::remove(vec.begin(), vec.end(), child), vec.end());
        }
    }

    ch->parent = parent;
    ph->children.push_back(child);

    if (auto* t = TryGetTrans(coord, child)) t->dirty = true;
    return true;
}

bool HierarchySystem::Detach(Coordinator& coord, Entity child) {
    auto* ch = TryGetHier(coord, child);
    if (!ch) return false;
    if (ch->parent == HierarchyComponent::kNoParent) return true; // already detached

    if (auto* ph = TryGetHier(coord, ch->parent)) {
        auto& vec = ph->children;
        vec.erase(std::remove(vec.begin(), vec.end(), child), vec.end());
    }
    ch->parent = HierarchyComponent::kNoParent;
    if (auto* t = TryGetTrans(coord, child)) t->dirty = true;
    return true;
}
