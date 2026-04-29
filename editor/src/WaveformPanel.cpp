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
    m_Cache.Reset();

    if (!m_Clip || !m_Clip->IsValid()) {
        std::cerr << "[WaveformPanel] BuildWaveformView: invalid clip" << '\n';
        return;
    }

    const auto& samples = m_Clip->GetSamples();
    const uint32_t channels = m_Clip->GetChannels();
    const uint64_t frameCount = m_Clip->GetFrameCount();
    const float sampleRate = static_cast<float>(m_Clip->GetSampleRate());

    if (channels == 0 || frameCount == 0 || sampleRate <= 0.0f) {
        std::cerr << "[WaveformPanel] BuildWaveformView: invalid metadata - channels="
            << channels << " frameCount=" << frameCount
            << " sampleRate=" << sampleRate << '\n';
        return;
    }

    const uint64_t expectedSamples = frameCount * static_cast<uint64_t>(channels);
    if (samples.size() < expectedSamples) {
        std::cerr << "[WaveformPanel] BuildWaveformView: sample buffer too small - size="
            << samples.size() << " expected=" << expectedSamples << '\n';
        return;
    }

    channel = std::clamp(channel, 0u, channels - 1u);

    // level 0 - base resolution

    size_t buckets = std::min<uint64_t>(frameCount, targetPoints);
    if (buckets == 0) {
        std::cerr << "[WaveformPanel] BuildWaveformView: buckets is 0, returning" << '\n';
        return;
    }

    WaveformLOD base;
    base.time.resize(buckets);
    base.minEnv.resize(buckets);
    base.maxEnv.resize(buckets);

    for (size_t b = 0; b < buckets; ++b)
    {
        uint64_t start = (b * frameCount) / buckets;
        uint64_t end = ((b + 1) * frameCount) / buckets;

        float minVal = 1.0f;
        float maxVal = -1.0f;

        for (uint64_t f = start; f < end; ++f)
        {
            uint64_t idx = f * channels + channel;
            if (idx >= samples.size()) break;

            float s = samples[idx];
            minVal = std::min(minVal, s);
            maxVal = std::max(maxVal, s);
        }

        if (minVal > maxVal)
            minVal = maxVal = 0.0f;

        base.time[b] = (double)start / sampleRate;
        base.minEnv[b] = minVal;
        base.maxEnv[b] = maxVal;
    }

    m_Cache.levels.push_back(std::move(base));

    // --- Build lower resolutions (downsampling) ---
    while (m_Cache.levels.back().time.size() > 512)
    {
        const WaveformLOD& prev = m_Cache.levels.back();
        size_t newSize = prev.time.size() / 2;

        WaveformLOD next;
        next.time.resize(newSize);
        next.minEnv.resize(newSize);
        next.maxEnv.resize(newSize);

        for (size_t i = 0; i < newSize; ++i)
        {
            size_t i0 = i * 2;
            size_t i1 = i * 2 + 1;

            next.time[i] = prev.time[i0];

            next.minEnv[i] = std::min(prev.minEnv[i0], prev.minEnv[i1]);
            next.maxEnv[i] = std::max(prev.maxEnv[i0], prev.maxEnv[i1]);
        }

        m_Cache.levels.push_back(std::move(next));
    }

    m_Cache.channel = channel;
    m_Cache.ready = true;

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
    if (!m_Cache.ready || m_Cache.levels.empty())
        return;

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

        ImPlotRect limits = ImPlot::GetPlotLimits();
        double visibleTime = limits.X.Max - limits.X.Min;

        float pixels = ImGui::GetContentRegionAvail().x;
        if (pixels <= 0.0f) pixels = 1.0f;

        // choose best LOD
        const WaveformLOD* selected = &m_Cache.levels.back(); // fallback (lowest resolution)

        for (const auto& lod : m_Cache.levels)
        {
            if (lod.time.empty())
                continue;

            double samplesPerPixel = (double)lod.time.size() / pixels;

            if (samplesPerPixel < 2.0) // sweet spot
            {
                selected = &lod;
                break;
            }
        }

        // validate selected
        const int n = static_cast<int>(selected->time.size());

        if (n > 0 &&
            selected->minEnv.size() == selected->time.size() &&
            selected->maxEnv.size() == selected->time.size())
        {
            ImPlot::PlotShaded(
                "##env",
                selected->time.data(),
                selected->minEnv.data(),
                selected->maxEnv.data(),
                n
            );

            ImPlot::PlotLine("##max",
                selected->time.data(),
                selected->maxEnv.data(),
                n);

            ImPlot::PlotLine("##min",
                selected->time.data(),
                selected->minEnv.data(),
                n);

            const double zeroX[2] = {
                selected->time.front(),
                selected->time.back()
            };
            const double zeroY[2] = { 0.0, 0.0 };

            ImPlot::PlotLine("##zero", zeroX, zeroY, 2);
        }

        ImPlot::EndPlot();
    }

    ImPlot::PopStyleColor(2);
    ImPlot::PopStyleVar(1);
}
