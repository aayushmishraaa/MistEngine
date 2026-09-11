
#ifndef RENDERABLE_H
#define RENDERABLE_H

#include "Scene/AABB.h"
#include "Shader.h"

class Renderable {
public:
    virtual ~Renderable() {}
    virtual void Draw(Shader& shader) = 0; // Pure virtual function

    // Object-space bounds, for frustum culling.
    //
    // Returns false when this renderable cannot describe its own extent. The
    // default is false on purpose: a renderable with unknown bounds must be
    // drawn unconditionally. Culling something because nobody computed its
    // bounds is a far worse failure than not culling it — the first makes
    // geometry vanish, the second costs a draw call.
    virtual bool GetLocalBounds(AABB& out) const { (void)out; return false; }
};

#endif