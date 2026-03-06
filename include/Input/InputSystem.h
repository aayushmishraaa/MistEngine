#pragma once
#ifndef MIST_INPUT_SYSTEM_H
#define MIST_INPUT_SYSTEM_H

#include "InputAction.h"
#include "InputContext.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

class InputSystem {
public:
    void Init(GLFWwindow* window);
    void Update();

    // Context management
    void PushContext(const InputContextMap& context);
    void PopContext();
    void SetContext(const InputContextMap& context);
    const InputContextMap* GetCurrentContext() const;

    // Action queries
    bool IsActionPressed(const std::string& action) const;
    bool IsActionJustPressed(const std::string& action) const;
    bool IsActionJustReleased(const std::string& action) const;
    float GetAxisValue(const std::string& action) const;
    glm::vec2 GetAxis2D(const std::string& xAction, const std::string& yAction) const;

    // Raw input queries (fallback)
    bool IsKeyPressed(int key) const;
    bool IsKeyJustPressed(int key) const;
    bool IsMouseButtonPressed(int button) const;
    bool IsMouseButtonJustPressed(int button) const;
    glm::vec2 GetMousePosition() const;
    glm::vec2 GetMouseDelta() const;
    float GetScrollDelta() const;

    // Gamepad
    bool IsGamepadConnected(int id = 0) const;
    float GetGamepadAxis(int id, int axis) const;
    bool IsGamepadButtonPressed(int id, int button) const;

    // Rebinding
    void RebindAction(const std::string& action, const InputBinding& newBinding);

    // Callbacks
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);

private:
    GLFWwindow* m_Window = nullptr;
    std::vector<InputContextMap> m_ContextStack;

    // Keyboard state
    bool m_Keys[512] = {};
    bool m_KeysPrev[512] = {};

    // Mouse state
    bool m_MouseButtons[8] = {};
    bool m_MouseButtonsPrev[8] = {};
    glm::vec2 m_MousePos = {0, 0};
    glm::vec2 m_MousePosPrev = {0, 0};
    float m_ScrollDelta = 0.0f;
    float m_ScrollAccum = 0.0f;

    // Gamepad
    static constexpr int MAX_GAMEPADS = 4;

    float evaluateBinding(const InputBinding& binding) const;

    static InputSystem* s_Instance;
};

#endif
