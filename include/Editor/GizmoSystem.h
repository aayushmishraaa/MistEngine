#pragma once
#ifndef MIST_GIZMO_SYSTEM_H
#define MIST_GIZMO_SYSTEM_H

#include <glm/glm.hpp>

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

    // Call from ImGui render loop with selected entity's transform
    // Returns true if the transform was modified
    bool Manipulate(const glm::mat4& view, const glm::mat4& projection,
                    glm::mat4& matrix, float* deltaMatrix = nullptr) {
        // ImGuizmo integration point
        // In actual implementation, this would call:
        // ImGuizmo::SetOrthographic(false);
        // ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
        //     currentOp, currentMode, glm::value_ptr(matrix), deltaMatrix, snap);
        (void)view; (void)projection; (void)matrix; (void)deltaMatrix;
        return false; // Placeholder until ImGuizmo is integrated
    }

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
