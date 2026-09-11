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
