#pragma once
#ifndef MIST_VIEWPORT_H
#define MIST_VIEWPORT_H

#include <glad/glad.h>

// Viewport — first-class render target description.
//
// Purpose: before G2, MistEngine had exactly one rendering destination, the
// HDR texture plumbed into an ImGui panel. When the panel was closed, the
// rendered scene had nowhere to appear and the user saw ImGui's raw
// background — the "blue screen" we diagnosed during FPS-mode debugging.
//
// Viewport names the concept so the same render pipeline can target either:
//   - the editor's Scene View panel (texture handed to UIManager)
//   - the default framebuffer fullscreen (blit at present time)
//   - future: render-to-texture for reflection probes, minimap, etc.
//
// This first-landed version is intentionally thin. It carries dimensions
// and the output texture handle; the per-viewport PostProcessStack,
// ShadowSystem, and Camera still live on Renderer. The shape is enough to
// fix the blue-screen bug today and gives us a target to migrate that
// state into in a later cycle.
struct Viewport {
    int    width         = 0;
    int    height        = 0;
    GLuint outputTexture = 0;   // final post-tonemap result, ready to sample
    bool   presentFullscreen = false; // blit directly to default FB if true
};

#endif // MIST_VIEWPORT_H
