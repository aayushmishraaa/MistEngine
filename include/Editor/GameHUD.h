#pragma once
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

void Render(HUDState& state, float dt);

} // namespace GameHUD
