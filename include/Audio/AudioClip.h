#pragma once
#ifndef MIST_AUDIO_CLIP_H
#define MIST_AUDIO_CLIP_H

#include <string>
#include <cstdint>

struct AudioClip {
    std::string name;
    std::string filePath;
    float duration = 0.0f;
    bool loaded = false;
    // Internal handle used by AudioEngine (miniaudio sound index)
    uint32_t internalHandle = 0;
};

#endif
