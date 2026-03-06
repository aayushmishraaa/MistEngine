#include "Animator.h"
#include "Core/Logger.h"
#include <glm/gtc/matrix_transform.hpp>

// --- BoneAnimation ---

glm::vec3 BoneAnimation::InterpolatePosition(float time) const {
    if (positions.size() == 1) return positions[0].position;
    for (size_t i = 0; i + 1 < positions.size(); i++) {
        if (time < positions[i + 1].timeStamp) {
            float t = (time - positions[i].timeStamp) / (positions[i + 1].timeStamp - positions[i].timeStamp);
            return glm::mix(positions[i].position, positions[i + 1].position, t);
        }
    }
    return positions.back().position;
}

glm::quat BoneAnimation::InterpolateRotation(float time) const {
    if (rotations.size() == 1) return rotations[0].orientation;
    for (size_t i = 0; i + 1 < rotations.size(); i++) {
        if (time < rotations[i + 1].timeStamp) {
            float t = (time - rotations[i].timeStamp) / (rotations[i + 1].timeStamp - rotations[i].timeStamp);
            return glm::slerp(rotations[i].orientation, rotations[i + 1].orientation, t);
        }
    }
    return rotations.back().orientation;
}

glm::vec3 BoneAnimation::InterpolateScale(float time) const {
    if (scales.size() == 1) return scales[0].scale;
    for (size_t i = 0; i + 1 < scales.size(); i++) {
        if (time < scales[i + 1].timeStamp) {
            float t = (time - scales[i].timeStamp) / (scales[i + 1].timeStamp - scales[i].timeStamp);
            return glm::mix(scales[i].scale, scales[i + 1].scale, t);
        }
    }
    return scales.back().scale;
}

glm::mat4 BoneAnimation::GetLocalTransform(float time) const {
    glm::mat4 m(1.0f);
    m = glm::translate(m, InterpolatePosition(time));
    m *= glm::mat4_cast(InterpolateRotation(time));
    m = glm::scale(m, InterpolateScale(time));
    return m;
}

// --- Animation ---

BoneAnimation* Animation::FindBoneAnimation(const std::string& boneName) {
    auto it = boneNameToIndex.find(boneName);
    if (it != boneNameToIndex.end()) return &boneAnimations[it->second];
    return nullptr;
}

// --- Animator ---

Animator::Animator() {
    m_BoneMatrices.resize(MAX_BONES, glm::mat4(1.0f));
}

Animator::~Animator() {
    if (m_BoneSSBO) glDeleteBuffers(1, &m_BoneSSBO);
}

void Animator::Init() {
    glCreateBuffers(1, &m_BoneSSBO);
    glNamedBufferStorage(m_BoneSSBO, MAX_BONES * sizeof(glm::mat4), nullptr, GL_DYNAMIC_STORAGE_BIT);
    LOG_INFO("Animator initialized: max ", MAX_BONES, " bones");
}

void Animator::Update(float dt) {
    if (!m_CurrentAnimation) return;

    m_CurrentTime += dt * m_CurrentAnimation->ticksPerSecond;
    if (m_CurrentTime > m_CurrentAnimation->duration) {
        m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->duration);
    }

    calculateBoneTransforms(m_CurrentAnimation.get(), m_CurrentTime, m_BoneMatrices);

    // Handle blending
    if (m_BlendTarget && m_BlendDuration > 0.0f) {
        m_BlendTime += dt;
        m_BlendFactor = glm::clamp(m_BlendTime / m_BlendDuration, 0.0f, 1.0f);

        std::vector<glm::mat4> targetMatrices(MAX_BONES, glm::mat4(1.0f));
        calculateBoneTransforms(m_BlendTarget.get(), m_CurrentTime, targetMatrices);

        for (int i = 0; i < MAX_BONES; i++) {
            // Simple matrix lerp (not ideal but sufficient)
            for (int c = 0; c < 4; c++) {
                m_BoneMatrices[i][c] = glm::mix(m_BoneMatrices[i][c], targetMatrices[i][c], m_BlendFactor);
            }
        }

        if (m_BlendFactor >= 1.0f) {
            m_CurrentAnimation = m_BlendTarget;
            m_BlendTarget = nullptr;
            m_BlendDuration = 0.0f;
            m_BlendTime = 0.0f;
        }
    }

    // Upload to GPU
    glNamedBufferSubData(m_BoneSSBO, 0, m_BoneMatrices.size() * sizeof(glm::mat4), m_BoneMatrices.data());
}

void Animator::PlayAnimation(std::shared_ptr<Animation> animation) {
    m_CurrentAnimation = animation;
    m_CurrentTime = 0.0f;
    m_BlendTarget = nullptr;
}

void Animator::BlendTo(std::shared_ptr<Animation> target, float duration) {
    m_BlendTarget = target;
    m_BlendDuration = duration;
    m_BlendTime = 0.0f;
}

void Animator::BindBoneSSBO(int binding) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_BoneSSBO);
}

void Animator::calculateBoneTransforms(Animation* anim, float time, std::vector<glm::mat4>& matrices) {
    for (auto& boneAnim : anim->boneAnimations) {
        if (boneAnim.boneID >= 0 && boneAnim.boneID < MAX_BONES) {
            matrices[boneAnim.boneID] = boneAnim.GetLocalTransform(time);
        }
    }
}
