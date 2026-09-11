# 2026-09-11

- Bundle created. Godot 4 researched from the official documentation; MistEngine surveyed against
  commit `04b00b4`.
- Recorded decision 0001: prefabs will be a separate `.mistprefab` asset type rather than Godot's
  "a scene is also a prefab" model.
- Roadmap drafted as four dependency-ordered phases.
- All documents start `status: draft` / unverified trust tier.

# 2026-09-11 — phase 1 part A

- Hoisted the reflection↔JSON codec into `include/Core/ReflectionJson.h`. It
  existed twice already, both file-static (`SceneSerializer.cpp`,
  `MaterialSerializer.cpp`); the Environment resource and the prefab serializer
  would have made it four copies.
- Entity identity landed: `NameComponent`, serialized `name` field,
  `Mist::FindEntityByName`, Lua `find_entity` / `entity_name` /
  `set_entity_name`. `UIManager::m_EntityNames` deleted outright.
- Added `SceneSerializer::SaveToString` / `LoadFromString`, which made the
  serializer headlessly testable for the first time.
- Corrections to this bundle, found while verifying it against the tree:
  - `tests/test_scene_serializer.cpp` **did** already contain full round-trip
    tests (added at `b542818`); the claim in
    [phase 3](/roadmap/phase-3-prefabs.md) that only nlohmann behaviour tests
    existed was wrong, and came from reading only the file's header comment.
  - The suite is **not order-independent**, and was not before this work:
    several files each called `Coordinator::Init()` from their own latch, and
    `Init()` replaces all three managers, so whichever file reset last decided
    what was registered. `tests/test_world.h` now owns one shared reset.
    A separate pre-existing failure in `tests/test_material_asset.cpp` under
    randomised order is recorded but not addressed here.
- Tests: 113 → 122 green.

# 2026-09-11 — phase 1 part B

- `Environment` landed: one reflected, serialized struct owning 34 render
  tunables, replacing public fields on nine renderer objects. The fields were
  deleted rather than shadowed, and `PostProcessStack::Execute` plus every
  sub-renderer `Render` now take a `const Environment&`.
- Three View-menu panels collapsed into one `DrawReflectedProperties` call.
- `src/Editor/EditorUI.cpp` deleted early (phase 4 had it scheduled): it
  referenced the removed tunables, and porting unreferenced code to a new API
  purely to keep it compiling was not defensible.
- Correction: the bundle's "41 tunables" was an over-count from counting
  declared members per header. The real figure is 34; `SSRRenderer` in
  particular was credited with four and had two.
- Tests: 122 → 125 green.

# 2026-09-11 — phase 2

- Camera as a component. The depth-range hazard the phase doc warns about is
  handled by resolving one `CameraView` per frame rather than threading two
  floats to four subsystems. Two latent bugs surfaced and were fixed: the
  cluster-grid cache key did not include near/far, and
  `ShadowSystem::CalculateCascades` hardcoded a 1.6 aspect ratio while the
  projection used the real viewport ratio.
- Frustum culling across all six geometry passes, each with its own frustum.
  `tests/test_frustum.cpp` was written before the wiring, because the frustum
  code had never actually executed — its only caller was the unreachable
  `SceneGraph`. It turned out to be correct.
- Deleted `SceneGraph` / `SceneNode` once the culling was lifted out.
- Fixed: the Inspector's Position field never set `TransformComponent::dirty`,
  and `RecomputeSubtree` gates the whole subtree on that flag — so dragging a
  parent's position in the Inspector left its children behind. Pre-existing.
- Tests: 130 → 145 green.

# 2026-09-11 — phase 3

- `.mistprefab` landed: separate asset type, name-path addressing, partial
  reflected overrides, propagation on reload, PathGuard from the first commit.
- Hoisted the per-entity component payload into `Scene/ComponentBlocks.h`,
  shared by the scene and prefab serializers. Same reasoning as
  `Core/ReflectionJson.h` one level up: a second copy guarantees drift.
- Editor: save-as-prefab, `.mistprefab` drop, an Inspector block with per-field
  revert, override recording on edit, and undo for instancing and reverting.
- `PackageIO` now walks prefab references and the materials inside them.
- **Fixed the pre-existing order-dependence recorded on 2026-09-11 (phase 1
  part A).** Root cause: `tests/test_path_guard.cpp` repointed the process-wide
  `PathGuard::project_root()` and never restored it, so every test that ran
  afterwards resolved project paths against a temp directory. That is what made
  `tests/test_material_asset.cpp` fail under randomised order, and it broke the
  prefab scene tests the same way until a scoped restorer was added. The suite
  is now order-independent — verified across four seeds.
- Tests: 145 → 160 green.

# 2026-09-11 — phase 4

- All six built-but-dead subsystems wired: `InputSystem`, play mode, shortcut
  dispatch, editor plugin docks, lifecycle signals, shader hot-reload.
- `InputSystem::Init` stopped installing GLFW callbacks and `Update()` polls
  instead. Installing them only coexists with ImGui's backend in one specific
  order, and `InputManager` already documented callback conflicts with ImGui as
  the reason the live path went pure-polling. Trade-off: `GetScrollDelta()` is
  inert without a host-installed `ScrollCallback`.
- The W/E collision is fixed by making viewport navigation modal — the fly
  context is pushed only while RMB is held. **Deliberate behaviour change:**
  WASD alone no longer moves the camera.
- Binding table serialized to `input_map.json`, merged over defaults so a rebind
  survives a restart and a new action still gets its default.
- Lua gained `is_action_pressed`, `is_action_just_pressed`, `get_axis`,
  `get_vector`.
- Fixed: File → New Scene destroyed nothing, so the previous scene kept
  rendering. `Duplicate` gained the undo command it never had.
- Deleted `src/Core/Engine.cpp` + header (registered 3 components / 2 systems
  against `main()`'s 7 / 5 — it would have booted with no hierarchy, lights or
  scripts), both `#if 0` FPS-UI blocks, and the uncalled `DrawCrosshair`.
  `CLAUDE.md` gotcha #2 and `docs/architecture.md` updated accordingly.
- Recorded wire-or-delete recommendations for `EventBus`, `SystemScheduler`,
  `CommandQueue` and the audio stack in the phase-4 document. Not acted on.
- Tests: 160 → 176 green. `InputSystem` and `EditorState` had no tests at all
  before. Order-independent across three seeds.
