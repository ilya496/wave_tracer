#pragma once

#include "wtpch.h"
#include "audio/AudioClip.h"
#include "audio/AudioFormat.h"

class AudioImporter {
public:
    static Ref<AudioClip> Import(const std::filesystem::path& filePath);

    static AudioFileFormat GetFormat(const std::filesystem::path& filePath);
    static bool IsSupportedFormat(const std::filesystem::path& filePath);

private:
    AudioImporter() = delete;
};