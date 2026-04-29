#pragma once

enum class AudioFileFormat
{
    MP3,
    WAV,
    FLAC,
    Unknown
};

enum class AudioSampleRate : uint32_t
{
    Hz8000 = 8000,
    Hz22050 = 22050,
    Hz44100 = 44100,
    Hz48000 = 48000,
    Hz96000 = 96000
};

struct AudioMetadata {
    AudioFileFormat format = AudioFileFormat::Unknown;
    uint32_t sampleRate = 0;
    uint32_t channels = 0;
    uint64_t frameCount = 0;
    float durationSeconds = 0.0f;
    uint32_t bitDepth = 0;
};