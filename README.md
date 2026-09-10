# MistEngine 0.5.0

[![Version](https://img.shields.io/badge/version-0.5.0--prealpha-brightgreen.svg)](https://github.com/aayushmishraaa/MistEngine)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-blue.svg)](https://github.com/aayushmishraaa/MistEngine)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://github.com/aayushmishraaa/MistEngine)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

> A C++17 OpenGL 4.6 engine — editor and renderer sandbox.

MistEngine is a 3D engine built around an Entity-Component-System core, a PBR forward renderer with
cascaded shadow maps and a screen-space post-process chain, Bullet physics, a Dear ImGui editor with
dockable panels and transform gizmos, Lua scripting via sol2, and hot-loadable plugin modules. It
builds from one CMake tree on Linux and Windows.

**Pre-alpha.** This is a working sandbox, not a shipping engine. See
[Known limitations](#known-limitations) before starting anything that depends on it.

---

## Platform support

| Platform | Builds | Tests | Editor runs |
|---|---|---|---|
| Linux (x86_64) | yes | yes | yes |
| Windows (x64, vcpkg) | yes | yes | yes |
| macOS (Apple Silicon / Intel) | yes | yes | **no** |

macOS builds the library and runs the full headless test suite, but **cannot run the editor**.
Apple's OpenGL implementation is deprecated and frozen at 4.1, while the renderer depends on
OpenGL 4.3–4.6 features throughout: direct-state-access entry points (~220 call sites), compute
shaders (clustered light culling, Hi-Z reduction, SSR, GPU particles), shader storage buffers, image
load/store, and `#version 460 core` in all 51 shaders. Supporting it would mean a second,
GL 4.1-compatible backend — or a Vulkan backend behind `RenderingDevice` running on MoltenVK. Neither
exists yet. Develop the renderer on Linux or Windows; use the macOS preset for tests.

---

## Building

### Dependencies

OpenGL, GLFW 3, GLM, Assimp, Bullet 3, Freetype. Fetched automatically via CMake `FetchContent`:
Dear ImGui (docking branch), ImGuizmo, nlohmann/json, Lua 5.4, sol2, Catch2. Every dependency is
pinned to an exact tag or SHA.

```bash
# Linux
sudo apt-get install ninja-build libglfw3-dev libglm-dev libassimp-dev \
                     libbullet-dev libfreetype-dev
cmake --preset linux-release && cmake --build --preset linux-release -j
./build-linux-release/MistEngine

# macOS (library + tests only)
brew install cmake ninja glfw glm assimp bullet freetype
cmake --preset mac-debug && cmake --build --preset mac-debug -j
ctest --preset mac-debug --output-on-failure

# Windows (needs VCPKG_ROOT)
cmake --preset windows-release && cmake --build --preset windows-release --config Release
```

### Presets

| Preset | Notes |
|---|---|
| `linux-debug` | Debug + AddressSanitizer + UBSan |
| `linux-release` | Optimised |
| `linux-ci` | Debug, no sanitizers — what CI runs |
| `linux-runtime` | `MIST_BUILD_EDITOR=OFF`, headless runtime |
| `mac-debug` | Library + tests; editor will not run |
| `windows-release` | Visual Studio 17 2022, vcpkg toolchain |

### Options

`MIST_BUILD_EDITOR` (ON) · `MIST_ENABLE_SCRIPTING` (ON) · `MIST_ENABLE_TESTS` (ON) ·
`MIST_ENABLE_AUDIO` (OFF) · `MIST_ASAN` (OFF) · `MIST_TEST_GL` (OFF)

Run the test suite with `ctest --preset <preset>`. It is headless by design — no window, no GL
context, no network.

---

## What's actually in here

### Rendering

- Forward PBR (Cook-Torrance GGX), HDR pipeline, AgX / ACES / Reinhard tonemapping
- Cascaded shadow maps — 4 cascades at 2048², PCSS soft shadows with separated blocker bias
- Omni shadows — cubemap array, up to 4 shadow-casting point lights
- Clustered lighting — 16×9×24 grid, compute-shader culling, 1024-light SSBO
- Post-process chain: bloom, SSAO, TAA (Halton jitter + variance clipping), SSR over a Hi-Z
  pyramid, bokeh depth of field, motion blur, FXAA
- Depth prepass writing octahedral-packed normals and roughness
- Procedural and atmospheric skyboxes

### Engine

- ECS with signature-based system matching and a flat scene-graph component
- Bullet physics with RAII ownership wrappers, fixed 60 Hz timestep, shape-hash body rebuild
- Lua scripting (sol2) with per-script sandboxed environments — spawn, transform, physics, lighting,
  animation and raycast bindings
- Reflection (`MIST_REFLECT`) driving the Inspector, scene JSON and `.mistmat` materials from one
  declaration
- Assimp scene import (`.obj` / `.fbx` / `.gltf` / `.glb`) with PBR texture resolution
- `.mist` scene format and `.mistpkg` single-file archives
- Hot-loadable plugin modules (`.so` / `.dll` / `.dylib`) behind a sandboxed loader

### Editor

- Dockable ImGui layout with persisted arrangement, three themes
- Hierarchy with drag-drop reparenting, reflection-driven Inspector, asset browser
- ImGuizmo translate/rotate/scale handles with snapping
- Undo/redo with Godot-style merge semantics (slider drags collapse to one step)
- Console doubling as a Lua REPL
- Profiler with non-blocking GPU timer queries

---

## Controls

| Key | Action |
|---|---|
| `WASD` / `QE` | Fly camera |
| Right-drag | Mouse look |
| Middle-drag | Orbit · `Shift` + middle-drag to pan |
| Scroll | Zoom |
| `Numpad 1/3/7` | Front / right / top view |
| `Numpad 5` | Toggle orbit mode · `Numpad 0` resets |
| `F` | Focus selection |
| `W` / `E` / `R` | Translate / rotate / scale gizmo |
| `Ctrl+Z` / `Ctrl+Y` | Undo / redo |
| `F1` | ImGui demo window |
| `Esc` | Quit |

---

## Known limitations

Honest list, because several of these look like they work:

- **Skeletal animation renders a static bind pose.** Rigs import and clips populate the Inspector,
  but the bone hierarchy walk is unimplemented and the inverse bind matrix is never applied.
- **Image-based lighting is not wired up.** `IBL::ProcessEnvironmentMap` has no caller and its
  cubemap convolution draws without a bound VAO. Ambient falls back to a constant.
- **GPU particles allocate but never draw.** No render program is loaded and the indirect draw count
  is never written.
- **Several subsystems are unreachable from `main()`** — the action-mapping `InputSystem`, the
  `EventBus`, `SystemScheduler`, the audio stack, and the `.import` asset pipeline. They compile and
  are unit-tested; they are not connected.
- **Keyboard shortcuts are display-only** apart from undo/redo and the gizmo keys. Menu chords render
  from `ShortcutRegistry` but nothing dispatches them.
- **Play / Pause / Stop change editor state only.** No scene snapshot or restore.
- **The backend abstraction is cosmetic.** `RenderingDevice` exists, but consumers cast to
  `GLRenderingDevice` to reach the raw handle, so it is not yet a seam a second backend could use.
- **`GameExporter` is vestigial**, still shaped around the FPS demo removed in 0.5.0.

---

## Project layout

```
MistEngine/
├── include/            Public headers (ECS, Renderer, Editor, Core, Script, …)
├── src/                Implementation
├── shaders/            51 GLSL shaders (#version 460 core)
├── scripts/            Lua — bootstrap.lua is the opening scene
├── tests/              Catch2 suite, headless
├── modules/            Example hot-loadable plugin
├── docs/               Architecture and user guides
└── CMakeLists.txt
```

The editor's opening scene is authored in `scripts/showcase.lua` — edit that, not C++, to change
what you see on launch.

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Enable the pre-commit hook once per clone:

```bash
git config core.hooksPath .githooks
```

It runs `clang-format` over staged sources and rejects trailing whitespace.

---

## License

MIT — see [LICENSE](LICENSE).

## Acknowledgments

Bullet Physics · GLFW · GLM · Dear ImGui · ImGuizmo · Assimp · Lua & sol2 · nlohmann/json · Catch2 ·
stb_image · glad
