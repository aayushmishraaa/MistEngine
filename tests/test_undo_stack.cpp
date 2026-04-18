#include <catch2/catch_all.hpp>

#include "Editor/UndoStack.h"

#include <thread>

using Mist::Editor::Command;
using Mist::Editor::UndoStack;

namespace {

// Small helper that binds a Command around mutating a single int
// via a redo lambda and reverting via undo. Reused across cases so the
// plumbing isn't repeated each time.
Command makeIntSet(int& target, int newValue, int oldValue,
                   std::uint64_t mergeKey, const char* label) {
    Command c;
    c.redo = [&target, newValue]() { target = newValue; };
    c.undo = [&target, oldValue]() { target = oldValue; };
    c.label     = label;
    c.merge_key = mergeKey;
    return c;
}

} // namespace

TEST_CASE("UndoStack Push/Undo/Redo roundtrips a single command", "[undo]") {
    UndoStack s;
    int v = 0;

    s.Push(makeIntSet(v, 42, 0, 0, "set42"));
    // Caller is contractually expected to apply before pushing; mirror
    // that here so the stack state stays truthful.
    v = 42;

    REQUIRE(s.CanUndo());
    REQUIRE_FALSE(s.CanRedo());

    s.Undo();
    REQUIRE(v == 0);
    REQUIRE(s.CanRedo());

    s.Redo();
    REQUIRE(v == 42);
}

TEST_CASE("UndoStack merges same-key pushes within the window", "[undo][merge]") {
    UndoStack s;
    int v = 0;

    // Three "drag frames" on the same merge key, spaced tightly so
    // they all fall inside kMergeWindow. Only one undo step should
    // exist at the end, and undoing should revert to the pre-drag
    // value (the *original* undo is preserved).
    s.Push(makeIntSet(v, 1, 0, /*key=*/7, "drag"));  v = 1;
    s.Push(makeIntSet(v, 2, 1, /*key=*/7, "drag"));  v = 2;
    s.Push(makeIntSet(v, 3, 2, /*key=*/7, "drag"));  v = 3;

    REQUIRE(s.UndoCount() == 1);
    s.Undo();
    REQUIRE(v == 0);  // reverts to pre-drag state, not v==2
}

TEST_CASE("UndoStack does NOT merge when merge_key differs", "[undo][merge]") {
    UndoStack s;
    int v = 0;

    s.Push(makeIntSet(v, 1, 0, /*key=*/1, "a")); v = 1;
    s.Push(makeIntSet(v, 2, 1, /*key=*/2, "b")); v = 2;

    REQUIRE(s.UndoCount() == 2);
    s.Undo();
    REQUIRE(v == 1);
    s.Undo();
    REQUIRE(v == 0);
}

TEST_CASE("UndoStack does NOT merge when merge_key is zero", "[undo][merge]") {
    UndoStack s;
    int v = 0;

    s.Push(makeIntSet(v, 1, 0, /*key=*/0, "one")); v = 1;
    s.Push(makeIntSet(v, 2, 1, /*key=*/0, "two")); v = 2;

    // merge_key == 0 is the "never merge" signal (entity create/delete
    // use this so every add/remove is its own undo step).
    REQUIRE(s.UndoCount() == 2);
}

TEST_CASE("UndoStack does NOT merge across the time window", "[undo][merge]") {
    UndoStack s;
    int v = 0;

    s.Push(makeIntSet(v, 1, 0, /*key=*/5, "slow")); v = 1;
    // Sleep past kMergeWindow (500ms) so the next push lands in a
    // new bucket — this proves slow deliberate edits stay as separate
    // undo steps even with the same key.
    std::this_thread::sleep_for(UndoStack::kMergeWindow + std::chrono::milliseconds(50));
    s.Push(makeIntSet(v, 2, 1, /*key=*/5, "slow")); v = 2;

    REQUIRE(s.UndoCount() == 2);
}

TEST_CASE("UndoStack IsDirty tracks against MarkSaved", "[undo][dirty]") {
    UndoStack s;
    int v = 0;

    REQUIRE_FALSE(s.IsDirty());

    s.Push(makeIntSet(v, 1, 0, 0, "a")); v = 1;
    REQUIRE(s.IsDirty());

    s.MarkSaved();
    REQUIRE_FALSE(s.IsDirty());

    s.Push(makeIntSet(v, 2, 1, 0, "b")); v = 2;
    REQUIRE(s.IsDirty());

    s.Undo();
    v = 1;
    REQUIRE_FALSE(s.IsDirty());  // back at the saved marker
}

TEST_CASE("UndoStack Push invalidates redo history", "[undo]") {
    UndoStack s;
    int v = 0;
    s.Push(makeIntSet(v, 1, 0, 0, "a")); v = 1;
    s.Push(makeIntSet(v, 2, 1, 0, "b")); v = 2;
    s.Undo(); v = 1;   // now there's redo available
    REQUIRE(s.CanRedo());

    s.Push(makeIntSet(v, 99, 1, 0, "c")); v = 99;
    // Doing "something else" after an undo invalidates the redo —
    // matches every editor's timeline-branch semantics.
    REQUIRE_FALSE(s.CanRedo());
}

TEST_CASE("UndoStack Clear wipes both stacks", "[undo]") {
    UndoStack s;
    int v = 0;
    s.Push(makeIntSet(v, 1, 0, 0, "a")); v = 1;
    s.Push(makeIntSet(v, 2, 1, 0, "b")); v = 2;
    s.Undo(); v = 1;

    s.Clear();
    REQUIRE_FALSE(s.CanUndo());
    REQUIRE_FALSE(s.CanRedo());
    REQUIRE_FALSE(s.IsDirty());
}
