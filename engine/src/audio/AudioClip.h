#pragma once

#include "wtpch.h"
#include "audio/AudioFormat.h"

class AudioClip {
public:
    AudioClip() = default;
    AudioClip(AudioMetadata metadata, std::vector<float> samples, std::string sourcePath)
        : m_Metadata(std::move(metadata)), m_Samples(std::move(samples)), m_SourcePath(std::move(sourcePath)) {
    }

    const AudioMetadata& GetMetadata() const { return m_Metadata; }
    AudioFileFormat GetFormat() const { return m_Metadata.format; }
    uint32_t GetSampleRate() const { return m_Metadata.sampleRate; }
    uint32_t GetChannels() const { return m_Metadata.channels; }
    uint64_t GetFrameCount() const { return m_Metadata.frameCount; }
    float GetDuration() const { return m_Metadata.durationSeconds; }
    const std::string& GetSourcePath() const { return m_SourcePath; }

    const std::vector<float>& GetSamples() const { return m_Samples; }
    std::vector<float>& GetSamples() { return m_Samples; }

    // non-owning pointer to the raw pcm buffer
    const float* Data() const { return m_Samples.data(); }
    size_t Size() const { return m_Samples.size(); }

    bool IsValid() const
    {
        return !m_Samples.empty()
            && m_Metadata.channels > 0
            && m_Metadata.frameCount > 0
            && m_Metadata.sampleRate > 0;
    }


    UUID id;
private:
    AudioMetadata      m_Metadata;
    std::vector<float> m_Samples;
    std::string        m_SourcePath;
};