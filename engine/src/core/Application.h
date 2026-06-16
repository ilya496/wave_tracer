#pragma once

#include "wtpch.h"
#include "audio/AudioDeviceContext.h"
#include "audio/AudioTrack.h"

class Window;
class Layer;
using LayerStack = std::vector<Layer*>;

class Application {
public:
    Application();
    virtual ~Application();

    void Run();
    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);

    Window* GetWindow();
    static Application& Get();

    AudioDeviceContext* GetAudioContext() { return m_AudioContext.get(); }
    bool StartSystemRecord(int deviceId, uint32_t sampleRate, uint32_t channels, const std::filesystem::path& targetPath);
    void StopSystemRecord(AudioTrack& targetTrackToFill);
    bool IsSystemRecording() const { return m_IsRecording.load(); }

protected:
    virtual void Shutdown();

private:
    void OnEvent(Event& e);
    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);

    // background worker responsible for streaming out of the ring buffer directly onto storage media
    void StorageWriterThreadWorker(std::filesystem::path outputPath, uint32_t sampleRate, uint32_t channels);

private:
    Scope<Window> m_Window;
    LayerStack m_LayerStack;
    bool m_Running = true;
    bool m_Minimized = false;
    float m_LastFrameTime = 0.0f;

    // audio subsystem components
    Scope<AudioDeviceContext> m_AudioContext;
    Ref<RingBuffer<float>> m_RecordingRingBuffer;

    // thread controls
    std::thread m_StorageWriterThread;
    std::atomic<bool> m_IsRecording{ false };

    // cached record payload info needed to assemble the final dynamic AudioClip structure
    std::filesystem::path m_ActiveOutputFilePath;
    uint32_t m_ActiveRecordSampleRate = 0;
    uint32_t m_ActiveRecordChannels = 0;

    static Application* s_Instance;

};