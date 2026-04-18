#include "Editor/Toaster.h"

#include "imgui.h"

#include <algorithm>

namespace Mist::Editor {

namespace {

ImVec4 colorForLevel(ToastLevel l) {
    switch (l) {
        case ToastLevel::Info:  return ImVec4(0.26f, 0.59f, 0.98f, 0.92f); // accent blue
        case ToastLevel::Warn:  return ImVec4(0.95f, 0.75f, 0.18f, 0.92f); // amber
        case ToastLevel::Error: return ImVec4(0.95f, 0.30f, 0.30f, 0.92f); // red
    }
    return ImVec4(0.5f, 0.5f, 0.5f, 0.92f);
}

const char* prefixForLevel(ToastLevel l) {
    switch (l) {
        case ToastLevel::Info:  return "[i]";
        case ToastLevel::Warn:  return "[!]";
        case ToastLevel::Error: return "[X]";
    }
    return "[?]";
}

} // namespace

Toaster& Toaster::Instance() {
    static Toaster t;
    return t;
}

void Toaster::Push(ToastLevel level, const std::string& message, float ttl) {
    Toast t;
    t.level   = level;
    t.message = message;
    t.ttl     = ttl;
    m_Toasts.push_back(std::move(t));

    // Cap queue depth so a spam-source (per-frame warning) can't grow
    // the deque without bound.
    while (m_Toasts.size() > kMaxToasts) {
        m_Toasts.pop_front();
    }
}

void Toaster::Clear() {
    m_Toasts.clear();
}

void Toaster::Draw(float deltaTime) {
    if (m_Toasts.empty()) return;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const float   padding   = 10.0f;
    const ImVec2  base      = ImVec2(vp->WorkPos.x + vp->WorkSize.x - padding,
                                     vp->WorkPos.y + vp->WorkSize.y - padding);
    const ImVec2  pivot     = ImVec2(1.0f, 1.0f);   // anchor bottom-right

    // Expire from the front (oldest). Do this before rendering so the
    // list is stable during draw, and advance age only when not hovered
    // (hover freezes the countdown — Godot-style "user is reading this").
    for (auto& t : m_Toasts) {
        if (!t.hovered) t.age += deltaTime;
    }
    m_Toasts.erase(
        std::remove_if(m_Toasts.begin(), m_Toasts.end(),
                       [](const Toast& t) { return t.age >= t.ttl; }),
        m_Toasts.end());

    float yOffset = 0.0f;
    // Render newest at the bottom, stack upward from there.
    int i = 0;
    for (auto it = m_Toasts.rbegin(); it != m_Toasts.rend(); ++it, ++i) {
        Toast& t = *it;

        // Smooth fade during the last 0.5s of life.
        float alpha = 1.0f;
        if (t.ttl - t.age < 0.5f) alpha = std::max(0.0f, (t.ttl - t.age) / 0.5f);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, colorForLevel(t.level));
        ImGui::PushStyleColor(ImGuiCol_Text,     ImVec4(1.0f, 1.0f, 1.0f, alpha));
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);

        char winName[32];
        std::snprintf(winName, sizeof(winName), "##toast_%d", i);
        ImGui::SetNextWindowPos(ImVec2(base.x, base.y - yOffset), ImGuiCond_Always, pivot);
        // NoDocking would be nice once we swap ImGui for the docking
        // branch, but the master-branch vendored build doesn't expose
        // that flag. Toasts are auto-resize + no-saved-settings — they
        // behave like floating overlays without it.
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
                               | ImGuiWindowFlags_NoNav
                               | ImGuiWindowFlags_AlwaysAutoResize
                               | ImGuiWindowFlags_NoFocusOnAppearing
                               | ImGuiWindowFlags_NoSavedSettings;
        if (ImGui::Begin(winName, nullptr, flags)) {
            ImGui::TextUnformatted(prefixForLevel(t.level));
            ImGui::SameLine();
            ImGui::TextUnformatted(t.message.c_str());
            t.hovered = ImGui::IsWindowHovered();
            yOffset += ImGui::GetWindowHeight() + 4.0f;
        }
        ImGui::End();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }
}

} // namespace Mist::Editor
