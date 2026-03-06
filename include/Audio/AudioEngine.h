#pragma once
#ifndef MIST_AUDIO_ENGINE_H
#define MIST_AUDIO_ENGINE_H

#include "AudioClip.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

// Forward declare miniaudio types to avoid including the header everywhere
struct ma_engine;
struct ma_sound;

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool Initialize();
    void Shutdown();
    void Update(const glm::vec3& listenerPos, const glm::vec3& listenerFront, const glm::vec3& listenerUp);

    // Clip management
    std::shared_ptr<AudioClip> LoadClip(const std::string& path, const std::string& name = "");
    void UnloadClip(const std::string& name);

    // Playback
    void PlaySound(const std::string& clipName, float volume = 1.0f);
    void PlaySound3D(const std::string& clipName, const glm::vec3& position,
                     float volume = 1.0f, float minDist = 1.0f, float maxDist = 50.0f);
    void PlayMusic(const std::string& path, float volume = 1.0f, bool loop = true);
    void StopMusic();

    // Volume control
    void SetMasterVolume(float volume);
    void SetSFXVolume(float volume);
    void SetMusicVolume(float volume);
    float GetMasterVolume() const { return m_MasterVolume; }
    float GetSFXVolume() const { return m_SFXVolume; }
    float GetMusicVolume() const { return m_MusicVolume; }

    bool IsInitialized() const { return m_Initialized; }

private:
    ma_engine* m_Engine = nullptr;
    bool m_Initialized = false;

    float m_MasterVolume = 1.0f;
    float m_SFXVolume = 1.0f;
    float m_MusicVolume = 0.5f;

    std::unordered_map<std::string, std::shared_ptr<AudioClip>> m_Clips;

    // Active sounds for 3D positioning
    struct ActiveSound {
        ma_sound* sound = nullptr;
        std::string clipName;
    };
    std::vector<ActiveSound> m_ActiveSounds;

    ma_sound* m_MusicSound = nullptr;

    void cleanupFinishedSounds();
};

#endif
