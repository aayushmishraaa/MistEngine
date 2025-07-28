#include "InputManager.h"
#include "Camera.h"
#include "imgui.h"
#include <iostream>

// Global pointer for GLFW callbacks (static to avoid symbol collision)
static InputManager* g_inputManager = nullptr;

// GLFW callback wrappers (kept for backward compatibility but not used)
static void mouse_callback_wrapper(GLFWwindow* window, double xpos, double ypos) {
    if (g_inputManager) {
        g_inputManager->OnMouseMove(xpos, ypos);
    }
}

static void mouse_button_callback_wrapper(GLFWwindow* window, int button, int action, int mods) {
    if (g_inputManager) {
        g_inputManager->OnMouseButton(button, action, mods);
    }
}

static void key_callback_wrapper(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (g_inputManager) {
        g_inputManager->OnKeyboard(key, scancode, action, mods);
    }
}

static void scroll_callback_wrapper(GLFWwindow* window, double xoffset, double yoffset) {
    if (g_inputManager) {
        g_inputManager->OnScroll(xoffset, yoffset);
    }
}

InputManager::InputManager()
    : m_Window(nullptr)
    , m_Camera(nullptr)
    , m_CurrentContext(InputContext::SCENE_EDITOR)
    , m_CurrentMode(InputMode::CAMERA_FLY)
    , m_SceneEditorMode(true)
    , m_CameraControlEnabled(true)
    , m_CameraMouseCaptured(false)
    , m_LastMouseX(0.0)
    , m_LastMouseY(0.0)
    , m_FirstMouse(true)
    , m_RightMousePressed(false)
{
    g_inputManager = this;
}

InputManager::~InputManager() {
    g_inputManager = nullptr;
}

void InputManager::Initialize(GLFWwindow* window) {
    m_Window = window;
    
    std::cout << "InputManager: Initializing with PURE POLLING (no callback conflicts)" << std::endl;
    
    // Ensure mouse look is DISABLED at startup
    m_CameraMouseCaptured = false;
    m_RightMousePressed = false;
    m_FirstMouse = true;
    
    // Set initial cursor mode - always start with normal cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
    // DO NOT install any cursor position callback - let ImGui handle it
    // We'll use pure polling for mouse position when we need camera look
    
    std::cout << "InputManager: Pure polling mode - ImGui callbacks untouched" << std::endl;
}

void InputManager::Update(float deltaTime) {
    // Check ImGui state first
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
        m_CurrentContext = InputContext::UI_FOCUSED;
        // When UI has focus, ensure camera look is disabled
        if (m_CameraMouseCaptured) {
            SetCameraMouseCapture(false);
        }
        return;
    }
    
    // Reset context if not UI focused
    if (m_CurrentContext == InputContext::UI_FOCUSED) {
        m_CurrentContext = m_SceneEditorMode ? InputContext::SCENE_EDITOR : InputContext::GAME_PLAY;
    }
    
    // Update key states using direct polling
    UpdateKeyStatesFromPolling();
    
    // Update mouse button states using polling (critical for mouse look control)
    UpdateMouseStatesFromPolling();
    
    // Process input based on current context
    switch (m_CurrentContext) {
        case InputContext::SCENE_EDITOR:
            ProcessSceneEditorInput(deltaTime);
            break;
        case InputContext::GAME_PLAY:
            ProcessGameplayInput(deltaTime);
            break;
        case InputContext::UI_FOCUSED:
            ProcessUIInput(deltaTime);
            break;
    }
}

void InputManager::SetInputContext(InputContext context) {
    if (m_CurrentContext != context) {
        m_CurrentContext = context;
        
        // Update cursor mode based on context
        if (m_Window) {
            switch (context) {
                case InputContext::SCENE_EDITOR:
                    // In Scene Editor, mouse is free unless right-click is held
                    if (!m_RightMousePressed) {
                        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                        m_CameraMouseCaptured = false;
                    }
                    break;
                case InputContext::GAME_PLAY:
                    // In Gameplay, mouse is always captured
                    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    m_CameraMouseCaptured = true;
                    break;
                case InputContext::UI_FOCUSED:
                    // When UI has focus, mouse is always free
                    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    m_CameraMouseCaptured = false;
                    break;
            }
        }
    }
}

void InputManager::SetInputMode(InputMode mode) {
    m_CurrentMode = mode;
}

void InputManager::EnableSceneEditorMode(bool enable) {
    m_SceneEditorMode = enable;
    if (enable) {
        SetInputContext(InputContext::SCENE_EDITOR);
    } else {
        SetInputContext(InputContext::GAME_PLAY);
    }
}

void InputManager::SetCameraMouseCapture(bool capture) {
    // Only change if state is actually different
    if (m_CameraMouseCaptured == capture) return;
    
    m_CameraMouseCaptured = capture;
    if (m_Window) {
        if (capture) {
            // Enable camera look mode
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            m_FirstMouse = true; // Reset to prevent camera jump
            
            // Get current position as starting point
            double xpos, ypos;
            glfwGetCursorPos(m_Window, &xpos, &ypos);
            m_LastMouseX = xpos;
            m_LastMouseY = ypos;
        } else {
            // Disable camera look mode
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            
            // Reset mouse tracking to prevent jump when re-enabling
            m_FirstMouse = true;
        }
    }
}

void InputManager::SetCamera(Camera* camera) {
    m_Camera = camera;
}

void InputManager::OnMouseMove(double xpos, double ypos) {
    // This callback method is not used in pure polling mode
    // All mouse handling is done in UpdateMouseStatesFromPolling()
    // Keep this method for potential future use
}

void InputManager::OnMouseButton(int button, int action, int mods) {
    // This callback is not used anymore since we're using polling
    // All mouse button processing happens in UpdateMouseStatesFromPolling()
    // Keep this method for backward compatibility but don't process anything
}

void InputManager::OnKeyboard(int key, int scancode, int action, int mods) {
    // This callback is not used anymore since we're using polling
    // All keyboard processing happens in UpdateKeyStatesFromPolling()
    // Keep this method for backward compatibility but don't process anything
}

void InputManager::OnScroll(double xoffset, double yoffset) {
    if (m_Camera && m_CameraControlEnabled) {
        m_Camera->ProcessMouseScroll(static_cast<float>(yoffset));
    }
}

void InputManager::ProcessSceneEditorInput(float deltaTime) {
    // In Scene Editor mode, camera movement works regardless of right mouse state
    // This allows for free camera navigation like in professional editors
    if (m_CameraControlEnabled) {
        ProcessCameraMovement(deltaTime);
    }
    
    // Mouse look only when right mouse is held (Unity style)
    // This keeps the cursor free for UI interaction
    
    // Other scene editor specific input handling can go here
}

void InputManager::ProcessGameplayInput(float deltaTime) {
    // Always process camera movement in gameplay mode
    if (m_CameraControlEnabled) {
        ProcessCameraMovement(deltaTime);
    }
    
    // Other gameplay specific input handling
}

void InputManager::ProcessUIInput(float deltaTime) {
    // UI is handling input, don't process camera or other game inputs
}

void InputManager::ProcessCameraMovement(float deltaTime) {
    if (!m_Camera) {
        static bool errorShown = false;
        if (!errorShown) {
            std::cout << "ERROR: No camera set for InputManager!" << std::endl;
            errorShown = true;
        }
        return;
    }
    
    bool keyPressed = false;
    
    if (m_KeyStates[GLFW_KEY_W]) {
        m_Camera->ProcessKeyboard(FORWARD, deltaTime);
        keyPressed = true;
    }
    if (m_KeyStates[GLFW_KEY_S]) {
        m_Camera->ProcessKeyboard(BACKWARD, deltaTime);
        keyPressed = true;
    }
    if (m_KeyStates[GLFW_KEY_A]) {
        m_Camera->ProcessKeyboard(LEFT, deltaTime);
        keyPressed = true;
    }
    if (m_KeyStates[GLFW_KEY_D]) {
        m_Camera->ProcessKeyboard(RIGHT, deltaTime);
        keyPressed = true;
    }
    if (m_KeyStates[GLFW_KEY_Q]) {
        m_Camera->ProcessKeyboard(DOWN, deltaTime);
        keyPressed = true;
    }
    if (m_KeyStates[GLFW_KEY_E]) {
        m_Camera->ProcessKeyboard(UP, deltaTime);
        keyPressed = true;
    }
    
    // Show debug only first time camera moves
    static bool firstMovement = true;
    if (keyPressed && firstMovement) {
        std::cout << "SUCCESS: Camera movement working! WASD/QE controls active." << std::endl;
        firstMovement = false;
    }
}

void InputManager::ProcessCameraLook(double xpos, double ypos) {
    // This should only be called when mouse look is explicitly enabled
    if (!m_Camera) return;
    
    if (m_FirstMouse) {
        m_LastMouseX = xpos;
        m_LastMouseY = ypos;
        m_FirstMouse = false;
        return; // Don't process movement on first frame to prevent jump
    }

    float xoffset = static_cast<float>(xpos - m_LastMouseX);
    float yoffset = static_cast<float>(m_LastMouseY - ypos); // reversed since y-coordinates go from bottom to top
    
    // Update last mouse position for next frame
    m_LastMouseX = xpos;
    m_LastMouseY = ypos;

    // Apply camera movement
    m_Camera->ProcessMouseMovement(xoffset, yoffset);
}

void InputManager::UpdateKeyStatesFromPolling() {
    if (!m_Window) return;
    
    // Update key states for WASD/QE using direct polling
    // This bypasses callback issues with ImGui
    
    // Check each key and update state
    m_KeyStates[GLFW_KEY_W] = (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS);
    m_KeyStates[GLFW_KEY_A] = (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS);
    m_KeyStates[GLFW_KEY_S] = (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS);
    m_KeyStates[GLFW_KEY_D] = (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS);
    m_KeyStates[GLFW_KEY_Q] = (glfwGetKey(m_Window, GLFW_KEY_Q) == GLFW_PRESS);
    m_KeyStates[GLFW_KEY_E] = (glfwGetKey(m_Window, GLFW_KEY_E) == GLFW_PRESS);
    
    // Also check mode toggle key
    m_KeyStates[GLFW_KEY_F3] = (glfwGetKey(m_Window, GLFW_KEY_F3) == GLFW_PRESS);
    
    // Check if any movement keys are pressed
    bool wasdPressed = m_KeyStates[GLFW_KEY_W] || m_KeyStates[GLFW_KEY_A] || 
                       m_KeyStates[GLFW_KEY_S] || m_KeyStates[GLFW_KEY_D] ||
                       m_KeyStates[GLFW_KEY_Q] || m_KeyStates[GLFW_KEY_E];
    
    // Show confirmation only when first detecting movement
    static bool movementDetected = false;
    if (wasdPressed && !movementDetected) {
        std::cout << "SUCCESS: WASD/QE input detected and working!" << std::endl;
        movementDetected = true;
    }
    
    // Handle F3 toggle manually since we're not using callbacks
    static bool lastF3State = false;
    bool currentF3State = m_KeyStates[GLFW_KEY_F3];
    if (currentF3State && !lastF3State) {
        // F3 just pressed
        EnableSceneEditorMode(!m_SceneEditorMode);
        if (m_SceneEditorMode) {
            std::cout << "=== SCENE EDITOR MODE ===" << std::endl;
            std::cout << "Camera: WASD/QE for movement (always active)" << std::endl;
            std::cout << "Mouse: Right-click + drag for look around" << std::endl;
        } else {
            std::cout << "=== GAMEPLAY MODE ===" << std::endl;
            std::cout << "Camera: WASD/QE + mouse look" << std::endl;
            std::cout << "Mouse: Locked for immersive play" << std::endl;
        }
    }
    lastF3State = currentF3State;
}

void InputManager::UpdateMouseStatesFromPolling() {
    if (!m_Window) return;
    
    // Get current mouse button states using direct polling
    bool rightMousePressed = (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    bool leftMousePressed = (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    
    // Update mouse button states
    m_MouseStates[GLFW_MOUSE_BUTTON_RIGHT] = rightMousePressed;
    m_MouseStates[GLFW_MOUSE_BUTTON_LEFT] = leftMousePressed;
    
    // Handle right mouse button for camera look control in Scene Editor mode
    if (m_SceneEditorMode) {
        if (rightMousePressed && !m_RightMousePressed) {
            // Right mouse just pressed - enable camera look
            m_RightMousePressed = true;
            SetCameraMouseCapture(true);
            
            // Store current mouse position as starting point to prevent jump
            double xpos, ypos;
            glfwGetCursorPos(m_Window, &xpos, &ypos);
            m_LastMouseX = xpos;
            m_LastMouseY = ypos;
            m_FirstMouse = true; // This will be set to false on first movement
            
            std::cout << "Mouse look ENABLED (right-click)" << std::endl;
        } else if (!rightMousePressed && m_RightMousePressed) {
            // Right mouse just released - disable camera look
            m_RightMousePressed = false;
            SetCameraMouseCapture(false);
            m_FirstMouse = true; // Reset for next time
            std::cout << "Mouse look DISABLED (right-click released)" << std::endl;
        }
    } else {
        // In Gameplay mode, mouse look is always enabled
        if (!m_CameraMouseCaptured) {
            SetCameraMouseCapture(true);
        }
    }
    
    // Only poll mouse position and process camera look if we explicitly want it
    if (m_CameraMouseCaptured && m_Camera && m_CameraControlEnabled) {
        // Get current mouse position using polling
        double xpos, ypos;
        glfwGetCursorPos(m_Window, &xpos, &ypos);
        ProcessCameraLook(xpos, ypos);
    }
    
    // When mouse look is disabled, we don't touch mouse position at all
    // This leaves ImGui free to handle all mouse events for UI interactions
}