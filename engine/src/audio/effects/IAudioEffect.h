#pragma once

#include "wtpch.h"

class IAudioEffect {
public:
    virtual ~IAudioEffect() = default;
    virtual void Process(float* input, float* output, uint64_t frameCount, uint32_t sampleRate) = 0;
};