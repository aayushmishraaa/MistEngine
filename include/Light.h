#pragma once
#ifndef MIST_LIGHT_H
#define MIST_LIGHT_H

#include <glm/glm.hpp>

enum class LightType : int {
    Directional = 0,
    Point = 1,
    Spot = 2
};

struct Light {
    glm::vec4 position;    // xyz = position, w = type (0=dir, 1=point, 2=spot)
    glm::vec4 direction;   // xyz = direction, w = innerCone
    glm::vec4 color;       // xyz = color, w = intensity
    glm::vec4 params;      // x = range, y = outerCone, z = shadowIdx, w = padding
};

#endif
