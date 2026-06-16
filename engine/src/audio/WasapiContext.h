#pragma once

#include "wtpch.h"
#include "AudioDeviceContext.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class WasapiContext : public AudioDeviceContext {
public:
    WasapiContext();
    virtual ~WasapiContext();

    AudioDeviceAPI GetAPI() const override { return AudioDeviceAPI::Windows_WASAPI; }

    std::vector<AudioDeviceInfo> EnumerateInputDevices() override;

    bool InitializeCapture(int deviceId, uint32_t sampleRate, uint32_t channels, size_t internalBufferSize) override;
    bool StartCapture(Ref<RingBuffer<float>> targetBuffer) override;
    void StopCapture() override;
    bool IsCapturing() const override { return m_IsCapturing.load(); }

private:
    void CaptureThreadWorker();
    ComPtr<IMMDevice> GetDeviceById(int deviceId);
    WAVEFORMATEXTENSIBLE BuildFloatFormat(uint32_t sampleRate, uint32_t channels);

private:
    std::vector<std::wstring> m_DeviceEndpointIds; // Maps int ID to Windows COM string ID

    ComPtr<IAudioClient> m_AudioClient;
    ComPtr<IAudioCaptureClient> m_CaptureClient;

    Ref<RingBuffer<float>> m_TargetBuffer;

    HANDLE m_BufferEvent = nullptr;
    HANDLE m_StopEvent = nullptr;

    std::thread m_CaptureThread;
    std::atomic<bool> m_IsCapturing{ false };

    uint32_t m_CurrentChannels = 0;
};