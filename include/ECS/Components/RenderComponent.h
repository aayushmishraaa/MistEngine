#ifndef RENDERCOMPONENT_H
#define RENDERCOMPONENT_H

#include "Renderable.h"

struct RenderComponent {
    Renderable* renderable;
    bool visible = true;
};

#endif // RENDERCOMPONENT_H
