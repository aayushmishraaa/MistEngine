#pragma once
#ifndef MIST_ANIMATION_COMPONENT_H
#define MIST_ANIMATION_COMPONENT_H

#include "Animator.h"
#include "Animation.h"

#include <memory>
#include <string>
#include <vector>

// Per-entity animation state. The actual skinning matrices live on
// the Animator's GPU-visible SSBO; this component is the *authoring*
// surface — which clip is playing, how fast, and which clips are
// available for the Inspector dropdown (Phase D).
//
// `availableClips` is populated by the SceneImporter when a rigged
// model is loaded. Each entry is one aiAnimation from the source
// file (e.g. "idle", "walk", "run") parsed via
// AnimatedModel::ExtractAllAnimations.
struct AnimationComponent {
    Animator                                       animator;
    std::shared_ptr<Animation>                     currentAnimation;
    std::string                                    currentAnimName;
    std::vector<std::shared_ptr<Animation>>        availableClips;

    float playbackSpeed = 1.0f;
    bool  playing       = false;
    bool  loop          = true;

    void Play(std::shared_ptr<Animation> anim, const std::string& name = "") {
        currentAnimation = anim;
        currentAnimName  = name;
        animator.PlayAnimation(anim);
        playing = true;
    }

    void BlendTo(std::shared_ptr<Animation> anim, float duration, const std::string& name = "") {
        currentAnimation = anim;
        currentAnimName  = name;
        animator.BlendTo(anim, duration);
        playing = true;
    }

    void Stop() { playing = false; }

    void Update(float dt) {
        if (playing && currentAnimation) {
            animator.Update(dt * playbackSpeed);
        }
    }

    // Convenience: switch to a named clip from `availableClips`.
    // Returns true if the clip was found and started.
    bool PlayByName(const std::string& name) {
        for (auto& c : availableClips) {
            if (c && c->name == name) {
                Play(c, name);
                return true;
            }
        }
        return false;
    }
};

#endif
