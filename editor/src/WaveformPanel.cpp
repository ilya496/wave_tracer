#include "WaveformPanel.h"

#include "imgui.h"
#include "implot.h"
#include "import/AudioImporter.h"

#include "wtpch.h"

void WaveformPanel::LoadClip(Ref<AudioClip> clip) {
    if (!clip || !clip->IsValid()) {
        std::cerr << "[WaveformPanel] LoadClip: invalid clip" << std::endl;
        return;
    }

    m_Clip = clip;
    m_SelectedChannel = 0;
    m_FitOnNextDraw = true;

    BuildWaveformView(m_SelectedChannel);
}

void WaveformPanel::OnImGuiRender() {
    ImGui::Begin("Waveform");

    if (!m_Clip) {
        RenderDropZone();
        ImGui::End();
        return;
    }

    RenderMetadata();
    ImGui::Separator();
    RenderWaveform();

    ImGui::End();
}

void WaveformPanel::BuildWaveformView(uint32_t channel, uint32_t targetPoints)
{
    m_View.Reset();

    if (!m_Clip || !m_Clip->IsValid())
        return;

    const auto& samples = m_Clip->GetSamples();
    const uint32_t chans = m_Clip->GetChannels();
    const uint64_t frames = m_Clip->GetFrameCount();
    const float sr = static_cast<float>(m_Clip->GetSampleRate());

    if (chans == 0 || frames == 0 || sr <= 0.0f)
        return;

    const uint64_t expectedSamples = frames * static_cast<uint64_t>(chans);
    if (samples.size() < expectedSamples)
        return;

    channel = std::clamp(channel, 0u, chans - 1u);

    const size_t buckets = static_cast<size_t>(std::min<uint64_t>(frames, targetPoints));
    if (buckets == 0)
        return;

    const double framesPerBucket = static_cast<double>(frames) / static_cast<double>(buckets);

    m_View.time.resize(buckets);
    m_View.minEnv.resize(buckets);
    m_View.maxEnv.resize(buckets);

    for (size_t b = 0; b < buckets; ++b)
    {
        const uint64_t frameStart = static_cast<uint64_t>(b * framesPerBucket);
        const uint64_t frameEnd = static_cast<uint64_t>((b + 1) * framesPerBucket);

        float minVal = 1.0f;
        float maxVal = -1.0f;

        for (uint64_t f = frameStart; f < frameEnd && f < frames; ++f)
        {
            const uint64_t idx = f * static_cast<uint64_t>(chans) + channel;
            if (idx >= samples.size())
                break;

            const float s = samples[idx];
            minVal = std::min(minVal, s);
            maxVal = std::max(maxVal, s);
        }

        if (minVal > maxVal)
            minVal = maxVal = 0.0f;

        m_View.time[b] = static_cast<double>(frameStart) / sr;
        m_View.minEnv[b] = static_cast<double>(minVal);
        m_View.maxEnv[b] = static_cast<double>(maxVal);
    }

    m_View.channel = channel;
    m_View.ready = true;

    m_ZoomMin = 0.0f;
    m_ZoomMax = m_Clip->GetDuration();
}

void WaveformPanel::RenderDropZone()
{
    // centred hint
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const ImVec2 cursor = ImGui::GetCursorScreenPos();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRect(
        cursor,
        { cursor.x + avail.x, cursor.y + avail.y },
        IM_COL32(120, 120, 120, 160),
        8.0f,   // rounding
        0,      // flags
        2.0f    // thickness
    );

    const char* label = "Drop an audio file here";
    // const char* sublabel = "Supported: .mp3 .wav .flac";
    const char* sublabel = "Supported: .wav .wave";

    const ImVec2 labelSize = ImGui::CalcTextSize(label);
    const ImVec2 sublabelSize = ImGui::CalcTextSize(sublabel);
    const float  totalH = labelSize.y + 6.0f + sublabelSize.y;

    ImGui::SetCursorScreenPos({
        cursor.x + (avail.x - labelSize.x) * 0.5f,
        cursor.y + (avail.y - totalH) * 0.5f
        });
    ImGui::TextDisabled("%s", label);

    ImGui::SetCursorScreenPos({
        cursor.x + (avail.x - sublabelSize.x) * 0.5f,
        cursor.y + (avail.y - totalH) * 0.5f + labelSize.y + 6.0f
        });
    ImGui::TextDisabled("%s", sublabel);
}

void WaveformPanel::RenderMetadata()
{
    if (!m_Clip) return;

    const auto& meta = m_Clip->GetMetadata();

    const char* fmtName = "Unknown";
    switch (meta.format)
    {
    case AudioFileFormat::MP3: fmtName = "MP3";  break;
    case AudioFileFormat::WAV: fmtName = "WAV";  break;
    case AudioFileFormat::FLAC: fmtName = "FLAC"; break;
    default: break;
    }

    ImGui::TextDisabled("File:"); ImGui::SameLine();
    const std::string& srcPath = m_Clip->GetSourcePath();
    const size_t slash = srcPath.find_last_of("/\\");
    const std::string name = (slash != std::string::npos)
        ? srcPath.substr(slash + 1)
        : srcPath;
    ImGui::Text("%s", name.c_str());

    ImGui::SameLine(0, 20);
    ImGui::TextDisabled("Format:"); ImGui::SameLine();
    ImGui::Text("%s", fmtName);

    ImGui::SameLine(0, 20);
    ImGui::TextDisabled("Sample rate:"); ImGui::SameLine();
    ImGui::Text("%u Hz", meta.sampleRate);

    ImGui::SameLine(0, 20);
    ImGui::TextDisabled("Channels:"); ImGui::SameLine();
    ImGui::Text("%u", meta.channels);

    ImGui::SameLine(0, 20);
    ImGui::TextDisabled("Duration:"); ImGui::SameLine();
    const int    mins = static_cast<int>(meta.durationSeconds) / 60;
    const float  secs = meta.durationSeconds - mins * 60.0f;
    ImGui::Text("%d:%05.2f", mins, secs);

    if (meta.channels > 1)
    {
        ImGui::SameLine(0, 20);
        ImGui::TextDisabled("Show channel:"); ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::BeginCombo("##chan", m_SelectedChannel == 0 ? "Left" : "Right"))
        {
            for (int c = 0; c < static_cast<int>(meta.channels); ++c)
            {
                const char* label = (c == 0) ? "Left" : (c == 1) ? "Right" : "Ch N";
                if (ImGui::Selectable(label, m_SelectedChannel == c))
                {
                    m_SelectedChannel = c;
                    BuildWaveformView(c);
                }
            }
            ImGui::EndCombo();
        }
    }
}

void WaveformPanel::RenderWaveform()
{
    std::cerr << "[WaveformPanel] RenderWaveform entry ready=" << m_View.ready
        << " time=" << m_View.time.size()
        << " minEnv=" << m_View.minEnv.size()
        << " maxEnv=" << m_View.maxEnv.size() << std::endl;

    if (!m_View.ready) return;

    const int timeSize = static_cast<int>(m_View.time.size());
    const int minEnvSize = static_cast<int>(m_View.minEnv.size());
    const int maxEnvSize = static_cast<int>(m_View.maxEnv.size());

    if (timeSize <= 0 || minEnvSize != timeSize || maxEnvSize != timeSize) {
        std::cerr << "[WaveformPanel] Invalid view data: time=" << timeSize
            << " minEnv=" << minEnvSize << " maxEnv=" << maxEnvSize << "\n";
        return;
    }

    const ImVec2 plotSize = { -1.0f, -1.0f };

    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(8, 8));
    ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
    ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));

    ImPlot::SetNextAxesLimits(
        m_ZoomMin, m_ZoomMax,
        -2.00, 2.00,
        m_FitOnNextDraw ? ImPlotCond_Always : ImPlotCond_Once
    );
    m_FitOnNextDraw = false;

    if (ImPlot::BeginPlot("##waveform", plotSize,
        ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText))
    {
        ImPlot::SetupAxis(ImAxis_X1, "Time (s)");
        ImPlot::SetupAxis(ImAxis_Y1, "Amplitude");
        ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, -2.00, 2.00);

        const int n = timeSize;

        if (n > 0) {
            ImPlot::PlotShaded(
                "##env",
                m_View.time.data(),
                m_View.minEnv.data(),
                m_View.maxEnv.data(),
                n
            );

            ImPlot::PlotLine("##max", m_View.time.data(), m_View.maxEnv.data(), n);
            ImPlot::PlotLine("##min", m_View.time.data(), m_View.minEnv.data(), n);

            const double zeroX[2] = { m_View.time.front(), m_View.time.back() };
            const double zeroY[2] = { 0.0, 0.0 };
            ImPlot::PlotLine("##zero", zeroX, zeroY, 2);
        }

        ImPlot::EndPlot();
    }

    ImPlot::PopStyleColor(2);
    ImPlot::PopStyleVar(1);
}
