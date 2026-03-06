#pragma once
#ifndef MIST_AABB_H
#define MIST_AABB_H

#include <glm/glm.hpp>
#include <algorithm>
#include <array>

struct Frustum;

struct AABB {
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());

    glm::vec3 Center() const { return (min + max) * 0.5f; }
    glm::vec3 Extents() const { return (max - min) * 0.5f; }

    void Merge(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    void Merge(const AABB& other) {
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
    }

    AABB Transform(const glm::mat4& m) const {
        std::array<glm::vec3, 8> corners = {
            glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z),
            glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z),
            glm::vec3(min.x, min.y, max.z), glm::vec3(max.x, min.y, max.z),
            glm::vec3(min.x, max.y, max.z), glm::vec3(max.x, max.y, max.z)
        };
        AABB result;
        for (auto& c : corners) {
            glm::vec4 t = m * glm::vec4(c, 1.0f);
            result.Merge(glm::vec3(t));
        }
        return result;
    }

    bool Intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }

    bool IsValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }
};

#endif
