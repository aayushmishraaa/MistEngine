#ifndef RENDERCOMPONENT_H
#define RENDERCOMPONENT_H

#include "Core/Reflection.h"
#include "Renderable.h"

struct RenderComponent {
    Renderable* renderable = nullptr;
    bool visible = true;
};

MIST_REFLECT(RenderComponent)
    MIST_FIELD(RenderComponent, visible, ::Mist::PropertyHint::None, "")
MIST_REFLECT_END(RenderComponent)

#endif // RENDERCOMPONENT_H
