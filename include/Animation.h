#pragma once
#ifndef MIST_ANIMATION_H
#define MIST_ANIMATION_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <map>

struct BoneInfo {
    int id = -1;
    glm::mat4 offset = glm::mat4(1.0f);
};

struct KeyPosition { float timeStamp; glm::vec3 position; };
struct KeyRotation { float timeStamp; glm::quat orientation; };
struct KeyScale    { float timeStamp; glm::vec3 scale; };

class BoneAnimation {
public:
    std::string name;
    int boneID = -1;
    std::vector<KeyPosition> positions;
    std::vector<KeyRotation> rotations;
    std::vector<KeyScale> scales;

    glm::vec3 InterpolatePosition(float time) const;
    glm::quat InterpolateRotation(float time) const;
    glm::vec3 InterpolateScale(float time) const;
    glm::mat4 GetLocalTransform(float time) const;
};

class Animation {
public:
    std::string name;
    float duration = 0.0f;
    float ticksPerSecond = 25.0f;
    std::vector<BoneAnimation> boneAnimations;
    std::map<std::string, int> boneNameToIndex;

    BoneAnimation* FindBoneAnimation(const std::string& boneName);
};

#endif
