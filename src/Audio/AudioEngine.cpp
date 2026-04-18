#include "Audio/AudioEngine.h"
#include "Core/Logger.h"

#include <algorithm>
#include <filesystem>
#include <memory>

#ifdef MIST_ENABLE_AUDIO

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace {
// Custom deleters so we can use unique_ptr for ma_engine / ma_sound. These
// objects need their ma_*_uninit called before delete, otherwise miniaudio
// leaks internal decoder state even if the C++ side frees the struct.
struct MaEngineDeleter {
    void operator()(ma_engine* e) const noexcept {
        if (e) {
            ma_engine_uninit(e);
            delete e;
        }
    }
};
struct MaSoundDeleter {
    void operator()(ma_sound* s) const noexcept {
        if (s) {
            ma_sound_uninit(s);
            delete s;
        }
    }
};
using MaSoundPtr = std::unique_ptr<ma_sound, MaSoundDeleter>;
} // namespace

AudioEngine::AudioEngine() {}

AudioEngine::~AudioEngine() {
    Shutdown();
}

bool AudioEngine::Initialize() {
    m_Engine = new ma_engine();
    ma_engine_config config = ma_engine_config_init();
    config.listenerCount = 1;

    ma_result result = ma_engine_init(&config, m_Engine);
    if (result != MA_SUCCESS) {
        LOG_ERROR("AudioEngine: Failed to initialize miniaudio engine (error: ", result, ")");
        delete m_Engine;
        m_Engine = nullptr;
        return false;
    }

    m_Initialized = true;
    LOG_INFO("AudioEngine initialized");
    return true;
}

void AudioEngine::Shutdown() {
    if (!m_Initialized) return;

    StopMusic();

    for (auto& active : m_ActiveSounds) {
        if (active.sound) {
            ma_sound_uninit(active.sound);
            delete active.sound;
        }
    }
    m_ActiveSounds.clear();
    m_Clips.clear();

    if (m_Engine) {
        ma_engine_uninit(m_Engine);
        delete m_Engine;
        m_Engine = nullptr;
    }

    m_Initialized = false;
    LOG_INFO("AudioEngine shutdown");
}

void AudioEngine::Update(const glm::vec3& listenerPos, const glm::vec3& listenerFront, const glm::vec3& listenerUp) {
    if (!m_Initialized) return;

    ma_engine_listener_set_position(m_Engine, 0, listenerPos.x, listenerPos.y, listenerPos.z);
    ma_engine_listener_set_direction(m_Engine, 0, listenerFront.x, listenerFront.y, listenerFront.z);
    ma_engine_listener_set_world_up(m_Engine, 0, listenerUp.x, listenerUp.y, listenerUp.z);

    cleanupFinishedSounds();
}

std::shared_ptr<AudioClip> AudioEngine::LoadClip(const std::string& path, const std::string& name) {
    std::string clipName = name.empty() ? path : name;

    auto it = m_Clips.find(clipName);
    if (it != m_Clips.end()) return it->second;

    // Verify the file exists up front. Previously loaded=true was set
    // unconditionally, so PlaySound would silently fail at playback time with
    // no obvious cause.
    std::error_code ec;
    const bool exists = std::filesystem::exists(path, ec) && !ec;

    auto clip = std::make_shared<AudioClip>();
    clip->name = clipName;
    clip->filePath = path;
    clip->loaded = exists;
    m_Clips[clipName] = clip;

    if (exists) {
        LOG_INFO("AudioClip loaded: ", clipName);
    } else {
        LOG_WARN("AudioClip file missing: ", path);
    }
    return clip;
}

void AudioEngine::UnloadClip(const std::string& name) {
    m_Clips.erase(name);
}

void AudioEngine::PlaySound(const std::string& clipName, float volume) {
    if (!m_Initialized) return;

    auto it = m_Clips.find(clipName);
    if (it == m_Clips.end()) {
        LOG_WARN("AudioEngine: Clip not found: ", clipName);
        return;
    }

    ma_engine_play_sound(m_Engine, it->second->filePath.c_str(), nullptr);
}

void AudioEngine::PlaySound3D(const std::string& clipName, const glm::vec3& position,
                               float volume, float minDist, float maxDist) {
    if (!m_Initialized) return;

    auto it = m_Clips.find(clipName);
    if (it == m_Clips.end()) {
        LOG_WARN("AudioEngine: Clip not found: ", clipName);
        return;
    }

    // RAII: if ma_sound_init_from_file fails OR any set_* call below throws,
    // the unique_ptr destroys the ma_sound via ma_sound_uninit + delete.
    // We release ownership into the active-sounds vector only once everything
    // succeeded.
    ma_sound* raw = new ma_sound();
    ma_result result = ma_sound_init_from_file(m_Engine, it->second->filePath.c_str(),
        MA_SOUND_FLAG_DECODE, nullptr, nullptr, raw);

    if (result != MA_SUCCESS) {
        LOG_ERROR("AudioEngine: Failed to play 3D sound: ", clipName);
        // Init failed; miniaudio doesn't require uninit on failure. Avoid
        // passing the raw pointer to the RAII deleter which would call
        // ma_sound_uninit on an uninitialized object.
        delete raw;
        return;
    }

    MaSoundPtr sound(raw);
    ma_sound_set_position(sound.get(), position.x, position.y, position.z);
    ma_sound_set_volume(sound.get(), volume * m_SFXVolume * m_MasterVolume);
    ma_sound_set_min_distance(sound.get(), minDist);
    ma_sound_set_max_distance(sound.get(), maxDist);
    ma_sound_set_spatialization_enabled(sound.get(), MA_TRUE);
    ma_sound_start(sound.get());

    m_ActiveSounds.push_back({sound.release(), clipName});
}

void AudioEngine::PlayMusic(const std::string& path, float volume, bool loop) {
    if (!m_Initialized) return;
    StopMusic();

    m_MusicSound = new ma_sound();
    ma_result result = ma_sound_init_from_file(m_Engine, path.c_str(),
        MA_SOUND_FLAG_STREAM, nullptr, nullptr, m_MusicSound);

    if (result != MA_SUCCESS) {
        LOG_ERROR("AudioEngine: Failed to play music: ", path);
        delete m_MusicSound;
        m_MusicSound = nullptr;
        return;
    }

    ma_sound_set_volume(m_MusicSound, volume * m_MusicVolume * m_MasterVolume);
    ma_sound_set_looping(m_MusicSound, loop ? MA_TRUE : MA_FALSE);
    ma_sound_start(m_MusicSound);
}

void AudioEngine::StopMusic() {
    if (m_MusicSound) {
        ma_sound_stop(m_MusicSound);
        ma_sound_uninit(m_MusicSound);
        delete m_MusicSound;
        m_MusicSound = nullptr;
    }
}

void AudioEngine::SetMasterVolume(float volume) { m_MasterVolume = volume; }
void AudioEngine::SetSFXVolume(float volume) { m_SFXVolume = volume; }
void AudioEngine::SetMusicVolume(float volume) {
    m_MusicVolume = volume;
    if (m_MusicSound) {
        ma_sound_set_volume(m_MusicSound, m_MusicVolume * m_MasterVolume);
    }
}

void AudioEngine::cleanupFinishedSounds() {
    m_ActiveSounds.erase(
        std::remove_if(m_ActiveSounds.begin(), m_ActiveSounds.end(),
            [](ActiveSound& s) {
                if (s.sound && ma_sound_at_end(s.sound)) {
                    ma_sound_uninit(s.sound);
                    delete s.sound;
                    return true;
                }
                return false;
            }),
        m_ActiveSounds.end()
    );
}

#else // !MIST_ENABLE_AUDIO — stub implementation

AudioEngine::AudioEngine() {}
AudioEngine::~AudioEngine() {}
bool AudioEngine::Initialize() {
    LOG_WARN("AudioEngine: Audio disabled at compile time");
    return false;
}
void AudioEngine::Shutdown() {}
void AudioEngine::Update(const glm::vec3&, const glm::vec3&, const glm::vec3&) {}
std::shared_ptr<AudioClip> AudioEngine::LoadClip(const std::string&, const std::string&) { return nullptr; }
void AudioEngine::UnloadClip(const std::string&) {}
void AudioEngine::PlaySound(const std::string&, float) {}
void AudioEngine::PlaySound3D(const std::string&, const glm::vec3&, float, float, float) {}
void AudioEngine::PlayMusic(const std::string&, float, bool) {}
void AudioEngine::StopMusic() {}
void AudioEngine::SetMasterVolume(float v) { m_MasterVolume = v; }
void AudioEngine::SetSFXVolume(float v) { m_SFXVolume = v; }
void AudioEngine::SetMusicVolume(float v) { m_MusicVolume = v; }
void AudioEngine::cleanupFinishedSounds() {}

#endif
