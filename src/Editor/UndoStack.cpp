#include "Editor/UndoStack.h"

namespace Mist::Editor {

void UndoStack::Push(Command cmd) {
    if (!cmd.redo || !cmd.undo) return;  // no-op lambdas would corrupt the stack
    cmd.ts = std::chrono::steady_clock::now();

    // Any new action invalidates the redo history — matches Godot and
    // every other editor undo. The user did "something else" so
    // stepping forward past that would produce a nonsensical timeline.
    m_Redo.clear();

    if (cmd.merge_key != 0
        && !m_Undo.empty()
        && m_Undo.back().merge_key == cmd.merge_key
        && (cmd.ts - m_Undo.back().ts) < kMergeWindow) {
        // Replace redo + timestamp; keep the *original* undo which
        // still points at the pre-drag state. That's the "endpoints"
        // merge mode from Godot.
        m_Undo.back().redo = std::move(cmd.redo);
        m_Undo.back().ts   = cmd.ts;
        return;
    }

    // Non-merging push — if SavedMark pointed inside the old redo
    // stack, clearing that path above already invalidated it; no
    // extra fixup needed.
    m_Undo.push_back(std::move(cmd));
}

void UndoStack::Undo() {
    if (m_Undo.empty()) return;
    Command c = std::move(m_Undo.back());
    m_Undo.pop_back();
    c.undo();
    m_Redo.push_back(std::move(c));
}

void UndoStack::Redo() {
    if (m_Redo.empty()) return;
    Command c = std::move(m_Redo.back());
    m_Redo.pop_back();
    c.redo();
    m_Undo.push_back(std::move(c));
}

void UndoStack::Clear() {
    m_Undo.clear();
    m_Redo.clear();
    m_SavedMark = 0;
}

void UndoStack::MarkSaved() {
    m_SavedMark = m_Undo.size();
}

bool UndoStack::IsDirty() const {
    return m_Undo.size() != m_SavedMark;
}

std::string UndoStack::TopUndoLabel() const {
    return m_Undo.empty() ? std::string{} : m_Undo.back().label;
}

std::string UndoStack::TopRedoLabel() const {
    return m_Redo.empty() ? std::string{} : m_Redo.back().label;
}

} // namespace Mist::Editor
