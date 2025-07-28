#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <GLFW/glfw3.h>
#include <functional>
#include <unordered_map>
#include <vector>

enum class InputContext {
    SCENE_EDITOR,
    GAME_PLAY,
    UI_FOCUSED
};

enum class InputMode {
    CAMERA_FLY,
    CAMERA_ORBIT,
    OBJECT_SELECTION,
    GIZMO_MANIPULATION
};

class Camera; // Forward declaration

class InputManager {
public:
    InputManager();
    ~InputManager();

    void Initialize(GLFWwindow* window);
    void Update(float deltaTime);
    void SetCamera(Camera* camera);
    
    // Context management
    void SetInputContext(InputContext context);
    InputContext GetInputContext() const { return m_CurrentContext; }
    
    // Mode management for scene editor
    void SetInputMode(InputMode mode);
    InputMode GetInputMode() const { return m_CurrentMode; }
    
    // Scene editor specific
    void EnableSceneEditorMode(bool enable);
    bool IsSceneEditorMode() const { return m_SceneEditorMode; }
    
    // Camera control states
    void SetCameraControlEnabled(bool enabled) { m_CameraControlEnabled = enabled; }
    bool IsCameraControlEnabled() const { return m_CameraControlEnabled; }
    
    // Mouse capture for camera (like Unity's right-click to fly)
    void SetCameraMouseCapture(bool capture);
    bool IsCameraMouseCaptured() const { return m_CameraMouseCaptured; }
    
    // Check if mouse look should be processed
    bool ShouldProcessMouseLook() const { return m_CameraMouseCaptured && m_Camera && m_CameraControlEnabled; }

    // Callbacks
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(int button, int action, int mods);
    void OnKeyboard(int key, int scancode, int action, int mods);
    void OnScroll(double xoffset, double yoffset);

private:
    GLFWwindow* m_Window;
    Camera* m_Camera;
    
    InputContext m_CurrentContext;
    InputMode m_CurrentMode;
    bool m_SceneEditorMode;
    bool m_CameraControlEnabled;
    bool m_CameraMouseCaptured;
    
    // Mouse state
    double m_LastMouseX, m_LastMouseY;
    bool m_FirstMouse;
    bool m_RightMousePressed;
    
    // Input state tracking
    std::unordered_map<int, bool> m_KeyStates;
    std::unordered_map<int, bool> m_MouseStates;
    
    // Handle different input contexts
    void ProcessSceneEditorInput(float deltaTime);
    void ProcessGameplayInput(float deltaTime);
    void ProcessUIInput(float deltaTime);
    
    // Camera controls
    void ProcessCameraMovement(float deltaTime);
    void ProcessCameraLook(double xpos, double ypos);
    
    // Polling-based input (backup for when callbacks are overridden)
    void UpdateKeyStatesFromPolling();
    void UpdateMouseStatesFromPolling();
};

#endif // INPUT_MANAGER_H