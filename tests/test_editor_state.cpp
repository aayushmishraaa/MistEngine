#include <catch2/catch_all.hpp>

#include "Editor/EditorState.h"

#include <string>

// Play mode.
//
// EditorState had no test and no callers: SetSnapshotCallbacks and
// ShouldUpdateGame were never called, so Play/Pause/Stop mutated an enum and
// the toolbar buttons were inert. These pin the state machine and the snapshot
// contract main() now depends on to gate physics and scripts.

TEST_CASE("EditorState starts in Edit and does not tick the game", "[editor][playmode]") {
    EditorState st;
    REQUIRE(st.IsEditing());
    REQUIRE_FALSE(st.ShouldUpdateGame());
}

TEST_CASE("Play snapshots exactly once on entering play", "[editor][playmode]") {
    // The snapshot must not be retaken on Pause -> Play, or resuming would
    // discard everything that happened during the play session and Stop would
    // restore the wrong world.
    EditorState st;
    int saves = 0, restores = 0;
    st.SetSnapshotCallbacks([&] { ++saves; }, [&] { ++restores; });

    st.Play();
    REQUIRE(st.IsPlaying());
    REQUIRE(st.ShouldUpdateGame());
    REQUIRE(saves == 1);

    st.Pause();
    REQUIRE(st.IsPaused());
    REQUIRE_FALSE(st.ShouldUpdateGame());   // paused means the game does not tick

    st.Play();
    REQUIRE(st.IsPlaying());
    REQUIRE(saves == 1);                    // NOT retaken
    REQUIRE(restores == 0);
}

TEST_CASE("Stop restores and returns to Edit", "[editor][playmode]") {
    EditorState st;
    int saves = 0, restores = 0;
    st.SetSnapshotCallbacks([&] { ++saves; }, [&] { ++restores; });

    st.Play();
    st.Stop();
    REQUIRE(st.IsEditing());
    REQUIRE_FALSE(st.ShouldUpdateGame());
    REQUIRE(saves == 1);
    REQUIRE(restores == 1);
}

TEST_CASE("Stop from Edit restores nothing", "[editor][playmode]") {
    // F8 while already stopped must not wipe the scene the user is editing by
    // restoring a stale snapshot.
    EditorState st;
    int restores = 0;
    st.SetSnapshotCallbacks([] {}, [&] { ++restores; });

    st.Stop();
    REQUIRE(st.IsEditing());
    REQUIRE(restores == 0);
}

TEST_CASE("Pause outside Playing is ignored", "[editor][playmode]") {
    EditorState st;
    st.Pause();
    REQUIRE(st.IsEditing());
}

TEST_CASE("A full play cycle round-trips snapshot text", "[editor][playmode]") {
    // Mirrors what main() installs: save captures the world into a string,
    // restore reads it back. The string is the contract, so an empty snapshot
    // must be distinguishable from a taken one.
    EditorState st;
    std::string snapshot;
    std::string restored;

    st.SetSnapshotCallbacks(
        [&] { snapshot = R"({"version":"1.0","entities":[]})"; },
        [&] { restored = snapshot; });

    REQUIRE(snapshot.empty());
    st.Play();
    REQUIRE_FALSE(snapshot.empty());
    st.Stop();
    REQUIRE(restored == snapshot);
}
