#pragma once
#ifndef MIST_AUDIO_SOURCE_COMPONENT_H
#define MIST_AUDIO_SOURCE_COMPONENT_H

#include <string>

struct AudioSourceComponent {
    std::string clipName;
    float volume = 1.0f;
    float minDistance = 1.0f;
    float maxDistance = 50.0f;
    bool is3D = true;
    bool loop = false;
    bool playOnStart = false;
    bool playing = false;
};

#endif
