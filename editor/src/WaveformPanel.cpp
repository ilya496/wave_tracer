#include "WaveformPanel.h"

#include "imgui.h"
#include "implot.h"
#include "import/AudioImporter.h"

#include "wtpch.h"

void WaveformPanel::LoadClip(Ref<AudioClip> clip) {
    if (!clip || !clip->IsValid())
        return;

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

void WaveformPanel::BuildWaveformView(uint32_t channel, uint32_t targetPoints) {
    m_View = {};

    if (!m_Clip || !m_Clip->IsValid())
        return;

    const auto& samples = m_Clip->GetSamples();
    const uint32_t channels = m_Clip->GetChannels();
    const uint64_t frameCount = m_Clip->GetFrameCount();
    const float sampleRate = static_cast<float>(m_Clip->GetSampleRate());

    channel = std::clamp(channel, 0u, channels - 1u);

    // how many source frames fall into each display bucket
    const int buckets = std::min(static_cast<int>(frameCount), static_cast<int>(targetPoints));
    const double framesPerBucket = static_cast<double>(frameCount) / buckets;

    m_View.time.resize(buckets);
    m_View.minEnv.resize(buckets);
    m_View.maxEnv.resize(buckets);

    for (int b = 0; b < buckets; ++b)
    {
        const uint64_t frameStart = static_cast<uint64_t>(b * framesPerBucket);
        const uint64_t frameEnd = static_cast<uint64_t>((b + 1) * framesPerBucket);

        float minVal = 1.0f;
        float maxVal = -1.0f;

        for (uint64_t f = frameStart; f < frameEnd && f < frameCount; ++f)
        {
            const float s = samples[f * channels + channel];
            if (s < minVal) minVal = s;
            if (s > maxVal) maxVal = s;
        }

        // Guard against empty buckets at the very end
        if (minVal > maxVal) minVal = maxVal = 0.0f;

        m_View.time[b] = static_cast<double>(frameStart) / sampleRate;
        m_View.minEnv[b] = static_cast<double>(minVal);
        m_View.maxEnv[b] = static_cast<double>(maxVal);
    }

    m_View.channel = channel;
    m_View.ready = true;

    m_ZoomMin = 0.0f;
    m_ZoomMax = m_Clip->GetDuration();
}

// ---------------------------------------------------------------------------
// Private — UI
// ---------------------------------------------------------------------------

void WaveformPanel::RenderDropZone()
{
    // Centred "drop a file here" hint that fills the panel
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const ImVec2 cursor = ImGui::GetCursorScreenPos();

    // Dashed border rect
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRect(
        cursor,
        { cursor.x + avail.x, cursor.y + avail.y },
        IM_COL32(120, 120, 120, 160),
        8.0f,   // rounding
        0,      // flags
        2.0f    // thickness
    );

    // Centred label — ImGui has no native AddRectDashed so we fall back to AddRect if unavailable
    const char* label = "Drop an audio file here";
    const char* sublabel = "Supported: .mp3  .wav  .flac";

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

    // Format name
    const char* fmtName = "Unknown";
    switch (meta.format)
    {
    case AudioFileFormat::MP3:  fmtName = "MP3";  break;
    case AudioFileFormat::WAV:  fmtName = "WAV";  break;
    case AudioFileFormat::FLAC: fmtName = "FLAC"; break;
    default: break;
    }

    ImGui::TextDisabled("File:"); ImGui::SameLine();
    // Show just the filename, not the full path
    const std::string& srcPath = m_Clip->GetSourcePath();
    const size_t       slash = srcPath.find_last_of("/\\");
    const std::string  name = (slash != std::string::npos)
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

    // Channel selector (only visible for multi-channel files)
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
    if (!m_View.ready) return;

    // Let ImPlot fill the remaining panel space
    const ImVec2 plotSize = { -1.0f, -1.0f };

    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(8, 8));
    ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
    ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
    // ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
    // ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.2f, 0.7f, 1.0f, 0.25f));

    ImPlot::SetNextAxesLimits(
        m_ZoomMin, m_ZoomMax,
        -1.05, 1.05,
        m_FitOnNextDraw ? ImPlotCond_Always : ImPlotCond_Once
    );
    m_FitOnNextDraw = false;

    if (ImPlot::BeginPlot("##waveform", plotSize,
        ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText))
    {
        ImPlot::SetupAxis(ImAxis_X1, "Time (s)");
        ImPlot::SetupAxis(ImAxis_Y1, "Amplitude");
        ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, -1.05, 1.05);

        const int n = static_cast<int>(m_View.time.size());

        // Shaded envelope band (min → max per bucket)
        ImPlot::PlotShaded(
            "##env",
            m_View.time.data(),
            m_View.minEnv.data(),
            m_View.maxEnv.data(),
            n
        );

        // Upper and lower envelope lines for crispness
        ImPlot::PlotLine("##max", m_View.time.data(), m_View.maxEnv.data(), n);
        ImPlot::PlotLine("##min", m_View.time.data(), m_View.minEnv.data(), n);

        // Zero line
        ImPlot::PopStyleColor(2); // pop Line + Fill to override colour
        // ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 1.0f, 1.0f, 0.15f));
        const double zeroX[2] = { m_View.time.front(), m_View.time.back() };
        const double zeroY[2] = { 0.0, 0.0 };
        ImPlot::PlotLine("##zero", zeroX, zeroY, 2);
        ImPlot::PopStyleColor(1);
        // ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
        // ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.2f, 0.7f, 1.0f, 0.25f));

        ImPlot::EndPlot();
    }

    // ImPlot::PopStyleColor(4);
    ImPlot::PopStyleColor(2);
    ImPlot::PopStyleVar(1);
}
