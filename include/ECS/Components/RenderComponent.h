#ifndef RENDERCOMPONENT_H
#define RENDERCOMPONENT_H

#include "Core/Reflection.h"
#include "Renderable.h"
#include "Scene/AABB.h"

#include <string>

// Per-entity render state. `renderable` is a non-owning pointer into
// the AssetRegistry / importer stash (lifetime managed outside the
// ECS). `materialPath` is an optional override — when non-empty the
// renderer resolves it via the `.mistmat` loader and uses that
// material instead of the mesh's embedded PBRMaterial.
struct RenderComponent {
    Renderable*  renderable   = nullptr;
    bool         visible      = true;
    std::string  materialPath;    // "" = use the mesh's inline material

    // Where `renderable` came from, so the scene serializer can round-trip it.
    // Either a `builtin://cube|plane|sphere` primitive or an on-disk model
    // path. Empty means "not tracked" — the renderer ignores this field
    // entirely; it exists purely so saving a scene doesn't lose mesh identity.
    //
    // Without it, SceneSerializer::mesh_ref_for had no way to recover what a
    // `Renderable*` pointed at and unconditionally emitted `builtin://cube`,
    // so every plane, sphere and imported mesh came back as a cube on load.
    std::string  meshPath;

    // World-space bounds, recomputed each frame by RenderSystem::UpdateBounds
    // from the renderable's object-space bounds and the entity's world matrix.
    //
    // Derived state, so deliberately NOT reflected: it must never be written
    // to a scene file or shown in the Inspector. An invalid box means "this
    // renderable could not describe its extent", and every cull site treats
    // that as "always draw".
    AABB worldBounds;
};

MIST_REFLECT(RenderComponent)
    MIST_FIELD(RenderComponent, visible,      ::Mist::PropertyHint::None,        "")
    MIST_FIELD(RenderComponent, materialPath, ::Mist::PropertyHint::ResourceRef, "Material")
    MIST_FIELD(RenderComponent, meshPath,     ::Mist::PropertyHint::ResourceRef, "Mesh")
MIST_REFLECT_END(RenderComponent)

#endif // RENDERCOMPONENT_H
