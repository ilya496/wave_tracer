#pragma once

#include "wtpch.h"
#include "audio/AudioClip.h"
#include "audio/AudioFormat.h"

class WavDecoder {
public:
    Ref<AudioClip> Decode(const std::filesystem::path& filePath);
};