#include "Editor/EditorState.h"
#include "Editor/UndoRedo.h"
#include "Editor/GizmoSystem.h"
#include "Editor/AssetBrowser.h"
#include "Debug/Profiler.h"
#include "Debug/ConsoleSystem.h"
#include "Debug/DebugDraw.h"
#include "PostProcessStack.h"
#include "ShadowSystem.h"
#include "LightManager.h"
#include "SkyboxRenderer.h"
#include "ParticleSystem.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace EditorPanels {

void RenderPlayControls(EditorState& state) {
    ImGui::BeginMainMenuBar();
    float centerX = ImGui::GetWindowWidth() * 0.5f;
    ImGui::SetCursorPosX(centerX - 60);

    if (state.IsEditing()) {
        if (ImGui::Button("Play")) state.Play();
    } else {
        if (ImGui::Button("Stop")) state.Stop();
        ImGui::SameLine();
        if (state.IsPlaying()) {
            if (ImGui::Button("Pause")) state.Pause();
        } else {
            if (ImGui::Button("Resume")) state.Play();
        }
    }
    ImGui::EndMainMenuBar();
}

void RenderPostProcessControls(PostProcessStack& postProcess, float& exposure) {
    if (!ImGui::CollapsingHeader("Post-Processing")) return;

    ImGui::SliderFloat("Exposure", &exposure, 0.1f, 10.0f);
    ImGui::Checkbox("Bloom", &postProcess.enableBloom);
    if (postProcess.enableBloom) {
        ImGui::SliderFloat("Bloom Threshold", &postProcess.bloom.threshold, 0.0f, 5.0f);
        ImGui::SliderFloat("Bloom Intensity", &postProcess.bloom.intensity, 0.0f, 3.0f);
    }
    ImGui::Checkbox("SSAO", &postProcess.enableSSAO);
    if (postProcess.enableSSAO) {
        ImGui::SliderFloat("SSAO Radius", &postProcess.ssao.radius, 0.1f, 5.0f);
        ImGui::SliderFloat("SSAO Bias", &postProcess.ssao.bias, 0.001f, 0.1f);
    }
    ImGui::Checkbox("FXAA", &postProcess.enableFXAA);
}

void RenderShadowControls(ShadowSystem& shadows) {
    if (!ImGui::CollapsingHeader("Shadows")) return;

    ImGui::Checkbox("Debug Cascade Colors", &shadows.showCascadeColors);
}

void RenderLightEditor(LightManager& lights) {
    if (!ImGui::CollapsingHeader("Lights")) return;

    ImGui::Text("Active lights: %d", lights.GetLightCount());

    if (ImGui::Button("Add Point Light")) {
        Light light;
        light.position = glm::vec4(0, 5, 0, 1.0f);
        light.color = glm::vec4(1, 1, 1, 5.0f);
        light.params = glm::vec4(20.0f, 0, -1, 0);
        lights.AddLight(light);
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Spot Light")) {
        Light light;
        light.position = glm::vec4(0, 5, 0, 2.0f);
        light.direction = glm::vec4(0, -1, 0, glm::cos(glm::radians(25.0f)));
        light.color = glm::vec4(1, 1, 1, 10.0f);
        light.params = glm::vec4(30.0f, glm::cos(glm::radians(35.0f)), -1, 0);
        lights.AddLight(light);
    }

    for (int i = 0; i < lights.GetLightCount(); i++) {
        ImGui::PushID(i);
        Light& l = lights.GetLight(i);
        if (ImGui::TreeNode("Light", "Light %d", i)) {
            ImGui::DragFloat3("Position", glm::value_ptr(l.position), 0.1f);
            ImGui::ColorEdit3("Color", glm::value_ptr(l.color));
            ImGui::DragFloat("Intensity", &l.color.w, 0.1f, 0.0f, 100.0f);
            ImGui::DragFloat("Range", &l.params.x, 0.1f, 0.1f, 200.0f);

            if (ImGui::Button("Remove")) {
                lights.RemoveLight(i);
                ImGui::TreePop();
                ImGui::PopID();
                break;
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

void RenderSkyboxControls(SkyboxRenderer& skybox) {
    if (!ImGui::CollapsingHeader("Skybox")) return;

    const char* modes[] = {"Procedural", "HDR Cubemap", "Atmospheric"};
    int currentMode = static_cast<int>(skybox.GetMode());
    if (ImGui::Combo("Mode", &currentMode, modes, 3)) {
        skybox.SetMode(static_cast<SkyboxMode>(currentMode));
    }

    if (skybox.GetMode() == SkyboxMode::Atmospheric) {
        ImGui::DragFloat3("Sun Direction", glm::value_ptr(skybox.sunDirection), 0.01f, -1.0f, 1.0f);
        skybox.sunDirection = glm::normalize(skybox.sunDirection);
    }
}

void RenderProfilerWindow(Profiler& profiler) {
    if (!ImGui::Begin("Profiler")) { ImGui::End(); return; }

    ImGui::Text("FPS: %.1f (%.2f ms)", profiler.GetFPS(), profiler.GetFrameTimeMs());
    ImGui::Text("Draw Calls: %d", profiler.GetDrawCalls());
    ImGui::Text("Triangles: %d", profiler.GetTriangles());

    ImGui::PlotLines("FPS", profiler.GetFPSHistory(), profiler.GetFPSHistorySize(),
        profiler.GetFPSHistoryOffset(), nullptr, 0.0f, 120.0f, ImVec2(0, 60));

    ImGui::Separator();
    if (ImGui::BeginTable("Sections", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Section");
        ImGui::TableSetupColumn("CPU (ms)");
        ImGui::TableSetupColumn("GPU (ms)");
        ImGui::TableHeadersRow();

        for (auto& s : profiler.GetSections()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%s", s.name.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%.2f", s.cpuTimeMs);
            ImGui::TableNextColumn(); ImGui::Text("%.2f", s.gpuTimeMs);
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void RenderConsoleWindow(ConsoleSystem& console) {
    if (!ImGui::Begin("Console")) { ImGui::End(); return; }

    ImGui::BeginChild("ScrollRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false);
    for (auto& line : console.GetHistory()) {
        ImGui::TextUnformatted(line.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    static char inputBuf[256] = "";
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
    if (ImGui::InputText("##consoleinput", inputBuf, sizeof(inputBuf), flags)) {
        if (inputBuf[0] != '\0') {
            console.Execute(inputBuf);
            inputBuf[0] = '\0';
        }
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
}

void RenderAssetBrowserWindow(AssetBrowser& browser) {
    if (!ImGui::Begin("Asset Browser")) { ImGui::End(); return; }

    if (browser.CanNavigateUp()) {
        if (ImGui::Button("<- Back")) browser.NavigateUp();
        ImGui::SameLine();
    }
    ImGui::Text("Path: %s", browser.GetRelativePath().c_str());
    ImGui::Separator();

    static char filterBuf[128] = "";
    ImGui::InputText("Filter", filterBuf, sizeof(filterBuf));
    browser.SetFilter(filterBuf);

    auto entries = browser.GetFilteredEntries();
    for (auto& entry : entries) {
        if (entry.isDirectory) {
            if (ImGui::Selectable((std::string("[DIR] ") + entry.name).c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                if (ImGui::IsMouseDoubleClicked(0)) {
                    browser.NavigateTo(entry.fullPath);
                }
            }
        } else {
            if (ImGui::Selectable(entry.name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                if (browser.onAssetSelected) browser.onAssetSelected(entry);
                if (ImGui::IsMouseDoubleClicked(0) && browser.onAssetDoubleClicked) {
                    browser.onAssetDoubleClicked(entry);
                }
            }
        }
    }

    ImGui::End();
}

void RenderGizmoControls(GizmoSystem& gizmo) {
    ImGui::Text("Gizmo: ");
    ImGui::SameLine();
    if (ImGui::RadioButton("Translate", gizmo.GetMode() == GizmoMode::Translate))
        gizmo.SetMode(GizmoMode::Translate);
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", gizmo.GetMode() == GizmoMode::Rotate))
        gizmo.SetMode(GizmoMode::Rotate);
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", gizmo.GetMode() == GizmoMode::Scale))
        gizmo.SetMode(GizmoMode::Scale);

    bool snap = gizmo.IsSnapEnabled();
    if (ImGui::Checkbox("Snap", &snap)) gizmo.SetSnap(snap, gizmo.GetSnapValue());
    if (snap) {
        float snapVal = gizmo.GetSnapValue();
        ImGui::SameLine();
        if (ImGui::DragFloat("##snapval", &snapVal, 0.1f, 0.1f, 10.0f))
            gizmo.SetSnap(true, snapVal);
    }
}

} // namespace EditorPanels
