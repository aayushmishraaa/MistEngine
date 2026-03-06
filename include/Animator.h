#pragma once
#ifndef MIST_ANIMATOR_H
#define MIST_ANIMATOR_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "Animation.h"

class Animator {
public:
    static constexpr int MAX_BONES = 128;

    Animator();
    ~Animator();

    void Init();
    void Update(float dt);
    void PlayAnimation(std::shared_ptr<Animation> animation);
    void BlendTo(std::shared_ptr<Animation> target, float duration);
    void BindBoneSSBO(int binding = 6);

    const std::vector<glm::mat4>& GetBoneMatrices() const { return m_BoneMatrices; }
    bool IsPlaying() const { return m_CurrentAnimation != nullptr; }

private:
    std::shared_ptr<Animation> m_CurrentAnimation;
    std::shared_ptr<Animation> m_BlendTarget;
    float m_CurrentTime = 0.0f;
    float m_BlendFactor = 0.0f;
    float m_BlendDuration = 0.0f;
    float m_BlendTime = 0.0f;

    std::vector<glm::mat4> m_BoneMatrices;
    GLuint m_BoneSSBO = 0;

    void calculateBoneTransforms(Animation* anim, float time, std::vector<glm::mat4>& matrices);
};

#endif
