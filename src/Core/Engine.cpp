#include "Core/Engine.h"
#include "Core/Logger.h"
#include "Core/ServiceLocator.h"
#include "Renderer.h"
#include "Scene.h"
#include "UIManager.h"
#include "InputManager.h"
#include "PhysicsSystem.h"
#include "FPSGameManager.h"
#include "ModuleManager.h"
#include "Version.h"

#include "ECS/Coordinator.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/ECSPhysicsSystem.h"

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

    // Create Renderer
    m_Renderer = std::make_unique<Renderer>(width, height);
    if (!m_Renderer->Init()) return false;

    // UI Manager
    m_UIManager = std::make_unique<UIManager>();
    if (!m_UIManager->Initialize(m_Renderer->GetWindow())) {
        LOG_ERROR("Failed to initialize UI Manager");
        return false;
    }

    // Input Manager
    m_InputManager = std::make_unique<InputManager>();
    m_InputManager->Initialize(m_Renderer->GetWindow());
    m_InputManager->SetCamera(&m_Renderer->GetCamera());
    m_InputManager->EnableSceneEditorMode(true);

    // Physics
    m_PhysicsSystem = std::make_unique<PhysicsSystem>();
    m_UIManager->SetPhysicsSystem(m_PhysicsSystem.get());

    // FPS Game Manager
    m_FPSGameManager = std::make_unique<FPSGameManager>();
    m_FPSGameManager->Initialize(m_InputManager.get(), &m_Renderer->GetCamera(),
                                  m_UIManager.get(), m_PhysicsSystem.get());

    // Module Manager
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
    m_UIManager->SetFPSGameManager(m_FPSGameManager.get());
    m_ModuleManager->SetScene(m_Scene.get());

    // Register with ServiceLocator
    auto& sl = ServiceLocator::Instance();
    sl.SetCoordinator(&gCoordinator);
    sl.SetUIManager(m_UIManager.get());
    sl.SetInputManager(m_InputManager.get());
    sl.SetFPSGameManager(m_FPSGameManager.get());
    sl.SetPhysicsSystem(m_PhysicsSystem.get());
    sl.SetRenderer(m_Renderer.get());
    sl.SetScene(m_Scene.get());

    if (m_FPSGameManager->m_enemySystem) {
        sl.SetEnemySystem(m_FPSGameManager->m_enemySystem.get());
    }

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
    if (m_FPSGameManager) m_FPSGameManager->Shutdown();
    if (m_UIManager) m_UIManager->Shutdown();
    if (m_ModuleManager) m_ModuleManager->UnloadAllModules();
    LOG_INFO("=== Shutdown Complete ===");
}

void Engine::ProcessGlobalInput() {
    auto* window = m_Renderer->GetWindow();

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (!m_FPSGameManager->IsGameActive()) {
            glfwSetWindowShouldClose(window, true);
        }
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

    // F2: Toggle AI window
    {
        static bool f2Prev = false;
        bool f2Cur = glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS;
        if (f2Cur && !f2Prev) {
            m_UIManager->SetShowAI(!m_UIManager->IsShowingAI());
        }
        f2Prev = f2Cur;
    }
}

void Engine::UpdateSystems(float deltaTime) {
    m_FPSGameManager->Update(deltaTime);
    m_InputManager->Update(deltaTime);

    if (!m_Scene->getPhysicsRenderables().empty() && !m_FPSGameManager->IsGameActive()) {
        // Legacy physics input handled by InputManager now
    }

    m_ModuleManager->UpdateModules(deltaTime);
    m_PhysicsSystem->Update(deltaTime);
    m_ECSPhysicsSystem->Update(deltaTime);
}
