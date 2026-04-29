#include "AudioImporter.h"

#include "WavDecoder.h"

static std::string ToLowerExtension(const std::filesystem::path& filePath) {
    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

static const std::unordered_map<std::string, AudioFileFormat>& ExtensionMap() {
    static const std::unordered_map<std::string, AudioFileFormat> map =
    {
        { ".mp3",  AudioFileFormat::MP3 },
        { ".wav",  AudioFileFormat::WAV },
        { ".wave", AudioFileFormat::WAV },
        { ".flac", AudioFileFormat::FLAC },
    };

    return map;
}

AudioFileFormat AudioImporter::GetFormat(const std::filesystem::path& filePath) {
    const std::string ext = ToLowerExtension(filePath);
    const auto& map = ExtensionMap();
    auto it = map.find(ext);
    return it != map.end() ? it->second : AudioFileFormat::Unknown;
}

bool AudioImporter::IsSupportedFormat(const std::filesystem::path& filePath) {
    return GetFormat(filePath) != AudioFileFormat::Unknown;
}

Ref<AudioClip> AudioImporter::Import(const std::filesystem::path& filePath) {
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "[AudioImporter] File not found: " << filePath << "\n";
        return nullptr;
    }

    const AudioFileFormat format = GetFormat(filePath);
    if (format == AudioFileFormat::Unknown) {
        std::cerr << "[AudioImporter] Unsupported format '"
            << filePath.extension() << "': " << filePath << "\n";
        return nullptr;
    }

    switch (format) {
    case AudioFileFormat::MP3:
        // return ImportMP3(filePath);
        break;
    case AudioFileFormat::WAV:
        WavDecoder decoder;
        return decoder.Decode(filePath);
    case AudioFileFormat::FLAC:
        // return ImportFLAC(filePath);
        break;
    default:
        return nullptr;
    }
}