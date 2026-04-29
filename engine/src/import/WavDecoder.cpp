#include "WavDecoder.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

Ref<AudioClip> WavDecoder::Decode(const std::filesystem::path& filePath) {
    drwav wav;
    if (!drwav_init_file(&wav, filePath.string().c_str(), nullptr)) {
        std::cerr << "[WavDecoder] Failed to open: " << filePath << '\n';
        return nullptr;
    }

    const uint64_t frameCount = wav.totalPCMFrameCount;
    if (frameCount == 0) {
        std::cerr << "[WavDecoder] File has zero frames: " << filePath << '\n';
        drwav_uninit(&wav);
        return nullptr;
    }

    const uint32_t channels = wav.channels;
    const uint32_t sampleRate = wav.sampleRate;
    const uint32_t bitDepth = wav.bitsPerSample;

    std::vector<float> samples(frameCount * channels);
    const uint64_t framesRead = drwav_read_pcm_frames_f32(&wav, frameCount, samples.data());
    drwav_uninit(&wav);

    if (framesRead != frameCount) {
        std::cerr << "[WavDecoder] Expected " << frameCount << " frames, read " << framesRead << ": " << filePath << '\n';
        return nullptr;
    }

    AudioMetadata meta;
    meta.format = AudioFileFormat::WAV;
    meta.channels = channels;
    meta.sampleRate = sampleRate;
    meta.frameCount = frameCount;
    meta.durationSeconds = static_cast<float>(frameCount) / static_cast<float>(sampleRate);
    meta.bitDepth = bitDepth;

    return CreateRef<AudioClip>(std::move(meta), std::move(samples), filePath.string());
}