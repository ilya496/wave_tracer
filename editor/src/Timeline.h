#pragma once

#include "wtpch.h"
#include "import/AudioImporter.h"

struct TimelineClip {
    Ref<AudioClip> audioData = nullptr;
    std::string name = "New Clip";
    float startTime = 0.0f;
    float duration = 5.0f;
    float offset = 0.0f;
};

struct AudioTrack {
    std::string name = "Audio Track";
    float volume = 1.0f;
    float pan = 0.0f;
    bool mute = false;
    bool solo = false;
    std::vector<TimelineClip> clips;
};

struct Timeline {
    float playheadPosition = 0.0f;
    float zoomPixelsPerSecond = 100.0f;
    std::vector<AudioTrack> tracks;
};