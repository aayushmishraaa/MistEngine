#include "ECS/Components/AudioSourceComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Audio/AudioEngine.h"
#include "Core/Logger.h"
#include <vector>

class AudioSystem {
public:
    void Init(AudioEngine* engine) {
        m_Engine = engine;
    }

    void Update(const std::vector<std::pair<AudioSourceComponent*, TransformComponent*>>& sources) {
        if (!m_Engine || !m_Engine->IsInitialized()) return;

        for (auto& [audio, transform] : sources) {
            if (!audio || !transform) continue;

            if (audio->playOnStart && !audio->playing) {
                if (audio->is3D) {
                    m_Engine->PlaySound3D(audio->clipName, transform->position,
                        audio->volume, audio->minDistance, audio->maxDistance);
                } else {
                    m_Engine->PlaySound(audio->clipName, audio->volume);
                }
                audio->playing = true;
                if (!audio->loop) audio->playOnStart = false;
            }
        }
    }

private:
    AudioEngine* m_Engine = nullptr;
};
