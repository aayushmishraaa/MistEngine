#pragma once
#ifndef MIST_INPUT_ACTION_H
#define MIST_INPUT_ACTION_H

#include <string>
#include <vector>

enum class InputDevice {
    Keyboard,
    Mouse,
    GamepadButton,
    GamepadAxis
};

enum class InputActionType {
    Button,   // pressed/released
    Axis1D,   // single axis value
    Axis2D    // two-axis (e.g. stick)
};

struct InputBinding {
    InputDevice device = InputDevice::Keyboard;
    int code = 0;        // GLFW key/button code or axis index
    float scale = 1.0f;  // axis multiplier (e.g. -1 for negative axis)
};

struct InputAction {
    std::string name;
    InputActionType type = InputActionType::Button;
    std::vector<InputBinding> bindings;
};

#endif
