#pragma once
#ifndef MIST_GIZMO_SYSTEM_H
#define MIST_GIZMO_SYSTEM_H

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "ImGuizmo.h"

enum class GizmoMode {
    Translate,
    Rotate,
    Scale
};

enum class GizmoSpace {
    Local,
    World
};

class GizmoSystem {
public:
    void SetMode(GizmoMode mode) { m_Mode = mode; }
    GizmoMode GetMode() const { return m_Mode; }

    void SetSpace(GizmoSpace space) { m_Space = space; }
    GizmoSpace GetSpace() const { return m_Space; }

    void SetSnap(bool enabled, float value = 1.0f) {
        m_SnapEnabled = enabled;
        m_SnapValue = value;
    }

    bool IsSnapEnabled() const { return m_SnapEnabled; }
    float GetSnapValue() const { return m_SnapValue; }

    // Begin/End a gizmo frame — call Begin once before any Manipulate
    // calls, pairs with End in the UIManager render flow. Begin tells
    // ImGuizmo which drawlist to render into and which screen rect to
    // occupy; matches the hover-test rectangle.
    static void BeginFrame(float x, float y, float w, float h) {
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetRect(x, y, w, h);
    }

    // Show the gizmo for `matrix`, modify in place if the user drags
    // a handle. Returns true if `matrix` was changed this call.
    // `deltaMatrix` (if non-null) receives the frame-local delta for
    // cases where callers prefer composing the delta with a cached
    // transform instead of decomposing the result.
    bool Manipulate(const glm::mat4& view, const glm::mat4& projection,
                    glm::mat4& matrix, float* deltaMatrix = nullptr) {
        ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
        switch (m_Mode) {
            case GizmoMode::Translate: op = ImGuizmo::TRANSLATE; break;
            case GizmoMode::Rotate:    op = ImGuizmo::ROTATE;    break;
            case GizmoMode::Scale:     op = ImGuizmo::SCALE;     break;
        }
        ImGuizmo::MODE mode = (m_Space == GizmoSpace::Local)
            ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

        float snap[3] = {m_SnapValue, m_SnapValue, m_SnapValue};
        return ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(projection),
            op, mode,
            glm::value_ptr(matrix),
            deltaMatrix,
            m_SnapEnabled ? snap : nullptr);
    }

    // True while the user is dragging a gizmo handle. UI code uses this
    // to suppress camera orbit/pan so the mouse doesn't do two things
    // at once.
    static bool IsUsing() { return ImGuizmo::IsUsing(); }
    static bool IsOver()  { return ImGuizmo::IsOver();  }

    void CycleMode() {
        switch (m_Mode) {
            case GizmoMode::Translate: m_Mode = GizmoMode::Rotate; break;
            case GizmoMode::Rotate:    m_Mode = GizmoMode::Scale; break;
            case GizmoMode::Scale:     m_Mode = GizmoMode::Translate; break;
        }
    }

    void ToggleSpace() {
        m_Space = (m_Space == GizmoSpace::Local) ? GizmoSpace::World : GizmoSpace::Local;
    }

private:
    GizmoMode m_Mode = GizmoMode::Translate;
    GizmoSpace m_Space = GizmoSpace::World;
    bool m_SnapEnabled = false;
    float m_SnapValue = 1.0f;
};

#endif
