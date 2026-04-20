#ifndef LIGHTSYSTEM_H
#define LIGHTSYSTEM_H

#include "../System.h"
#include "../Coordinator.h"

class LightManager;

// Syncs ECS light entities into LightManager each frame.
//
// Operates on {Transform + LightComponent}. Called from Renderer
// BEFORE `LightManager::UploadToGPU` / `CullLights`. Each frame it
// clears the LightManager's array and re-pushes every light
// entity's translated state. Godot does the same (RenderingServer
// rebuilds the light list from the scene tree per-frame).
class LightSystem : public System {
public:
    using System::Update;
    void Update(Coordinator& coord, LightManager& lights);
};

#endif // LIGHTSYSTEM_H
