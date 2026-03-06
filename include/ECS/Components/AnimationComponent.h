#pragma once
#ifndef MIST_ANIMATION_COMPONENT_H
#define MIST_ANIMATION_COMPONENT_H

#include "Animator.h"
#include "Animation.h"
#include <memory>
#include <string>

struct AnimationComponent {
    Animator animator;
    std::shared_ptr<Animation> currentAnimation;
    std::string currentAnimName;
    float playbackSpeed = 1.0f;
    bool playing = false;
    bool loop = true;

    void Play(std::shared_ptr<Animation> anim, const std::string& name = "") {
        currentAnimation = anim;
        currentAnimName = name;
        animator.PlayAnimation(anim);
        playing = true;
    }

    void BlendTo(std::shared_ptr<Animation> anim, float duration, const std::string& name = "") {
        currentAnimation = anim;
        currentAnimName = name;
        animator.BlendTo(anim, duration);
        playing = true;
    }

    void Stop() {
        playing = false;
    }

    void Update(float dt) {
        if (playing && currentAnimation) {
            animator.Update(dt * playbackSpeed);
        }
    }
};

#endif
