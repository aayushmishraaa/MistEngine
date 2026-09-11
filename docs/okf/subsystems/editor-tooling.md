---
type: Subsystem Parity
title: Editor tooling
description: Undo and reflection-driven inspectors are close to Godot's; the plugin system, shortcut dispatch and play mode are written but unreachable.
tags: [parity, godot, editor, imgui, tooling]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: https://docs.godotengine.org/en/stable/tutorials/plugins/editor/making_plugins.html
    title: Making plugins — Godot docs
  - resource: https://docs.godotengine.org/en/stable/tutorials/scripting/gdscript/gdscript_exports.html
    title: GDScript exported properties — Godot docs
---

# Godot model

The editor is itself a Godot project, extended through **`EditorPlugin`**: a `@tool` script plus a
`plugin.cfg`, with `_enter_tree` / `_exit_tree` lifecycle, and registration calls for docks
(`add_control_to_dock`), custom types (`add_custom_type`), autoload singletons, inspector editors
(`EditorInspectorPlugin`) and viewport gizmos.

The inspector is driven by the **`PropertyHint`** system. `@export_range(0, 100, 1, "suffix:m")`,
`@export_enum("Warrior", "Magician")`, `@export_flags("Fire:1", "Water:2")`, `@export_file`,
`@export_multiline`, `@export_group` / `@export_subgroup` / `@export_category` for hierarchy, and
`@export_custom` for raw hint control. Hints encode constraints as strings the inspector interprets,
so a property's editing widget is declared alongside the property.

Undo goes through **`EditorUndoRedoManager`**, with merge modes so a slider drag collapses to one
step.

# MistEngine today

Genuinely close, in two places:

**Reflection-driven inspectors.** `MIST_REFLECT` / `MIST_FIELD` register a type's fields with a
`PropertyType` and a `PropertyHint` plus hint string — the same shape as Godot's system, with
`Range`, `Enum`, `Color`, `Multiline`, `File` and `ResourceRef` hints. One declaration drives three
consumers: the Inspector widget, the scene JSON, and `.mistmat` round-tripping. That is real leverage.
Enum support landed at commit `b542818`; before that `PhysicsComponent::shape` showed as a greyed
"unreflected type" while `showcase.lua` told users to edit it.

**Undo.** `Mist::Editor::UndoStack` implements Godot's "endpoints" merge model — a matching
`merge_key` within 500 ms replaces the redo and keeps the *original* undo, so a slider drag is one
step. Dirty tracking via a saved-mark index drives the window title asterisk.

The rest of the editor is a dockable ImGui layout with a persisted arrangement, three themes,
hierarchy with drag-drop reparenting, ImGuizmo handles with snapping, an asset browser, a console
that doubles as a Lua REPL, toast notifications wired to the engine logger, and a profiler with
non-blocking GPU timer queries.

# Delta

Three subsystems **were** written and unreachable. All three are now wired
(phase 4):

- **`EditorPluginRegistry`** — `UIManager` iterates `SnapshotDocks()` at the end
  of `DrawEditorLayout` and `SnapshotMenus()` under a "Plugins" menu. One real
  in-tree plugin ships (`SceneStatsPlugin`), because a registry whose only
  plugin was a test double could be asserted to compile but not shown to work.
- **`ShortcutRegistry`** — one dispatch site in `UIManager::NewFrame`. All
  fifteen registered chords now fire; previously only the hardcoded Ctrl+Z /
  Ctrl+Y and the gizmo letters were live, which made the menu labels a standing
  lie. `New Scene`, `Duplicate` and `Save As` had to be extracted from menu
  bodies first.
- **`EditorState`** — `main()` installs snapshot callbacks over
  `SceneSerializer::SaveToString` / `LoadFromString` and gates physics, scripts
  and `_ready` on `ShouldUpdateGame()`. A global gate is the honest first cut;
  process modes do not exist.

Historical record of what they looked like before:

- **`EditorPluginRegistry`** implements `IEditorPlugin` with `OnEnable`/`OnDisable`, an
  `EditorContext` for registering docks and slash-pathed menu items, and thread-safe snapshots. Called
  only from `tests/test_editor_plugin.cpp`. `UIManager` never iterates plugin docks or menus, so the
  editor is not extensible.
- **`ShortcutRegistry`** holds ids, labels and chords, and renders correct chord strings into menus.
  But `Matches` and `FindByChord` are called only from tests — so Ctrl+S, Ctrl+N, Ctrl+O, Ctrl+D,
  Delete, F5 and F8 are **displayed and not dispatched**. Only Ctrl+Z / Ctrl+Y and the gizmo keys are
  hardcoded and live.
- **`EditorState`** models Edit / Playing / Paused with snapshot callbacks. Play/Pause/Stop mutate the
  enum; nothing reads `ShouldUpdateGame()` and `SetSnapshotCallbacks` is never called. So Play does
  not snapshot and Stop does not restore — the buttons are inert.

Other gaps against Godot:

- **No property groups or categories**, so a component with 14 reflected fields (`LightComponent`)
  renders as one flat list.
- **No inspector plugin hook** — a custom widget for a custom type means editing `UIManager`.
- **No viewport gizmo registration**; light and collision gizmos are hardcoded in `Renderer.cpp`.
- **Dead duplicates** — mostly removed. `src/Editor/EditorUI.cpp` (226 lines, `namespace
  EditorPanels`) went in phase 1, the two `#if 0` FPS-UI blocks and `DrawCrosshair` in phase 4.
  `UIManager::DrawSceneView`, `DrawAssetBrowser` and `DrawConsole` remain: each appears exactly once,
  as its own definition, and `DrawSceneView` is the only caller of
  `Renderer::SetFullscreenPresent`, so the whole `Viewport` fullscreen-blit path is dead with it.
- **Menu items that do nothing:** File → Exit's body is a comment; Hierarchy → Rename re-selects the
  entity. (File → New Scene used to clear a UI cache and the selection *without destroying a single
  entity* — fixed in phase 4, which also gave `Duplicate` the undo command it never had.)
- **The Light Editor panel is overwritten every frame** — it adds lights to `LightManager`, which
  `LightSystem::Update` clears and rebuilds from ECS at the top of the next frame.

# Evidence

- `include/Core/Reflection.h` — `PropertyType` / `PropertyHint` and the `MIST_REFLECT` macros.
- `src/Editor/UndoStack.cpp` — merge window and endpoint semantics.
- `grep -rn "EditorPluginRegistry" src tests` → `src/Editor/EditorPlugin.cpp` and
  `tests/test_editor_plugin.cpp` only.
- `grep -rn "Matches(\|FindByChord" src tests` → `src/Editor/ShortcutRegistry.cpp` and
  `tests/test_shortcut_registry.cpp` only.
- `grep -rn "ShouldUpdateGame\|SetSnapshotCallbacks" src` → definitions only, no readers.
- `src/Editor/EditorUI.cpp` — `namespace EditorPanels`, never referenced.
- `src/UIManager.cpp` `DrawLightEditor` vs `src/ECS/Systems/LightSystem.cpp` per-frame rebuild.

# Depends on / Blocks

Play mode depends on scene snapshot/restore, which the scene serializer can now provide since it
became round-trip safe at commit `b542818`.
Implementation: [phase 4](/roadmap/phase-4-wire-the-built-but-dead.md).
