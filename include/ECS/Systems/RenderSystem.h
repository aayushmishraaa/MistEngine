#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H

#include "../System.h"
#include "../Coordinator.h"
#include "../../Shader.h"

extern Coordinator gCoordinator;

class RenderSystem : public System {
public:
    // Hide the base's `Update(float)` explicitly. RenderSystem takes a shader
    // rather than deltaTime because it's driven by the Renderer pass, not by
    // the generic scheduler — `using` import silences GCC's
    // -Woverloaded-virtual warning without changing runtime behaviour.
    using System::Update;
    void Update(Shader& shader);
};

#endif // RENDERSYSTEM_H
