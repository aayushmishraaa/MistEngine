#ifndef HEALTHCOMPONENT_H
#define HEALTHCOMPONENT_H

#include <algorithm>

// Ensure min/max macros don't interfere
#ifdef min
#undef min
#endif
#ifdef max  
#undef max
#endif

struct HealthComponent {
    float maxHealth = 100.0f;
    float currentHealth = 100.0f;
    bool isDead = false;
    float respawnTime = 5.0f;
    float timeSinceDeath = 0.0f;
    
    void TakeDamage(float damage) {
        currentHealth -= damage;
        if (currentHealth <= 0.0f) {
            currentHealth = 0.0f;
            isDead = true;
        }
    }
    
    void Heal(float amount) {
        currentHealth = (std::min)(maxHealth, currentHealth + amount);
        if (currentHealth > 0.0f) {
            isDead = false;
        }
    }
    
    void Reset() {
        currentHealth = maxHealth;
        isDead = false;
        timeSinceDeath = 0.0f;
    }
};

#endif // HEALTHCOMPONENT_H