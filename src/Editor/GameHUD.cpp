#include <imgui.h>
#include <string>

namespace GameHUD {

struct HUDState {
    int health = 100;
    int maxHealth = 100;
    int ammo = 30;
    int maxAmmo = 30;
    int reserveAmmo = 90;
    int score = 0;
    int wave = 1;
    float crosshairSize = 20.0f;
    bool showDamageIndicator = false;
    float damageIndicatorTimer = 0.0f;
    std::string weaponName = "Pistol";
    bool isReloading = false;
    float reloadProgress = 0.0f;
};

void RenderCrosshair(float screenW, float screenH, float size) {
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    float cx = screenW * 0.5f;
    float cy = screenH * 0.5f;
    float half = size * 0.5f;
    ImU32 white = IM_COL32(255, 255, 255, 200);
    float gap = 4.0f;

    draw->AddLine(ImVec2(cx - half, cy), ImVec2(cx - gap, cy), white, 2.0f);
    draw->AddLine(ImVec2(cx + gap, cy), ImVec2(cx + half, cy), white, 2.0f);
    draw->AddLine(ImVec2(cx, cy - half), ImVec2(cx, cy - gap), white, 2.0f);
    draw->AddLine(ImVec2(cx, cy + gap), ImVec2(cx, cy + half), white, 2.0f);
}

void RenderHealthBar(const HUDState& state) {
    ImGui::SetNextWindowPos(ImVec2(20, ImGui::GetIO().DisplaySize.y - 80));
    ImGui::SetNextWindowSize(ImVec2(250, 60));
    ImGui::Begin("##health", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBackground);

    float healthPct = static_cast<float>(state.health) / static_cast<float>(state.maxHealth);
    ImVec4 barColor = healthPct > 0.5f ? ImVec4(0.2f, 0.8f, 0.2f, 0.9f)
                    : healthPct > 0.25f ? ImVec4(0.9f, 0.7f, 0.1f, 0.9f)
                    : ImVec4(0.9f, 0.1f, 0.1f, 0.9f);

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
    ImGui::ProgressBar(healthPct, ImVec2(200, 20),
        (std::to_string(state.health) + "/" + std::to_string(state.maxHealth)).c_str());
    ImGui::PopStyleColor();
    ImGui::Text("HP");
    ImGui::End();
}

void RenderAmmoDisplay(const HUDState& state) {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 220, ImGui::GetIO().DisplaySize.y - 80));
    ImGui::SetNextWindowSize(ImVec2(200, 60));
    ImGui::Begin("##ammo", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBackground);

    ImGui::Text("%s", state.weaponName.c_str());
    ImGui::Text("%d / %d  [%d]", state.ammo, state.maxAmmo, state.reserveAmmo);

    if (state.isReloading) {
        ImGui::ProgressBar(state.reloadProgress, ImVec2(180, 10), "Reloading...");
    }

    ImGui::End();
}

void RenderScoreWave(const HUDState& state) {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 200, 20));
    ImGui::SetNextWindowSize(ImVec2(180, 50));
    ImGui::Begin("##score", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBackground);

    ImGui::Text("Score: %d", state.score);
    ImGui::Text("Wave: %d", state.wave);
    ImGui::End();
}

void RenderDamageIndicator(float& timer, float dt) {
    if (timer <= 0.0f) return;
    timer -= dt;

    float alpha = timer * 2.0f; // fade over 0.5s
    if (alpha > 1.0f) alpha = 1.0f;

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImU32 red = IM_COL32(255, 0, 0, static_cast<int>(alpha * 80));

    // Vignette edges
    float thickness = 40.0f;
    draw->AddRectFilled(ImVec2(0, 0), ImVec2(displaySize.x, thickness), red);
    draw->AddRectFilled(ImVec2(0, displaySize.y - thickness), ImVec2(displaySize.x, displaySize.y), red);
    draw->AddRectFilled(ImVec2(0, 0), ImVec2(thickness, displaySize.y), red);
    draw->AddRectFilled(ImVec2(displaySize.x - thickness, 0), ImVec2(displaySize.x, displaySize.y), red);
}

void Render(HUDState& state, float dt) {
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    RenderCrosshair(displaySize.x, displaySize.y, state.crosshairSize);
    RenderHealthBar(state);
    RenderAmmoDisplay(state);
    RenderScoreWave(state);
    RenderDamageIndicator(state.damageIndicatorTimer, dt);
}

} // namespace GameHUD
