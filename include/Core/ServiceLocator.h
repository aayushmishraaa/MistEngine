#pragma once
#ifndef MIST_SERVICE_LOCATOR_H
#define MIST_SERVICE_LOCATOR_H

class Coordinator;
class UIManager;
class InputManager;
class PhysicsSystem;
class Renderer;
class Scene;

// Global service registry. Previously also held FPSGameManager and
// EnemyAISystem pointers; those were removed alongside the FPS gameplay
// module. Callers that used them should depend on the editor + rendering
// services listed here instead.
class ServiceLocator {
public:
    static ServiceLocator& Instance() {
        static ServiceLocator instance;
        return instance;
    }

    void SetCoordinator(Coordinator* c)     { m_Coordinator = c; }
    void SetUIManager(UIManager* u)         { m_UIManager = u; }
    void SetInputManager(InputManager* i)   { m_InputManager = i; }
    void SetPhysicsSystem(PhysicsSystem* p) { m_PhysicsSystem = p; }
    void SetRenderer(Renderer* r)           { m_Renderer = r; }
    void SetScene(Scene* s)                 { m_Scene = s; }

    Coordinator*    GetCoordinator()    const { return m_Coordinator; }
    UIManager*      GetUIManager()      const { return m_UIManager; }
    InputManager*   GetInputManager()   const { return m_InputManager; }
    PhysicsSystem*  GetPhysicsSystem()  const { return m_PhysicsSystem; }
    Renderer*       GetRenderer()       const { return m_Renderer; }
    Scene*          GetScene()          const { return m_Scene; }

private:
    ServiceLocator() = default;
    Coordinator*   m_Coordinator   = nullptr;
    UIManager*     m_UIManager     = nullptr;
    InputManager*  m_InputManager  = nullptr;
    PhysicsSystem* m_PhysicsSystem = nullptr;
    Renderer*      m_Renderer      = nullptr;
    Scene*         m_Scene         = nullptr;
};

#endif // MIST_SERVICE_LOCATOR_H
