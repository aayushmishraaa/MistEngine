#pragma once
#ifndef MIST_SERVICE_LOCATOR_H
#define MIST_SERVICE_LOCATOR_H

class Coordinator;
class UIManager;
class InputManager;
class FPSGameManager;
class PhysicsSystem;
class EnemyAISystem;
class Renderer;
class Scene;

class ServiceLocator {
public:
    static ServiceLocator& Instance() {
        static ServiceLocator instance;
        return instance;
    }

    void SetCoordinator(Coordinator* c)       { m_Coordinator = c; }
    void SetUIManager(UIManager* u)           { m_UIManager = u; }
    void SetInputManager(InputManager* i)     { m_InputManager = i; }
    void SetFPSGameManager(FPSGameManager* f) { m_FPSGameManager = f; }
    void SetPhysicsSystem(PhysicsSystem* p)   { m_PhysicsSystem = p; }
    void SetEnemySystem(EnemyAISystem* e)     { m_EnemySystem = e; }
    void SetRenderer(Renderer* r)             { m_Renderer = r; }
    void SetScene(Scene* s)                   { m_Scene = s; }

    Coordinator*    GetCoordinator()    const { return m_Coordinator; }
    UIManager*      GetUIManager()      const { return m_UIManager; }
    InputManager*   GetInputManager()   const { return m_InputManager; }
    FPSGameManager* GetFPSGameManager() const { return m_FPSGameManager; }
    PhysicsSystem*  GetPhysicsSystem()  const { return m_PhysicsSystem; }
    EnemyAISystem*  GetEnemySystem()    const { return m_EnemySystem; }
    Renderer*       GetRenderer()       const { return m_Renderer; }
    Scene*          GetScene()          const { return m_Scene; }

private:
    ServiceLocator() = default;
    Coordinator*    m_Coordinator    = nullptr;
    UIManager*      m_UIManager      = nullptr;
    InputManager*   m_InputManager   = nullptr;
    FPSGameManager* m_FPSGameManager = nullptr;
    PhysicsSystem*  m_PhysicsSystem  = nullptr;
    EnemyAISystem*  m_EnemySystem    = nullptr;
    Renderer*       m_Renderer       = nullptr;
    Scene*          m_Scene          = nullptr;
};

#endif // MIST_SERVICE_LOCATOR_H
