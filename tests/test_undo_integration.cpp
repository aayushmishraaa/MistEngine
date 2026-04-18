#include <catch2/catch_all.hpp>

#include "Editor/UndoStack.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/HierarchyComponent.h"
#include "ECS/Systems/HierarchySystem.h"

#include <glm/glm.hpp>

// Integration-flavoured tests that drive the real ECS (not a mock)
// through the same Command shapes UIManager pushes. Catches regressions
// in the redo/undo lambdas themselves plus the interplay with
// HierarchySystem::Attach/Detach.
//
// These tests don't spin up UIManager — it owns a window and ImGui
// context. Instead we exercise the UndoStack against a plain
// Coordinator fixture, which is the substrate UIManager's commands
// ultimately mutate.

namespace {
struct Fixture {
    Coordinator coord;
    std::shared_ptr<HierarchySystem> sys;

    Fixture() {
        coord.Init();
        coord.RegisterComponent<TransformComponent>();
        coord.RegisterComponent<HierarchyComponent>();
        sys = coord.RegisterSystem<HierarchySystem>();
        Signature sig;
        sig.set(coord.GetComponentType<TransformComponent>());
        sig.set(coord.GetComponentType<HierarchyComponent>());
        coord.SetSystemSignature<HierarchySystem>(sig);
    }

    Entity MakeEntity(glm::vec3 pos = {0, 0, 0}) {
        Entity e = coord.CreateEntity();
        TransformComponent t; t.position = pos;
        coord.AddComponent(e, t);
        coord.AddComponent(e, HierarchyComponent{});
        return e;
    }
};
} // namespace

TEST_CASE("Transform edit undo round-trips position", "[undo][integration]") {
    Fixture f;
    Entity e = f.MakeEntity({1, 0, 0});
    Mist::Editor::UndoStack stack;

    // Mirror what DrawTransformComponent pushes after a live-edit.
    glm::vec3 oldPos = f.coord.GetComponent<TransformComponent>(e).position;
    glm::vec3 newPos = {5, 0, 0};
    f.coord.GetComponent<TransformComponent>(e).position = newPos;

    Coordinator* c = &f.coord;
    Mist::Editor::Command cmd;
    cmd.label = "Transform";
    cmd.merge_key = (static_cast<std::uint64_t>(e) << 8) | 0x01;
    cmd.redo = [c, e, newPos]() { c->GetComponent<TransformComponent>(e).position = newPos; };
    cmd.undo = [c, e, oldPos]() { c->GetComponent<TransformComponent>(e).position = oldPos; };
    stack.Push(std::move(cmd));

    REQUIRE(f.coord.GetComponent<TransformComponent>(e).position == newPos);
    stack.Undo();
    REQUIRE(f.coord.GetComponent<TransformComponent>(e).position == oldPos);
    stack.Redo();
    REQUIRE(f.coord.GetComponent<TransformComponent>(e).position == newPos);
}

TEST_CASE("Reparent undo restores old parent link", "[undo][integration]") {
    Fixture f;
    Entity root  = f.MakeEntity();
    Entity childA = f.MakeEntity();
    HierarchySystem::Attach(f.coord, root, childA);

    Entity newParent = f.MakeEntity();
    Mist::Editor::UndoStack stack;

    Coordinator* c = &f.coord;
    Entity oldParent = f.coord.GetComponent<HierarchyComponent>(childA).parent;
    REQUIRE(oldParent == root);

    // Apply: detach + reattach under newParent.
    HierarchySystem::Detach(f.coord, childA);
    HierarchySystem::Attach(f.coord, newParent, childA);

    Mist::Editor::Command cmd;
    cmd.label = "Reparent";
    cmd.merge_key = 0;
    cmd.redo = [c, childA, newParent]() {
        HierarchySystem::Detach(*c, childA);
        HierarchySystem::Attach(*c, newParent, childA);
    };
    cmd.undo = [c, childA, oldParent]() {
        HierarchySystem::Detach(*c, childA);
        HierarchySystem::Attach(*c, oldParent, childA);
    };
    stack.Push(std::move(cmd));

    REQUIRE(f.coord.GetComponent<HierarchyComponent>(childA).parent == newParent);
    stack.Undo();
    REQUIRE(f.coord.GetComponent<HierarchyComponent>(childA).parent == root);
    // Old parent's children list should contain childA again.
    const auto& kids = f.coord.GetComponent<HierarchyComponent>(root).children;
    REQUIRE(std::find(kids.begin(), kids.end(), childA) != kids.end());
}

TEST_CASE("Entity delete + undo restores the entity", "[undo][integration]") {
    Fixture f;
    Entity e = f.MakeEntity({3, 4, 5});
    REQUIRE(f.coord.GetLivingEntities().count(e) == 1);

    // Snapshot what we'll need to restore; mirrors UIManager's
    // EntitySnapshot shape but inline for the test.
    glm::vec3 pos = f.coord.GetComponent<TransformComponent>(e).position;

    f.coord.DestroyEntity(e);
    REQUIRE(f.coord.GetLivingEntities().count(e) == 0);

    Mist::Editor::UndoStack stack;
    Coordinator* c = &f.coord;

    // New entity on respawn — the undo lambda captures the snapshot
    // by value, creates a fresh entity, re-adds components. Redo
    // destroys the latest id.
    auto newIdRef = std::make_shared<Entity>(static_cast<Entity>(-1));
    Mist::Editor::Command cmd;
    cmd.label = "Delete entity";
    cmd.merge_key = 0;
    cmd.undo = [c, pos, newIdRef]() {
        Entity n = c->CreateEntity();
        TransformComponent t; t.position = pos;
        c->AddComponent(n, t);
        c->AddComponent(n, HierarchyComponent{});
        *newIdRef = n;
    };
    cmd.redo = [c, newIdRef]() {
        if (c->GetLivingEntities().count(*newIdRef)) c->DestroyEntity(*newIdRef);
    };
    stack.Push(std::move(cmd));

    stack.Undo();
    REQUIRE(newIdRef.use_count() > 0);
    REQUIRE(*newIdRef != static_cast<Entity>(-1));
    REQUIRE(f.coord.GetComponent<TransformComponent>(*newIdRef).position == pos);

    stack.Redo();
    REQUIRE(f.coord.GetLivingEntities().count(*newIdRef) == 0);
}

TEST_CASE("Dirty flag flips with Push + clears at MarkSaved / Undo-to-mark", "[undo][integration]") {
    Mist::Editor::UndoStack s;
    int v = 0;
    auto makeCmd = [&v](int n, int o) {
        Mist::Editor::Command c;
        c.redo = [&v, n]() { v = n; };
        c.undo = [&v, o]() { v = o; };
        c.label = "set";
        return c;
    };

    REQUIRE_FALSE(s.IsDirty());
    s.Push(makeCmd(1, 0)); v = 1;
    REQUIRE(s.IsDirty());

    s.MarkSaved();
    REQUIRE_FALSE(s.IsDirty());

    s.Push(makeCmd(2, 1)); v = 2;
    REQUIRE(s.IsDirty());
    s.Undo();
    // We're back at the saved mark.
    REQUIRE_FALSE(s.IsDirty());
}
