#pragma once
#ifndef MIST_ENGINE_CORE_H
#define MIST_ENGINE_CORE_H

#include <memory>
#include <string>

class Renderer;
class UIManager;
class InputManager;
class PhysicsSystem;
class ModuleManager;
class Scene;
class RenderSystem;
class ECSPhysicsSystem;

namespace ECS { class Coordinator; }

class Engine {
public:
    Engine();
    ~Engine();

    bool Initialize(unsigned int width, unsigned int height);
    void Run();
    void Shutdown();

    Renderer*       GetRenderer()       { return m_Renderer.get(); }
    UIManager*      GetUIManager()      { return m_UIManager.get(); }
    InputManager*   GetInputManager()   { return m_InputManager.get(); }
    Scene*          GetScene()          { return m_Scene.get(); }

private:
    std::unique_ptr<Renderer>       m_Renderer;
    std::unique_ptr<UIManager>      m_UIManager;
    std::unique_ptr<InputManager>   m_InputManager;
    std::unique_ptr<PhysicsSystem>  m_PhysicsSystem;
    std::unique_ptr<ModuleManager>  m_ModuleManager;
    std::unique_ptr<Scene>          m_Scene;

    std::shared_ptr<RenderSystem>      m_RenderSystem;
    std::shared_ptr<ECSPhysicsSystem>  m_ECSPhysicsSystem;

    bool m_Running = false;

    void ProcessGlobalInput();
    void UpdateSystems(float deltaTime);
};

#endif // MIST_ENGINE_CORE_H
