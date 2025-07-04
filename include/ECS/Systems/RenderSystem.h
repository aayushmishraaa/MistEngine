#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H

#include "../System.h"
#include "../Coordinator.h"
#include "../../Shader.h"

extern Coordinator gCoordinator;

class RenderSystem : public System {
public:
    void Update(Shader& shader);
};

#endif // RENDERSYSTEM_H
