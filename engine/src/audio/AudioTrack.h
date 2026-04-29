#pragma once

#include "wtpch.h"
#include "AudioClip.h"
#include "effects/IAudioEffect.h"

struct AudioTrack {
    UUID id;
    std::string name;
    Ref<AudioClip> clip;
    uint64_t timelineOffset; // in frames
    float volume = 1.0f;
    float pan = 0.0f; // -1.0 (left) to 1.0 (right)
    bool muted = false;
    bool soloed = false;
    std::vector<Ref<IAudioEffect>> effects;
};