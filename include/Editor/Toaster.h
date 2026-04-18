#pragma once
#ifndef MIST_TOASTER_H
#define MIST_TOASTER_H

#include <deque>
#include <string>

// Non-blocking notification system inspired by Godot's `EditorToaster`.
// Engine code pushes short messages that appear stacked in the bottom-
// right corner and fade out after a TTL. Hovering a toast freezes its
// countdown so the user can read long errors without hurrying.
//
// Usage:
//   Toaster::Instance().Push(ToastLevel::Error, "Shader failed to compile");
//
// Rendering: UIManager::NewFrame() calls Toaster::Instance().Draw()
// once per frame after the main menu/layout.

namespace Mist::Editor {

enum class ToastLevel {
    Info,
    Warn,
    Error,
};

struct Toast {
    ToastLevel  level   = ToastLevel::Info;
    std::string message;
    float       ttl     = 4.0f;  // seconds before auto-dismiss
    float       age     = 0.0f;  // advanced by Draw() each frame
    bool        hovered = false; // set by Draw(); freezes aging
};

class Toaster {
public:
    static Toaster& Instance();

    void Push(ToastLevel level, const std::string& message, float ttl_seconds = 4.0f);

    // One call per frame from UIManager::NewFrame(). Pass the frame
    // delta (seconds) so TTL countdown is frame-rate independent.
    void Draw(float deltaTime);

    // Wipe all active toasts — useful when switching scenes.
    void Clear();

    std::size_t ActiveCount() const { return m_Toasts.size(); }

private:
    Toaster() = default;
    static constexpr std::size_t kMaxToasts = 5;  // cap to prevent spam-growth

    std::deque<Toast> m_Toasts;
};

} // namespace Mist::Editor

#endif // MIST_TOASTER_H
