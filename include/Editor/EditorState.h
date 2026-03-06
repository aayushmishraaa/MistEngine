#pragma once
#ifndef MIST_EDITOR_STATE_H
#define MIST_EDITOR_STATE_H

#include <string>
#include <functional>

enum class EditorPlayState {
    Edit,
    Playing,
    Paused
};

class EditorState {
public:
    EditorPlayState GetState() const { return m_State; }

    void Play() {
        if (m_State == EditorPlayState::Edit) {
            if (m_OnSaveSnapshot) m_OnSaveSnapshot();
            m_State = EditorPlayState::Playing;
        } else if (m_State == EditorPlayState::Paused) {
            m_State = EditorPlayState::Playing;
        }
    }

    void Pause() {
        if (m_State == EditorPlayState::Playing) {
            m_State = EditorPlayState::Paused;
        }
    }

    void Stop() {
        if (m_State != EditorPlayState::Edit) {
            m_State = EditorPlayState::Edit;
            if (m_OnRestoreSnapshot) m_OnRestoreSnapshot();
        }
    }

    bool IsPlaying() const { return m_State == EditorPlayState::Playing; }
    bool IsPaused() const { return m_State == EditorPlayState::Paused; }
    bool IsEditing() const { return m_State == EditorPlayState::Edit; }
    bool ShouldUpdateGame() const { return m_State == EditorPlayState::Playing; }

    void SetSnapshotCallbacks(std::function<void()> save, std::function<void()> restore) {
        m_OnSaveSnapshot = save;
        m_OnRestoreSnapshot = restore;
    }

private:
    EditorPlayState m_State = EditorPlayState::Edit;
    std::function<void()> m_OnSaveSnapshot;
    std::function<void()> m_OnRestoreSnapshot;
};

#endif
