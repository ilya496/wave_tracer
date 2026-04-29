#pragma once

#include "wtpch.h"
#include "audio/AudioClip.h"

struct WaveformView {
    std::vector<double> time; // x-axis seconds
    std::vector<double> minEnv; // lower envelope
    std::vector<double> maxEnv; // upper envelope
    uint32_t channel = 0;
    bool ready = false;

    void Reset() {
        time.clear();
        minEnv.clear();
        maxEnv.clear();
        channel = 0;
        ready = false;
    }
};

struct WaveformLOD {
    std::vector<double> time;
    std::vector<double> minEnv;
    std::vector<double> maxEnv;
};

struct WaveformCache {
    std::vector<WaveformLOD> levels;
    uint32_t channel = 0;
    bool ready = false;

    void Reset() {
        levels.clear();
        ready = false;
    }
};

class WaveformPanel {
public:
    WaveformPanel() {
        std::cerr << "[WaveformPanel] Constructor called" << std::endl;
    }
    ~WaveformPanel() {
        std::cerr << "[WaveformPanel] Destructor called" << std::endl;
    }

    void OnImGuiRender();

    void LoadClip(Ref<AudioClip> clip);
    bool HasClip() const { return m_Clip != nullptr; }

private:
    void BuildWaveformView(uint32_t channel, uint32_t targetPoints = 4096);
    void RenderDropZone();
    void RenderWaveform();
    void RenderMetadata();

private:
    Ref<AudioClip> m_Clip = nullptr;
    // WaveformView m_View;
    WaveformCache m_Cache;
    const WaveformLOD* m_CurrentLOD = nullptr;

    uint32_t m_SelectedChannel = 0;
    float m_ZoomMin = 0.0f; // seconds
    float m_ZoomMax = 0.0f; // seconds
    bool m_FitOnNextDraw = true;
};