#pragma once

#include "wtpch.h"

enum class AudioDeviceAPI {
    None = 0,
    Windows_WASAPI,
    Windows_ASIO,
    Linux_ALSA
};

struct AudioDeviceInfo {
    int id;
    std::string name;
    uint32_t maxInputChannels;
    uint32_t maxOutputChannels;
    std::vector<uint32_t> supportedSampleRates;
    bool isDefault;
};