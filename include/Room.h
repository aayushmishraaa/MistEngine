#ifndef ROOM_H
#define ROOM_H

#include <glm/glm.hpp>

class Room {
public:
    glm::vec3 position;
    glm::vec3 size;

    Room(glm::vec3 pos, glm::vec3 sz) : position(pos), size(sz) {}
};

#endif#pragma once
