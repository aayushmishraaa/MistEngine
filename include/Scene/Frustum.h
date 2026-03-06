#pragma once
#ifndef MIST_FRUSTUM_H
#define MIST_FRUSTUM_H

#include <glm/glm.hpp>
#include "AABB.h"

struct Plane {
    glm::vec3 normal;
    float distance;

    float DistanceToPoint(const glm::vec3& point) const {
        return glm::dot(normal, point) + distance;
    }
};

struct Frustum {
    Plane planes[6]; // Left, Right, Bottom, Top, Near, Far

    void ExtractFromVP(const glm::mat4& vp) {
        // Left
        planes[0].normal.x = vp[0][3] + vp[0][0];
        planes[0].normal.y = vp[1][3] + vp[1][0];
        planes[0].normal.z = vp[2][3] + vp[2][0];
        planes[0].distance = vp[3][3] + vp[3][0];
        // Right
        planes[1].normal.x = vp[0][3] - vp[0][0];
        planes[1].normal.y = vp[1][3] - vp[1][0];
        planes[1].normal.z = vp[2][3] - vp[2][0];
        planes[1].distance = vp[3][3] - vp[3][0];
        // Bottom
        planes[2].normal.x = vp[0][3] + vp[0][1];
        planes[2].normal.y = vp[1][3] + vp[1][1];
        planes[2].normal.z = vp[2][3] + vp[2][1];
        planes[2].distance = vp[3][3] + vp[3][1];
        // Top
        planes[3].normal.x = vp[0][3] - vp[0][1];
        planes[3].normal.y = vp[1][3] - vp[1][1];
        planes[3].normal.z = vp[2][3] - vp[2][1];
        planes[3].distance = vp[3][3] - vp[3][1];
        // Near
        planes[4].normal.x = vp[0][3] + vp[0][2];
        planes[4].normal.y = vp[1][3] + vp[1][2];
        planes[4].normal.z = vp[2][3] + vp[2][2];
        planes[4].distance = vp[3][3] + vp[3][2];
        // Far
        planes[5].normal.x = vp[0][3] - vp[0][2];
        planes[5].normal.y = vp[1][3] - vp[1][2];
        planes[5].normal.z = vp[2][3] - vp[2][2];
        planes[5].distance = vp[3][3] - vp[3][2];

        // Normalize
        for (auto& p : planes) {
            float len = glm::length(p.normal);
            p.normal /= len;
            p.distance /= len;
        }
    }

    bool Intersects(const AABB& aabb) const {
        for (int i = 0; i < 6; i++) {
            glm::vec3 pVertex(
                (planes[i].normal.x >= 0) ? aabb.max.x : aabb.min.x,
                (planes[i].normal.y >= 0) ? aabb.max.y : aabb.min.y,
                (planes[i].normal.z >= 0) ? aabb.max.z : aabb.min.z
            );
            if (planes[i].DistanceToPoint(pVertex) < 0) return false;
        }
        return true;
    }
};

#endif
