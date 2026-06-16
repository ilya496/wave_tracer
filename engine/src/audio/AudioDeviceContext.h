#pragma once

#include "wtpch.h"
#include "AudioDevice.h"
#include "core/RingBuffer.h"

class AudioDeviceContext {
public:
    virtual ~AudioDeviceContext() = default;

    virtual AudioDeviceAPI GetAPI() const = 0;
    virtual std::vector<AudioDeviceInfo> EnumerateInputDevices() = 0;

    // configures and links a physical device to an active recording capture ring buffer
    virtual bool InitializeCapture(int deviceId, uint32_t sampleRate, uint32_t channels, size_t internalBufferSize) = 0;
    virtual bool StartCapture(Ref<RingBuffer<float>> targetBuffer) = 0;
    virtual void StopCapture() = 0;
    virtual bool IsCapturing() const = 0;

    // factory method to initialize the proper platform context instance natively
    static Scope<AudioDeviceContext> Create();
};