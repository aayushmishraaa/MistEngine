#pragma once
#ifndef MIST_INPUT_CONTEXT_H
#define MIST_INPUT_CONTEXT_H

#include "InputAction.h"
#include <string>
#include <unordered_map>
#include <vector>

class InputContextMap {
public:
    std::string name;
    std::unordered_map<std::string, InputAction> actions;

    void AddAction(const InputAction& action) {
        actions[action.name] = action;
    }

    InputAction* FindAction(const std::string& actionName) {
        auto it = actions.find(actionName);
        return it != actions.end() ? &it->second : nullptr;
    }
};

// Predefined context factories
inline InputContextMap CreateEditorContext() {
    InputContextMap ctx;
    ctx.name = "Editor";

    // Camera movement
    ctx.AddAction({"MoveForward", InputActionType::Button, {{InputDevice::Keyboard, 87 /*W*/}}});   // GLFW_KEY_W
    ctx.AddAction({"MoveBackward", InputActionType::Button, {{InputDevice::Keyboard, 83 /*S*/}}});
    ctx.AddAction({"MoveLeft", InputActionType::Button, {{InputDevice::Keyboard, 65 /*A*/}}});
    ctx.AddAction({"MoveRight", InputActionType::Button, {{InputDevice::Keyboard, 68 /*D*/}}});
    ctx.AddAction({"MoveUp", InputActionType::Button, {{InputDevice::Keyboard, 69 /*E*/}}});
    ctx.AddAction({"MoveDown", InputActionType::Button, {{InputDevice::Keyboard, 81 /*Q*/}}});

    // Editor actions
    ctx.AddAction({"Select", InputActionType::Button, {{InputDevice::Mouse, 0 /*LMB*/}}});
    ctx.AddAction({"Undo", InputActionType::Button, {{InputDevice::Keyboard, 90 /*Z*/}}}); // + Ctrl modifier
    ctx.AddAction({"Redo", InputActionType::Button, {{InputDevice::Keyboard, 89 /*Y*/}}}); // + Ctrl modifier
    ctx.AddAction({"Delete", InputActionType::Button, {{InputDevice::Keyboard, 261 /*DELETE*/}}});

    return ctx;
}

inline InputContextMap CreateGameplayContext() {
    InputContextMap ctx;
    ctx.name = "Gameplay";

    // Movement
    ctx.AddAction({"MoveForward", InputActionType::Button, {
        {InputDevice::Keyboard, 87 /*W*/},
        {InputDevice::GamepadAxis, 1, -1.0f}  // Left stick Y (inverted)
    }});
    ctx.AddAction({"MoveBackward", InputActionType::Button, {
        {InputDevice::Keyboard, 83 /*S*/},
        {InputDevice::GamepadAxis, 1, 1.0f}
    }});
    ctx.AddAction({"MoveLeft", InputActionType::Button, {
        {InputDevice::Keyboard, 65 /*A*/},
        {InputDevice::GamepadAxis, 0, -1.0f}
    }});
    ctx.AddAction({"MoveRight", InputActionType::Button, {
        {InputDevice::Keyboard, 68 /*D*/},
        {InputDevice::GamepadAxis, 0, 1.0f}
    }});

    // Actions
    ctx.AddAction({"Jump", InputActionType::Button, {
        {InputDevice::Keyboard, 32 /*SPACE*/},
        {InputDevice::GamepadButton, 0 /*A button*/}
    }});
    ctx.AddAction({"Fire", InputActionType::Button, {
        {InputDevice::Mouse, 0 /*LMB*/},
        {InputDevice::GamepadAxis, 5, 1.0f}  // Right trigger
    }});
    ctx.AddAction({"Aim", InputActionType::Button, {
        {InputDevice::Mouse, 1 /*RMB*/},
        {InputDevice::GamepadAxis, 4, 1.0f}  // Left trigger
    }});
    ctx.AddAction({"Reload", InputActionType::Button, {
        {InputDevice::Keyboard, 82 /*R*/},
        {InputDevice::GamepadButton, 2 /*X button*/}
    }});
    ctx.AddAction({"Sprint", InputActionType::Button, {
        {InputDevice::Keyboard, 340 /*LEFT_SHIFT*/},
        {InputDevice::GamepadButton, 8 /*Left stick click*/}
    }});

    // Look (2D axis)
    ctx.AddAction({"LookX", InputActionType::Axis1D, {
        {InputDevice::GamepadAxis, 2, 1.0f}  // Right stick X
    }});
    ctx.AddAction({"LookY", InputActionType::Axis1D, {
        {InputDevice::GamepadAxis, 3, 1.0f}  // Right stick Y
    }});

    return ctx;
}

#endif
