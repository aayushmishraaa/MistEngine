// Renderer.h includes glad before anything else pulls in GL — keep this first.
#include "Renderer.h"

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <sys/stat.h>
#include <vector>

#include "Core/PathGuard.h"
#include "InputManager.h"
#include "Mesh.h"
#include "ModuleManager.h"
#include "Orb.h"
#include "PhysicsSystem.h"
#include "Resources/AssetRegistry.h"
#include "Resources/Ref.h"
#include "Scene.h"
#include "ShapeGenerator.h"
#include "Texture.h"
#include "UIManager.h"
#include "Version.h"

#include <glm/gtc/type_ptr.hpp>

// ECS
#include "ECS/Components/HierarchyComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Coordinator.h"
#include "ECS/Systems/ECSPhysicsSystem.h"
#include "ECS/Systems/HierarchySystem.h"
#include "ECS/Systems/RenderSystem.h"

// Optional Lua scripting (G10 concrete).
#if MIST_ENABLE_SCRIPTING
#include "ECS/Components/ScriptComponent.h"
#include "ECS/Systems/ScriptSystem.h"
#include "Script/LuaScriptLanguage.h"
#include "Script/ScriptRegistry.h"
#include <fstream>
#include <sstream>
#endif

// gCoordinator is defined in src/ECS/Coordinator.cpp so MistEngineLib exports
// the symbol for tests and modules.
extern Coordinator gCoordinator;

// Global managers kept for the few callbacks that still need them.
UIManager*      g_uiManager      = nullptr;
InputManager*   g_inputManager   = nullptr;
ModuleManager*  g_moduleManager  = nullptr;
PhysicsSystem*  g_physicsSystem  = nullptr;

// Legacy physics debug keys — unchanged, not tied to any game mode. Left as a
// quick way to push around the demo cube with IJKL while we work on the
// renderer/editor direction.
void ProcessLegacyPhysicsInput(GLFWwindow* window, PhysicsSystem& physicsSystem,
                               std::vector<PhysicsRenderable>& physicsRenderables, float /*deltaTime*/) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse || io.WantCaptureKeyboard) return;

    if (!physicsRenderables.empty() && physicsRenderables.size() > 1) {
        auto cubeBody = physicsRenderables[1].body;
        float force = 100.0f;
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
            physicsSystem.ApplyForce(cubeBody, glm::vec3(0.0f, 0.0f, -force));
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
            physicsSystem.ApplyForce(cubeBody, glm::vec3(0.0f, 0.0f, force));
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
            physicsSystem.ApplyForce(cubeBody, glm::vec3(-force, 0.0f, 0.0f));
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
            physicsSystem.ApplyForce(cubeBody, glm::vec3(force, 0.0f, 0.0f));
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            physicsSystem.ApplyForce(cubeBody, glm::vec3(0.0f, force * 2.0f, 0.0f));
    }
}

static bool DirectoryExists(const std::string& path) {
#ifdef _WIN32
    DWORD attributes = GetFileAttributesA(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat statbuf;
    return (stat(path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
#endif
}

int main() {
    const unsigned int SCR_WIDTH = 1200;
    const unsigned int SCR_HEIGHT = 800;

    std::cout << "=== " << MIST_ENGINE_NAME << " " << MIST_ENGINE_VERSION_STRING
              << " — editor + rendering sandbox ===" << std::endl;
    std::cout << "Built with " << MIST_ENGINE_COMPILER << " on " << MIST_ENGINE_PLATFORM << std::endl;
    std::cout << "Build Type: " << MIST_ENGINE_BUILD_TYPE << std::endl;
    std::cout << "Features: ";
#if MIST_ENGINE_HAS_AI_INTEGRATION
    std::cout << "AI ";
#endif
#if MIST_ENGINE_HAS_PHYSICS
    std::cout << "Physics ";
#endif
#if MIST_ENGINE_HAS_OPENGL
    std::cout << "OpenGL ";
#endif
#if MIST_ENGINE_HAS_IMGUI
    std::cout << "ImGui ";
#endif
    std::cout << std::endl;

    // Establish the project root once at startup so every `res://` lookup
    // resolves against the same base regardless of CWD churn later on.
    Mist::PathGuard::set_project_root(std::filesystem::current_path());
    Mist::Assets::AssetRegistry::Instance().RegisterDefaultLoaders();

    gCoordinator.Init();
    gCoordinator.RegisterComponent<TransformComponent>();
    gCoordinator.RegisterComponent<RenderComponent>();
    gCoordinator.RegisterComponent<PhysicsComponent>();
    gCoordinator.RegisterComponent<HierarchyComponent>();
#if MIST_ENABLE_SCRIPTING
    gCoordinator.RegisterComponent<ScriptComponent>();
#endif

    auto renderSystem     = gCoordinator.RegisterSystem<RenderSystem>();
    auto ecsPhysicsSystem = gCoordinator.RegisterSystem<ECSPhysicsSystem>();
    auto hierarchySystem  = gCoordinator.RegisterSystem<HierarchySystem>();
#if MIST_ENABLE_SCRIPTING
    auto scriptSystem     = gCoordinator.RegisterSystem<ScriptSystem>();
#endif

    Signature renderSignature;
    renderSignature.set(gCoordinator.GetComponentType<TransformComponent>());
    renderSignature.set(gCoordinator.GetComponentType<RenderComponent>());
    gCoordinator.SetSystemSignature<RenderSystem>(renderSignature);

    Signature physicsSignature;
    physicsSignature.set(gCoordinator.GetComponentType<TransformComponent>());
    physicsSignature.set(gCoordinator.GetComponentType<PhysicsComponent>());
    gCoordinator.SetSystemSignature<ECSPhysicsSystem>(physicsSignature);

    Signature hierarchySignature;
    hierarchySignature.set(gCoordinator.GetComponentType<TransformComponent>());
    hierarchySignature.set(gCoordinator.GetComponentType<HierarchyComponent>());
    gCoordinator.SetSystemSignature<HierarchySystem>(hierarchySignature);

#if MIST_ENABLE_SCRIPTING
    Signature scriptSignature;
    scriptSignature.set(gCoordinator.GetComponentType<TransformComponent>());
    scriptSignature.set(gCoordinator.GetComponentType<ScriptComponent>());
    gCoordinator.SetSystemSignature<ScriptSystem>(scriptSignature);

    // Register the Lua backend with the engine-wide ScriptRegistry.
    // Compile happens on demand when a ScriptComponent's path resolves.
    auto lua = std::make_shared<Mist::Script::LuaScriptLanguage>();
    Mist::Script::ScriptRegistry::Instance().Register(lua);

    scriptSystem->WireReadyCallback(gCoordinator);
#endif

    Renderer renderer(SCR_WIDTH, SCR_HEIGHT);
    if (!renderer.Init()) return -1;

    UIManager uiManager;
    g_uiManager = &uiManager;
    if (!uiManager.Initialize(renderer.GetWindow())) {
        std::cerr << "Failed to initialize UI Manager" << std::endl;
        return -1;
    }

    InputManager inputManager;
    g_inputManager = &inputManager;
    inputManager.Initialize(renderer.GetWindow());
    inputManager.SetCamera(&renderer.GetCamera());
    inputManager.EnableSceneEditorMode(true);
    std::cout << "Input Manager initialized successfully" << std::endl;

    PhysicsSystem physicsSystem;
    g_physicsSystem = &physicsSystem;
    uiManager.SetPhysicsSystem(&physicsSystem);

    ModuleManager moduleManager;
    g_moduleManager = &moduleManager;
    moduleManager.SetCoordinator(&gCoordinator);
    moduleManager.SetRenderer(&renderer);
    if (DirectoryExists("modules")) {
        std::cout << "Loading modules from 'modules' directory..." << std::endl;
        moduleManager.LoadModulesFromDirectory("modules");
    } else {
        std::cout << "No 'modules' directory found - continuing without external modules" << std::endl;
    }

    Scene scene;
    uiManager.SetScene(&scene);
    uiManager.SetCoordinator(&gCoordinator);
    uiManager.SetRenderer(&renderer);
    moduleManager.SetScene(&scene);

    // Default editor scene — authored in Lua. bootstrap.lua calls
    // spawn_plane / run_script('res://scripts/orbits.lua') which in
    // turn spawns a ring of cubes and attaches spinner.lua to each.
    // Changing the opening scene now means editing a .lua file, not
    // recompiling the engine. If scripting is disabled we fall back to
    // the minimal hardcoded ground+cube so the editor still has
    // something to render.
#if MIST_ENABLE_SCRIPTING
    {
        auto lang = Mist::Script::ScriptRegistry::Instance().Get(".lua");
        if (lang) {
            auto path = Mist::PathGuard::resolve_res_path("res://scripts/bootstrap.lua");
            std::ifstream in(path);
            if (in.is_open()) {
                std::stringstream ss; ss << in.rdbuf();
                auto inst = lang->Compile(ss.str());
                if (!inst) {
                    std::cerr << "[warn] bootstrap.lua failed to compile" << std::endl;
                }
                // Top-level already ran during Compile(); inst drops here.
            } else {
                std::cerr << "[warn] bootstrap.lua not found at " << path << std::endl;
            }
        }
    }
#else
    // No-scripting fallback — ground + cube so the viewport isn't empty.
    {
        auto& meshes   = Mist::Assets::AssetRegistry::Instance().meshes();
        auto planeMesh = LoadRef(meshes, "builtin://plane");
        auto cubeMesh  = LoadRef(meshes, "builtin://cube");

        Entity ground = gCoordinator.CreateEntity();
        TransformComponent gt;
        gt.position = {0.0f, -0.01f, 0.0f};
        gt.scale    = {20.0f, 1.0f, 20.0f};
        gCoordinator.AddComponent(ground, gt);
        RenderComponent gr;
        gr.renderable = planeMesh.get();
        gr.visible    = true;
        gCoordinator.AddComponent(ground, gr);
        uiManager.SetEntityName(ground, "Ground");

        Entity cube = gCoordinator.CreateEntity();
        TransformComponent ct;
        ct.position = {0.0f, 0.5f, 0.0f};
        gCoordinator.AddComponent(cube, ct);
        RenderComponent cr;
        cr.renderable = cubeMesh.get();
        cr.visible    = true;
        gCoordinator.AddComponent(cube, cr);
        uiManager.SetEntityName(cube, "Default Cube");
    }
#endif

    renderer.GetCamera().Position = glm::vec3(5.0f, 3.0f, 5.0f);
    renderer.GetCamera().Yaw      = -135.0f;
    renderer.GetCamera().Pitch    = -20.0f;
    renderer.GetCamera().updateCameraVectors();
    renderer.GetCamera().SetOrbitMode(true);

    std::cout << "=== Engine Initialization Complete ===" << std::endl;
    std::cout << "Editor ready. F1=Demo  F2=AI panel  F3=Scene editor  F=focus on selection" << std::endl;

    // Fixed-timestep physics. Decouples deterministic physics from the
    // variable-rate render frame: at 144 Hz display we still run physics at
    // 60 Hz, at 30 Hz display we catch up by stepping twice per frame.
    // Clamped at 0.25s (15 steps) to prevent the classic "spiral of death"
    // on a slow frame.
    constexpr float kPhysicsStep       = 1.0f / 60.0f;
    constexpr float kMaxFrameDelta     = 0.25f;
    float           physicsAccumulator = 0.0f;

    while (!glfwWindowShouldClose(renderer.GetWindow())) {
        float deltaTime = renderer.GetDeltaTime();

        if (glfwGetKey(renderer.GetWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(renderer.GetWindow(), true);
        }

        {
            static bool f1Pressed = false;
            bool f1Cur = glfwGetKey(renderer.GetWindow(), GLFW_KEY_F1) == GLFW_PRESS;
            if (f1Cur && !f1Pressed && g_uiManager) {
                g_uiManager->SetShowDemo(!g_uiManager->IsShowingDemo());
            }
            f1Pressed = f1Cur;
        }

        inputManager.Update(deltaTime);

        // F key — focus camera on selected entity (Godot-style).
        {
            static bool fWasPressed = false;
            bool fIsPressed = glfwGetKey(renderer.GetWindow(), GLFW_KEY_F) == GLFW_PRESS;
            if (fIsPressed && !fWasPressed && uiManager.HasSelectedEntity()) {
                ImGuiIO& io = ImGui::GetIO();
                if (!io.WantCaptureKeyboard) {
                    auto& t = gCoordinator.GetComponent<TransformComponent>(uiManager.GetSelectedEntity());
                    renderer.GetCamera().FocusOn(t.position);
                }
            }
            fWasPressed = fIsPressed;
        }

        if (!scene.getPhysicsRenderables().empty()) {
            ProcessLegacyPhysicsInput(renderer.GetWindow(), physicsSystem, scene.getPhysicsRenderables(), deltaTime);
        }

        moduleManager.UpdateModules(deltaTime);

        // Advance the physics clock in fixed-step chunks — see the
        // accumulator declared above for the reasoning. The render frame
        // itself still uses `deltaTime` for camera smoothing etc; only
        // physics is locked to 60 Hz.
        float frameDelta = std::min(deltaTime, kMaxFrameDelta);
        physicsAccumulator += frameDelta;
        while (physicsAccumulator >= kPhysicsStep) {
            physicsSystem.Update(kPhysicsStep);
            ecsPhysicsSystem->Update(kPhysicsStep);
            physicsAccumulator -= kPhysicsStep;
        }

        // Resolve parent→child transform chains into cachedGlobal, then
        // fire any pending OnReady callbacks, before rendering picks them up.
        hierarchySystem->UpdateTransforms(gCoordinator);
        hierarchySystem->FireReadyCallbacks(gCoordinator);

#if MIST_ENABLE_SCRIPTING
        // _process runs after _ready-via-OnReady so first-frame scripts
        // see a live transform. deltaTime is already clamped above.
        scriptSystem->Update(gCoordinator, deltaTime);
#endif

        renderer.RenderWithECSAndUI(scene, renderSystem, &uiManager);
    }

    std::cout << "=== MistEngine Shutting Down ===" << std::endl;
    uiManager.Shutdown();
    moduleManager.UnloadAllModules();
    std::cout << "=== Shutdown Complete ===" << std::endl;
    return 0;
}
