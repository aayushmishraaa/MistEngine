---
type: Subsystem Parity
title: Resources and assets
description: Godot's Resource is a refcounted, path-cached, serializable base class with an import pipeline; MistEngine has a capable cache with one loader installed out of four.
tags: [parity, godot, resources, assets, serialization]
status: draft
generated:
  by: claude-opus-5/1m
  at: 2026-09-11T00:00:00Z
sources:
  - resource: https://docs.godotengine.org/en/stable/tutorials/scripting/resources.html
    title: Resources — Godot docs
---

# Godot model

`Resource` is a data container — distinct from `Node`, which *does* things. Both derive from
`Object`. The properties that matter:

- **Path-cached and shared.** "When the engine loads a resource from disk it only loads it once";
  later requests return the same object.
- **Reference counted**, so lifetime is automatic.
- **Serializable for free** to `.tres` (text) or `.res` (binary), including nested sub-resources.
- **External or built-in** — the same resource is a standalone file or embedded inside a `.tscn`,
  and removing its path converts one to the other.
- **Inspector support for free**: a custom `Resource` subclass with exported properties is editable
  with no custom UI code.
- Custom types register via `class_name` (GDScript) or `[GlobalClass]` (C#).

Scenes themselves are resources — `PackedScene` — which is what makes
[instancing](/subsystems/scene-and-composition.md) fall out of the same system.

# MistEngine today

`ResourceManager<T>` is genuinely well built: path-keyed deduplication, a handle type with a
generation counter, thread-safe load with the loader invoked *outside* the mutex so concurrent loads
do not serialise, and a `ReloadAll` that bumps a generation.

What is missing is that almost nothing is plugged into it.

`AssetRegistry` owns four managers — meshes, textures, audio, shaders — and installs exactly **one
loader**, for meshes. That loader recognises three `builtin://` primitives and returns `nullptr` for
everything else, with a `TODO` about an on-disk mesh format. So `textures().Load(path)` always logs
an error and returns an invalid handle; the texture cache is inert.

Materials are the one real asset type: `.mistmat` round-trips through
`MaterialSerializer`, reflection-driven, so adding a field to `PBRMaterial` serialises for free. That
is the pattern the rest of the engine's resources should follow.

`.mistpkg` bundles a scene plus its dependencies as base64 blobs in one JSON file. As of commit
`04b00b4` it is no longer an arbitrary-file-write vector and its extraction target is inside the
scene sandbox, so import can actually succeed.

# Delta

- **No refcounting semantics.** `ResourceManager::Release` is manual and nothing calls it; loaded
  assets live until process exit. The `Ref<T>` wrapper is explicitly documented as non-owning.
- **No generic serializable resource.** There is no `.tres` equivalent — `PBRMaterial` has bespoke
  save/load rather than being an instance of a general mechanism. A second resource type means
  writing a second serializer.
- **No resource UID**, so renaming an asset breaks every reference to it.
- **No import pipeline.** `IAssetImporter` / `ImporterRegistry` / `PassThroughImporter` implement
  Godot's `.import`-sidecar pattern completely — and are referenced only from
  `tests/test_importer.cpp`. Nothing registers an importer at startup.
- **Async loading is unsound for GPU assets.** `ResourceManager::LoadAsync` runs the loader on a
  `std::async` worker, and the mesh loader constructs a `Mesh`, which issues GL calls. There is no GL
  context on that thread. It has no non-test callers, which is the only reason this has not crashed.

# Evidence

- `src/Resources/AssetRegistry.cpp:49` — `m_Meshes.SetLoader(...)` is the only `SetLoader` call.
- `src/Resources/AssetRegistry.cpp` `defaultMeshLoader` — three `builtin://` names, `nullptr`
  otherwise.
- `include/Resources/ResourceManager.h` — `LoadAsync` via `std::async(std::launch::async, ...)`.
- `include/Resources/Ref.h` — "Non-owning because eviction semantics come later".
- `grep -rn "ImporterRegistry" src tests` → `tests/test_importer.cpp` only.
- `src/Assets/MaterialSerializer.cpp` — reflection-driven `.mistmat`, and the one asset type that
  works end to end.

# Depends on / Blocks

Blocks: any second asset type, asset renaming, texture caching.
Not scheduled — the roadmap deliberately prioritises the scene model first, since a prefab is the
resource type most worth having and it can be built on the existing JSON + reflection pattern.
