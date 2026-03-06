#include "Input/InputSystem.h"
#include "Core/Logger.h"
#include <cstring>
#include <cmath>

InputSystem* InputSystem::s_Instance = nullptr;

void InputSystem::Init(GLFWwindow* window) {
    m_Window = window;
    s_Instance = this;

    std::memset(m_Keys, 0, sizeof(m_Keys));
    std::memset(m_KeysPrev, 0, sizeof(m_KeysPrev));
    std::memset(m_MouseButtons, 0, sizeof(m_MouseButtons));
    std::memset(m_MouseButtonsPrev, 0, sizeof(m_MouseButtonsPrev));

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    m_MousePos = m_MousePosPrev = glm::vec2(static_cast<float>(mx), static_cast<float>(my));

    LOG_INFO("InputSystem initialized");
}

void InputSystem::Update() {
    std::memcpy(m_KeysPrev, m_Keys, sizeof(m_Keys));
    std::memcpy(m_MouseButtonsPrev, m_MouseButtons, sizeof(m_MouseButtons));
    m_MousePosPrev = m_MousePos;
    m_ScrollDelta = m_ScrollAccum;
    m_ScrollAccum = 0.0f;

    double mx, my;
    glfwGetCursorPos(m_Window, &mx, &my);
    m_MousePos = glm::vec2(static_cast<float>(mx), static_cast<float>(my));
}

void InputSystem::PushContext(const InputContextMap& context) {
    m_ContextStack.push_back(context);
}

void InputSystem::PopContext() {
    if (!m_ContextStack.empty()) m_ContextStack.pop_back();
}

void InputSystem::SetContext(const InputContextMap& context) {
    m_ContextStack.clear();
    m_ContextStack.push_back(context);
}

const InputContextMap* InputSystem::GetCurrentContext() const {
    return m_ContextStack.empty() ? nullptr : &m_ContextStack.back();
}

bool InputSystem::IsActionPressed(const std::string& action) const {
    for (auto it = m_ContextStack.rbegin(); it != m_ContextStack.rend(); ++it) {
        auto actIt = it->actions.find(action);
        if (actIt != it->actions.end()) {
            for (auto& binding : actIt->second.bindings) {
                if (std::abs(evaluateBinding(binding)) > 0.5f) return true;
            }
            return false;
        }
    }
    return false;
}

bool InputSystem::IsActionJustPressed(const std::string& action) const {
    for (auto it = m_ContextStack.rbegin(); it != m_ContextStack.rend(); ++it) {
        auto actIt = it->actions.find(action);
        if (actIt != it->actions.end()) {
            for (auto& binding : actIt->second.bindings) {
                if (binding.device == InputDevice::Keyboard) {
                    if (binding.code >= 0 && binding.code < 512) {
                        if (m_Keys[binding.code] && !m_KeysPrev[binding.code]) return true;
                    }
                } else if (binding.device == InputDevice::Mouse) {
                    if (binding.code >= 0 && binding.code < 8) {
                        if (m_MouseButtons[binding.code] && !m_MouseButtonsPrev[binding.code]) return true;
                    }
                }
            }
            return false;
        }
    }
    return false;
}

bool InputSystem::IsActionJustReleased(const std::string& action) const {
    for (auto it = m_ContextStack.rbegin(); it != m_ContextStack.rend(); ++it) {
        auto actIt = it->actions.find(action);
        if (actIt != it->actions.end()) {
            for (auto& binding : actIt->second.bindings) {
                if (binding.device == InputDevice::Keyboard) {
                    if (binding.code >= 0 && binding.code < 512) {
                        if (!m_Keys[binding.code] && m_KeysPrev[binding.code]) return true;
                    }
                } else if (binding.device == InputDevice::Mouse) {
                    if (binding.code >= 0 && binding.code < 8) {
                        if (!m_MouseButtons[binding.code] && m_MouseButtonsPrev[binding.code]) return true;
                    }
                }
            }
            return false;
        }
    }
    return false;
}

float InputSystem::GetAxisValue(const std::string& action) const {
    float maxVal = 0.0f;
    for (auto it = m_ContextStack.rbegin(); it != m_ContextStack.rend(); ++it) {
        auto actIt = it->actions.find(action);
        if (actIt != it->actions.end()) {
            for (auto& binding : actIt->second.bindings) {
                float val = evaluateBinding(binding);
                if (std::abs(val) > std::abs(maxVal)) maxVal = val;
            }
            return maxVal;
        }
    }
    return 0.0f;
}

glm::vec2 InputSystem::GetAxis2D(const std::string& xAction, const std::string& yAction) const {
    return glm::vec2(GetAxisValue(xAction), GetAxisValue(yAction));
}

bool InputSystem::IsKeyPressed(int key) const {
    return key >= 0 && key < 512 && m_Keys[key];
}

bool InputSystem::IsKeyJustPressed(int key) const {
    return key >= 0 && key < 512 && m_Keys[key] && !m_KeysPrev[key];
}

bool InputSystem::IsMouseButtonPressed(int button) const {
    return button >= 0 && button < 8 && m_MouseButtons[button];
}

bool InputSystem::IsMouseButtonJustPressed(int button) const {
    return button >= 0 && button < 8 && m_MouseButtons[button] && !m_MouseButtonsPrev[button];
}

glm::vec2 InputSystem::GetMousePosition() const { return m_MousePos; }
glm::vec2 InputSystem::GetMouseDelta() const { return m_MousePos - m_MousePosPrev; }
float InputSystem::GetScrollDelta() const { return m_ScrollDelta; }

bool InputSystem::IsGamepadConnected(int id) const {
    return glfwJoystickIsGamepad(id);
}

float InputSystem::GetGamepadAxis(int id, int axis) const {
    if (!glfwJoystickIsGamepad(id)) return 0.0f;
    GLFWgamepadstate state;
    if (glfwGetGamepadState(id, &state)) {
        if (axis >= 0 && axis < 6) {
            float val = state.axes[axis];
            // Dead zone
            if (std::abs(val) < 0.15f) return 0.0f;
            return val;
        }
    }
    return 0.0f;
}

bool InputSystem::IsGamepadButtonPressed(int id, int button) const {
    if (!glfwJoystickIsGamepad(id)) return false;
    GLFWgamepadstate state;
    if (glfwGetGamepadState(id, &state)) {
        if (button >= 0 && button < 15) return state.buttons[button] == GLFW_PRESS;
    }
    return false;
}

void InputSystem::RebindAction(const std::string& action, const InputBinding& newBinding) {
    for (auto& ctx : m_ContextStack) {
        auto it = ctx.actions.find(action);
        if (it != ctx.actions.end()) {
            it->second.bindings.clear();
            it->second.bindings.push_back(newBinding);
            return;
        }
    }
}

float InputSystem::evaluateBinding(const InputBinding& binding) const {
    switch (binding.device) {
        case InputDevice::Keyboard:
            return (binding.code >= 0 && binding.code < 512 && m_Keys[binding.code])
                ? binding.scale : 0.0f;
        case InputDevice::Mouse:
            return (binding.code >= 0 && binding.code < 8 && m_MouseButtons[binding.code])
                ? binding.scale : 0.0f;
        case InputDevice::GamepadButton:
            return IsGamepadButtonPressed(0, binding.code) ? binding.scale : 0.0f;
        case InputDevice::GamepadAxis:
            return GetGamepadAxis(0, binding.code) * binding.scale;
    }
    return 0.0f;
}

// Static callbacks
void InputSystem::KeyCallback(GLFWwindow*, int key, int, int action, int) {
    if (!s_Instance || key < 0 || key >= 512) return;
    if (action == GLFW_PRESS) s_Instance->m_Keys[key] = true;
    else if (action == GLFW_RELEASE) s_Instance->m_Keys[key] = false;
}

void InputSystem::MouseButtonCallback(GLFWwindow*, int button, int action, int) {
    if (!s_Instance || button < 0 || button >= 8) return;
    if (action == GLFW_PRESS) s_Instance->m_MouseButtons[button] = true;
    else if (action == GLFW_RELEASE) s_Instance->m_MouseButtons[button] = false;
}

void InputSystem::ScrollCallback(GLFWwindow*, double, double yoffset) {
    if (s_Instance) s_Instance->m_ScrollAccum += static_cast<float>(yoffset);
}

void InputSystem::CursorPosCallback(GLFWwindow*, double xpos, double ypos) {
    if (s_Instance) {
        s_Instance->m_MousePos = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
    }
}
