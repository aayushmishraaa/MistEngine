> **SUPERSEDED — do not trust this document.**
>
> Written against 0.4.2 and substantially wrong as of 0.5.0. Specifically: it diagrams
> `Core::Engine` as the composition root. That class was never instantiated and has since been
> deleted; the real root is `main()` in `src/MistEngine.cpp`. It documents an `AIManager` and an `HttpClient` that were deleted
> in 0.5.0. It references a plan file by absolute path on a machine that is not this one. And its
> PathGuard table is incomplete: `MaterialSerializer` and `PackageIO` were unguarded until commit
> `04b00b4`.
>
> For current architecture and the Godot-parity model, see [`okf/index.md`](okf/index.md).
> For known defects, see `CODE_AUDIT.md` at the repo root (untracked).

# MistEngine Architecture (post-0.4.2)

This doc captures the module layout after the Phase 1–3 remediation work.
Phase 4's renderer/physics refactor is tracked separately — see the plan at
`/home/it8/.claude/plans/vast-strolling-matsumoto.md` for the target layout.

## Runtime composition

```
                                   ┌────────────────────────┐
                                   │     MistEngine.cpp     │   (entry point)
                                   └───────────┬────────────┘
                                               │
                                   ┌───────────▼────────────┐
                                   │    Core::Engine        │
                                   │  - InputSystem         │
                                   │  - ModuleManager       │
                                   │  - UIManager (ImGui)   │
                                   └───────────┬────────────┘
                                               │
       ┌─────────────────┬─────────────────────┼──────────────────────┬─────────────────┐
       ▼                 ▼                     ▼                      ▼                 ▼
  ┌─────────┐      ┌──────────┐         ┌──────────────┐        ┌─────────┐      ┌───────────┐
  │Renderer │      │ Coordinator│       │ PhysicsSystem │        │ Scene   │      │AIManager  │
  │ (god    │◀─────│ (ECS)      │──────▶│  (Bullet)     │───────▶│         │      │ (optional)│
  │ object) │      └──────────┘         └──────────────┘        └─────────┘      └───────────┘
  └─────────┘
```

The Renderer currently owns GL context, shadow systems, post-process stack,
light manager, particle system, and IBL. Phase 4 splits this into
`RenderBackend` + `GeometryPass` / `ShadowPass` / `PostProcessPass` /
`SkyPass`, each communicating via a shared `DrawList`.

## Security boundaries enforced in 0.4.2

`include/Core/PathGuard.h` is the single source of truth for
user-supplied-path validation. Every surface that accepts a path from a
config file, scene file, or user input funnels through it:

| Site                                         | Sandbox root |
|----------------------------------------------|--------------|
| `SceneSerializer::Load`/`Save`               | `./scenes/`  |
| `GameExporter::ExportGame`                   | `./exports/` |
| `ModuleManager::LoadModule`                  | `./modules/` |
| `AssetBrowser::NavigateTo`/`NavigateUp`      | configured root |

HTTP requests from the AI panel route through `HttpClient::sanitizeUrlForLog`
before any log write, and API keys travel as headers rather than URL params.

## Threading

- **Main/UI thread**: GLFW events, ECS updates, rendering, ImGui.
- **HTTP worker threads** (optional): spawned by `AIManager::SendRequestAsync`
  via `std::async`. They write back into `m_conversationHistory` under
  `m_historyMutex`.
- **Not yet parallel**: ECS system scheduling, physics stepping, asset
  loading. These are single-threaded by design and flagged in the plan for
  a future SystemScheduler overhaul.

## Build matrix

| Platform | Config      | Dependencies                |
|----------|-------------|-----------------------------|
| Linux    | CMake/Ninja | apt packages (see CONTRIBUTING.md) |
| Windows  | CMake/MSBuild | vcpkg manifest (`vcpkg.json`) |
| Tests    | Catch2 via FetchContent | No system install needed |
