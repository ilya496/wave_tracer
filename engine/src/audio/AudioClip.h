#pragma once

#include "wtpch.h"
#include "UUID.h"

struct AudioClip {
    UUID id;
    std::string name;
    uint32_t sampleRate;
    uint32_t channels;
    uint64_t frameCount;
    std::vector<float> samples;
};