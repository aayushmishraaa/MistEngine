#include "Editor/MistTheme.h"

#include <algorithm>

namespace Mist::Editor {

namespace {

// Lerp from `c` toward black (factor > 0) or toward white
// (factor < 0). Matches Godot's `base_color.lerp(Color(0,0,0), f)`
// idiom — one direction from a single input gives us the entire
// shadow/highlight family off a single base colour.
ImVec4 darken(const ImVec4& c, float factor) {
    factor = std::clamp(factor, -1.0f, 1.0f);
    if (factor >= 0.0f) {
        return ImVec4(c.x * (1.0f - factor),
                      c.y * (1.0f - factor),
                      c.z * (1.0f - factor),
                      c.w);
    }
    float k = -factor;
    return ImVec4(c.x + (1.0f - c.x) * k,
                  c.y + (1.0f - c.y) * k,
                  c.z + (1.0f - c.z) * k,
                  c.w);
}

ImVec4 withAlpha(const ImVec4& c, float a) {
    return ImVec4(c.x, c.y, c.z, a);
}

} // namespace

ThemeParams MistTheme::MistDark() {
    return {};  // defaults match
}

ThemeParams MistTheme::GodotBlue() {
    ThemeParams p;
    p.base     = {0.11f, 0.13f, 0.16f, 1.0f};
    p.accent   = {0.23f, 0.65f, 1.00f, 1.0f};
    p.contrast = 0.28f;
    return p;
}

ThemeParams MistTheme::Monochrome() {
    ThemeParams p;
    p.base     = {0.15f, 0.15f, 0.15f, 1.0f};
    p.accent   = {0.55f, 0.55f, 0.55f, 1.0f};
    p.contrast = 0.22f;
    return p;
}

void MistTheme::Apply(const ThemeParams& p) {
    ImGuiStyle& s = ImGui::GetStyle();

    // Structural sizing — shared across presets so widgets feel the
    // same regardless of colour palette.
    s.WindowRounding    = 0.0f;
    s.FrameRounding     = p.rounding;
    s.ScrollbarRounding = p.rounding;
    s.TabRounding       = p.rounding;
    s.PopupRounding     = p.rounding;
    s.GrabRounding      = p.rounding;
    s.WindowBorderSize  = 1.0f;
    s.FrameBorderSize   = 0.0f;
    s.WindowPadding     = ImVec2(8, 8);
    s.FramePadding      = ImVec2(6, 4);
    s.ItemSpacing       = ImVec2(6, 4);
    s.IndentSpacing     = 16.0f;

    const ImVec4& base   = p.base;
    const ImVec4& accent = p.accent;
    const float   k      = p.contrast;

    // Derive the neighbourhood around `base`. `bgDeep` is the darkest
    // surface (titlebars, scrollbar backgrounds); `bgLight` is the
    // slightly-raised area (hovered buttons, selected rows).
    ImVec4 bgDeep  = darken(base,  k);
    ImVec4 bgLight = darken(base, -k * 0.5f);
    ImVec4 text    = ImVec4(0.90f, 0.91f, 0.93f, 1.0f);
    ImVec4 textDim = ImVec4(0.55f, 0.56f, 0.58f, 1.0f);
    ImVec4 border  = darken(base, k * 0.8f);

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text]                 = text;
    c[ImGuiCol_TextDisabled]         = textDim;
    c[ImGuiCol_WindowBg]             = base;
    c[ImGuiCol_ChildBg]              = base;
    c[ImGuiCol_PopupBg]              = bgDeep;
    c[ImGuiCol_Border]               = border;
    c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);

    c[ImGuiCol_FrameBg]              = bgDeep;
    c[ImGuiCol_FrameBgHovered]       = darken(bgDeep, -k * 0.6f);
    c[ImGuiCol_FrameBgActive]        = withAlpha(accent, 0.55f);

    c[ImGuiCol_TitleBg]              = bgDeep;
    c[ImGuiCol_TitleBgActive]        = bgDeep;
    c[ImGuiCol_TitleBgCollapsed]     = bgDeep;
    c[ImGuiCol_MenuBarBg]            = bgDeep;

    c[ImGuiCol_ScrollbarBg]          = bgDeep;
    c[ImGuiCol_ScrollbarGrab]        = bgLight;
    c[ImGuiCol_ScrollbarGrabHovered] = darken(bgLight, -k * 0.5f);
    c[ImGuiCol_ScrollbarGrabActive]  = accent;

    c[ImGuiCol_CheckMark]            = accent;
    c[ImGuiCol_SliderGrab]           = accent;
    c[ImGuiCol_SliderGrabActive]     = darken(accent, -k * 0.3f);

    c[ImGuiCol_Button]               = bgLight;
    c[ImGuiCol_ButtonHovered]        = darken(bgLight, -k * 0.5f);
    c[ImGuiCol_ButtonActive]         = accent;

    c[ImGuiCol_Header]               = withAlpha(accent, 0.35f);
    c[ImGuiCol_HeaderHovered]        = withAlpha(accent, 0.55f);
    c[ImGuiCol_HeaderActive]         = withAlpha(accent, 0.75f);

    c[ImGuiCol_Separator]            = border;
    c[ImGuiCol_SeparatorHovered]     = accent;
    c[ImGuiCol_SeparatorActive]      = accent;

    c[ImGuiCol_ResizeGrip]           = withAlpha(accent, 0.20f);
    c[ImGuiCol_ResizeGripHovered]    = withAlpha(accent, 0.55f);
    c[ImGuiCol_ResizeGripActive]     = accent;

    c[ImGuiCol_Tab]                  = bgDeep;
    c[ImGuiCol_TabHovered]           = withAlpha(accent, 0.55f);
    // 1.92-era ImGui renamed the "active tab" entry; use the present
    // name and fall back only if the header names it differently
    // in future upgrades. The enum is stable enough that this assign
    // is safe today.
    c[ImGuiCol_TabSelected]          = accent;
    c[ImGuiCol_TabDimmed]            = bgDeep;
    c[ImGuiCol_TabDimmedSelected]    = withAlpha(accent, 0.5f);

    c[ImGuiCol_PlotLines]            = accent;
    c[ImGuiCol_PlotLinesHovered]     = darken(accent, -k * 0.4f);
    c[ImGuiCol_PlotHistogram]        = accent;
    c[ImGuiCol_PlotHistogramHovered] = darken(accent, -k * 0.4f);

    c[ImGuiCol_TableHeaderBg]        = bgDeep;
    c[ImGuiCol_TableBorderStrong]    = border;
    c[ImGuiCol_TableBorderLight]     = darken(base, k * 0.4f);
    c[ImGuiCol_TableRowBg]           = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_TableRowBgAlt]        = withAlpha(bgLight, 0.30f);

    c[ImGuiCol_TextSelectedBg]       = withAlpha(accent, 0.35f);
    c[ImGuiCol_DragDropTarget]       = accent;
    c[ImGuiCol_NavCursor]            = accent;
    c[ImGuiCol_NavWindowingHighlight]= withAlpha(accent, 0.7f);
    c[ImGuiCol_NavWindowingDimBg]    = ImVec4(0, 0, 0, 0.4f);
    c[ImGuiCol_ModalWindowDimBg]     = ImVec4(0, 0, 0, 0.45f);
}

} // namespace Mist::Editor
