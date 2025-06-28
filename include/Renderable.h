
#ifndef RENDERABLE_H
#define RENDERABLE_H

#include "Shader.h"

class Renderable {
public:
    virtual ~Renderable() {}
    virtual void Draw(Shader& shader) = 0; // Pure virtual function
};

#endif