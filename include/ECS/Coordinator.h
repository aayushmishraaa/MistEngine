#ifndef COORDINATOR_H
#define COORDINATOR_H

#include <algorithm>
#include <memory>
#include "EntityManager.h"
#include "ComponentManager.h"
#include "SystemManager.h"
#include "Components/HierarchyComponent.h"
#include "Components/TransformComponent.h"

class Coordinator {
public:
    void Init() {
        m_EntityManager = std::make_unique<EntityManager>();
        m_ComponentManager = std::make_unique<ComponentManager>();
        m_SystemManager = std::make_unique<SystemManager>();
    }

    // Entity methods
    Entity CreateEntity() {
        return m_EntityManager->CreateEntity();
    }

    void DestroyEntity(Entity entity) {
        // Unlink from the scene graph BEFORE the components go away.
        //
        // Destroying a parented entity used to leave its id sitting in the
        // parent's `children` vector and leave its own children pointing at a
        // dead parent. The Hierarchy panel then rendered a phantom row — and
        // once the id was recycled by a later CreateEntity, that row silently
        // became a *different* entity nested under the wrong parent.
        //
        // This is the single choke point every destroy path funnels through
        // (editor delete, undo/redo, Lua destroy_entity, scene load's clear),
        // which is why the fix lives here rather than at each caller. The cost
        // is that the otherwise component-agnostic Coordinator has to know
        // about HierarchyComponent; the alternative was every call site
        // remembering to detach first, which is exactly what went wrong.
        UnlinkFromHierarchy(entity);

        m_EntityManager->DestroyEntity(entity);
        m_ComponentManager->EntityDestroyed(entity);
        m_SystemManager->EntityDestroyed(entity);
    }

    const std::unordered_set<Entity>& GetLivingEntities() const {
        return m_EntityManager->GetLivingEntities();
    }

    // Component methods
    template<typename T>
    void RegisterComponent() {
        m_ComponentManager->RegisterComponent<T>();
    }

    template<typename T>
    void AddComponent(Entity entity, T component) {
        m_ComponentManager->AddComponent<T>(entity, component);

        auto signature = m_EntityManager->GetSignature(entity);
        signature.set(m_ComponentManager->GetComponentType<T>(), true);
        m_EntityManager->SetSignature(entity, signature);

        m_SystemManager->EntitySignatureChanged(entity, signature);
    }

    template<typename T>
    void RemoveComponent(Entity entity) {
        m_ComponentManager->RemoveComponent<T>(entity);

        auto signature = m_EntityManager->GetSignature(entity);
        signature.set(m_ComponentManager->GetComponentType<T>(), false);
        m_EntityManager->SetSignature(entity, signature);

        m_SystemManager->EntitySignatureChanged(entity, signature);
    }

    template<typename T>
    T& GetComponent(Entity entity) {
        return m_ComponentManager->GetComponent<T>(entity);
    }

    template<typename T>
    bool HasComponent(Entity entity) {
        return m_ComponentManager->HasComponent<T>(entity);
    }

    template<typename T>
    ComponentType GetComponentType() {
        return m_ComponentManager->GetComponentType<T>();
    }

    // System methods
    template<typename T>
    std::shared_ptr<T> RegisterSystem() {
        return m_SystemManager->RegisterSystem<T>();
    }

    template<typename T>
    void SetSystemSignature(Signature signature) {
        m_SystemManager->SetSignature<T>(signature);
    }

private:
    // Detach `entity` from its parent and re-root its children, so no live
    // HierarchyComponent is left referencing a destroyed id. Deliberately does
    // NOT use HierarchySystem::Detach: HierarchySystem.h includes this header,
    // so calling into it would be a cycle. The logic is small enough to inline.
    void UnlinkFromHierarchy(Entity entity) {
        if (!HasComponent<HierarchyComponent>(entity)) return;
        auto& h = GetComponent<HierarchyComponent>(entity);

        // Remove ourselves from our parent's child list.
        if (h.parent != HierarchyComponent::kNoParent
            && HasComponent<HierarchyComponent>(h.parent)) {
            auto& siblings = GetComponent<HierarchyComponent>(h.parent).children;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), entity),
                           siblings.end());
        }

        // Re-root our children rather than orphaning them onto a dead id, and
        // mark them dirty so HierarchySystem recomputes them next frame.
        for (Entity child : h.children) {
            if (HasComponent<HierarchyComponent>(child)) {
                GetComponent<HierarchyComponent>(child).parent =
                    HierarchyComponent::kNoParent;
            }
            if (HasComponent<TransformComponent>(child)) {
                GetComponent<TransformComponent>(child).dirty = true;
            }
        }
        h.children.clear();
    }

    std::unique_ptr<EntityManager> m_EntityManager;
    std::unique_ptr<ComponentManager> m_ComponentManager;
    std::unique_ptr<SystemManager> m_SystemManager;
};

#endif // COORDINATOR_H
