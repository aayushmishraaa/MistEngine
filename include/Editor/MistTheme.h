#pragma once
#ifndef MIST_THEME_H
#define MIST_THEME_H

#include "imgui.h"

// Theme system patterned after Godot's `EditorThemeManager`. Every
// stylebox colour is derived from a small set of inputs — base hue,
// accent hue, and a contrast factor — instead of being hand-picked per
// widget. Changing one input re-skins the whole editor consistently,
// which is how Godot's theme presets stay coherent across ~55 widgets.
//
// Usage:
//   MistTheme::Apply(MistTheme::MistDark());
//
// Swapping themes at runtime is safe — callers should call Apply()
// whenever the preset changes. It re-writes the active ImGuiStyle in
// place, which ImGui picks up on the next frame.
namespace Mist::Editor {

struct ThemeParams {
    ImVec4 base     = {0.12f, 0.13f, 0.15f, 1.0f}; // dark slate
    ImVec4 accent   = {0.26f, 0.59f, 0.98f, 1.0f}; // Godot blue
    float  contrast = 0.25f;                       // 0 = flat, 1 = high
    float  rounding = 2.0f;                        // corners on frames/tabs
};

struct MistTheme {
    static ThemeParams MistDark();
    static ThemeParams GodotBlue();
    static ThemeParams Monochrome();

    // Apply a theme preset to the active ImGui context. Overwrites
    // every ImGuiCol_* entry in the style colour table.
    static void Apply(const ThemeParams& params);
};

} // namespace Mist::Editor

#endif // MIST_THEME_H
