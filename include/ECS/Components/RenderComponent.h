#ifndef RENDERCOMPONENT_H
#define RENDERCOMPONENT_H

#include "Core/Reflection.h"
#include "Renderable.h"

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
};

MIST_REFLECT(RenderComponent)
    MIST_FIELD(RenderComponent, visible,      ::Mist::PropertyHint::None,        "")
    MIST_FIELD(RenderComponent, materialPath, ::Mist::PropertyHint::ResourceRef, "Material")
MIST_REFLECT_END(RenderComponent)

#endif // RENDERCOMPONENT_H
