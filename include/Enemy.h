#ifndef ENEMY_H
#define ENEMY_H

#include <glm/glm.hpp>

class Enemy {
public:
    glm::vec3 position;
    float health;

    Enemy(glm::vec3 pos, float hp) : position(pos), health(hp) {}

    void takeDamage(float damage) {
        health -= damage;
        if (health <= 0) {
            std::cout << "Enemy defeated!\n";
        }
    }
};

#endif#pragma once
