// Renderer.h brings glad in first. Any header that pulls in GLFW/gl.h before
// glad (e.g. InputManager.h via GLFW) will otherwise trip glad's
// "OpenGL header already included" guard. Keep Renderer.h at the top of the
// include list in every TU that touches GL-adjacent code.
#include "Renderer.h"

#include "Core/Engine.h"
#include "Core/Logger.h"
#include "Core/ServiceLocator.h"
#include "InputManager.h"
#include "ModuleManager.h"
#include "PhysicsSystem.h"
#include "Scene.h"
#include "UIManager.h"
#include "Version.h"

#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Coordinator.h"
#include "ECS/Systems/ECSPhysicsSystem.h"
#include "ECS/Systems/RenderSystem.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <sys/stat.h>

extern Coordinator gCoordinator;

static bool DirectoryExists(const std::string& path) {
    struct stat statbuf;
    return (stat(path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
}

Engine::Engine() = default;
Engine::~Engine() { Shutdown(); }

bool Engine::Initialize(unsigned int width, unsigned int height) {
    LOG_INFO("=== ", MIST_ENGINE_NAME, " ", MIST_ENGINE_VERSION_STRING, " Starting Up ===");
    LOG_INFO("Built with ", MIST_ENGINE_COMPILER, " on ", MIST_ENGINE_PLATFORM);

    // Initialize ECS
    gCoordinator.Init();
    gCoordinator.RegisterComponent<TransformComponent>();
    gCoordinator.RegisterComponent<RenderComponent>();
    gCoordinator.RegisterComponent<PhysicsComponent>();

    m_RenderSystem = gCoordinator.RegisterSystem<RenderSystem>();
    m_ECSPhysicsSystem = gCoordinator.RegisterSystem<ECSPhysicsSystem>();

    Signature renderSig;
    renderSig.set(gCoordinator.GetComponentType<TransformComponent>());
    renderSig.set(gCoordinator.GetComponentType<RenderComponent>());
    gCoordinator.SetSystemSignature<RenderSystem>(renderSig);

    Signature physicsSig;
    physicsSig.set(gCoordinator.GetComponentType<TransformComponent>());
    physicsSig.set(gCoordinator.GetComponentType<PhysicsComponent>());
    gCoordinator.SetSystemSignature<ECSPhysicsSystem>(physicsSig);

    // Renderer
    m_Renderer = std::make_unique<Renderer>(width, height);
    if (!m_Renderer->Init()) return false;

    // UI
    m_UIManager = std::make_unique<UIManager>();
    if (!m_UIManager->Initialize(m_Renderer->GetWindow())) {
        LOG_ERROR("Failed to initialize UI Manager");
        return false;
    }

    // Input
    m_InputManager = std::make_unique<InputManager>();
    m_InputManager->Initialize(m_Renderer->GetWindow());
    m_InputManager->SetCamera(&m_Renderer->GetCamera());
    m_InputManager->EnableSceneEditorMode(true);

    // Physics
    m_PhysicsSystem = std::make_unique<PhysicsSystem>();
    m_UIManager->SetPhysicsSystem(m_PhysicsSystem.get());

    // Modules
    m_ModuleManager = std::make_unique<ModuleManager>();
    m_ModuleManager->SetCoordinator(&gCoordinator);
    m_ModuleManager->SetRenderer(m_Renderer.get());
    if (DirectoryExists("modules")) {
        m_ModuleManager->LoadModulesFromDirectory("modules");
    }

    // Scene
    m_Scene = std::make_unique<Scene>();
    m_UIManager->SetScene(m_Scene.get());
    m_UIManager->SetCoordinator(&gCoordinator);
    m_UIManager->SetRenderer(m_Renderer.get());
    m_ModuleManager->SetScene(m_Scene.get());

    auto& sl = ServiceLocator::Instance();
    sl.SetCoordinator(&gCoordinator);
    sl.SetUIManager(m_UIManager.get());
    sl.SetInputManager(m_InputManager.get());
    sl.SetPhysicsSystem(m_PhysicsSystem.get());
    sl.SetRenderer(m_Renderer.get());
    sl.SetScene(m_Scene.get());

    m_Running = true;
    LOG_INFO("=== Engine Initialization Complete ===");
    return true;
}

void Engine::Run() {
    while (!glfwWindowShouldClose(m_Renderer->GetWindow()) && m_Running) {
        float deltaTime = m_Renderer->GetDeltaTime();
        ProcessGlobalInput();
        UpdateSystems(deltaTime);
        m_Renderer->RenderWithECSAndUI(*m_Scene, m_RenderSystem, m_UIManager.get());
    }
}

void Engine::Shutdown() {
    if (!m_Running) return;
    m_Running = false;
    LOG_INFO("=== MistEngine Shutting Down ===");
    if (m_UIManager) m_UIManager->Shutdown();
    if (m_ModuleManager) m_ModuleManager->UnloadAllModules();
    LOG_INFO("=== Shutdown Complete ===");
}

void Engine::ProcessGlobalInput() {
    auto* window = m_Renderer->GetWindow();

    // Esc closes the editor outright now that there's no "in-game" state to
    // swallow the keypress.
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // F1: Toggle demo window
    {
        static bool f1Prev = false;
        bool f1Cur = glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS;
        if (f1Cur && !f1Prev) {
            m_UIManager->SetShowDemo(!m_UIManager->IsShowingDemo());
        }
        f1Prev = f1Cur;
    }

}

void Engine::UpdateSystems(float deltaTime) {
    m_InputManager->Update(deltaTime);
    m_ModuleManager->UpdateModules(deltaTime);
    m_PhysicsSystem->Update(deltaTime);
    m_ECSPhysicsSystem->Update(deltaTime);
}
