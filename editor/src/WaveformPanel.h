#pragma once

#include "wtpch.h"
#include "audio/AudioClip.h"

struct WaveformView {
    std::vector<double> time; // x-axis seconds
    std::vector<double> minEnv; // lower envelope
    std::vector<double> maxEnv; // upper envelope
    uint32_t channel = 0;
    bool ready = false;
};

class WaveformPanel {
public:
    void OnImGuiRender();

    void LoadClip(Ref<AudioClip> clip);
    bool HasClip() const { return m_Clip != nullptr; }

private:
    void BuildWaveformView(uint32_t channel, uint32_t targetPoints = 4096);
    void RenderDropZone();
    void RenderWaveform();
    void RenderMetadata();

private:
    Ref<AudioClip> m_Clip;
    WaveformView m_View;

    uint32_t m_SelectedChannel = 0;
    float m_ZoomMin = 0.0f; // seconds
    float m_ZoomMax = 0.0f; // seconds
    bool m_FitOnNextDraw = true;
};