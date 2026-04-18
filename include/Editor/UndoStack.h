#pragma once
#ifndef MIST_UNDO_STACK_H
#define MIST_UNDO_STACK_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Per-scene undo stack with *merge* semantics — ported from Godot's
// `EditorUndoRedoManager` + `UndoRedo::create_action(..., merge_mode)`.
//
// Each operation is a `Command` bundling a redo/undo lambda pair, a
// human-readable label, and a `merge_key`. When `Push` sees a command
// whose merge_key matches the one at the top of the stack AND the two
// arrived within `kMergeWindow` of each other, the new command's redo
// replaces the previous one (keeping the original undo intact). This
// is what lets a long slider drag collapse into a single undo step
// instead of producing one command per frame.
//
// Use merge_key = 0 to disable merging (entity create/delete, etc.).
// For continuous edits, hash a stable pair like `(entity_id,
// property_name)` into a non-zero key.
//
// The stack is single-scene for now. Multi-scene (tab per scene,
// per-tab history) fits the existing shape — the future
// `UIManager::LoadScene` call just swaps which UndoStack is active.

namespace Mist::Editor {

struct Command {
    std::function<void()> redo;
    std::function<void()> undo;
    std::string           label;
    std::uint64_t         merge_key = 0;
    std::chrono::steady_clock::time_point ts{};
};

class UndoStack {
public:
    // Window within which matching merge_keys collapse. 500ms matches
    // a typical slider-drag cadence: slow rebinding won't incorrectly
    // merge, fast drags will.
    static constexpr std::chrono::milliseconds kMergeWindow{500};

    // Push a new command. By contract the caller has *already* applied
    // the change — `redo` is stored for future Redo() calls; `undo`
    // must revert to the state the caller saw before applying.
    //
    // If the new command's merge_key is non-zero AND matches the top
    // of the stack AND the top arrived within kMergeWindow, the new
    // command's redo replaces the previous top's redo in place (the
    // original undo is preserved — that's the "endpoint" state we'd
    // want to undo back to).
    void Push(Command cmd);

    void Undo();
    void Redo();

    bool CanUndo() const { return !m_Undo.empty(); }
    bool CanRedo() const { return !m_Redo.empty(); }

    std::size_t UndoCount() const { return m_Undo.size(); }
    std::size_t RedoCount() const { return m_Redo.size(); }

    // Wipe both stacks. Called on LoadScene so you don't undo past
    // the scene transition into weird half-states.
    void Clear();

    // Mark the current top as "saved" — IsDirty() stays false until
    // Push/Undo/Redo move the top away from this mark.
    void MarkSaved();
    bool IsDirty() const;

    // Top-of-stack label for UI display ("Undo Translate").
    std::string TopUndoLabel() const;
    std::string TopRedoLabel() const;

private:
    std::vector<Command> m_Undo;
    std::vector<Command> m_Redo;
    std::size_t          m_SavedMark = 0;   // size of m_Undo when last saved
};

} // namespace Mist::Editor

#endif // MIST_UNDO_STACK_H
