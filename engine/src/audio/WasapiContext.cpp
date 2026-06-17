#include "WasapiContext.h"
#include <functiondiscoverykeys_devpkey.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "Mmdevapi.lib")

WasapiContext::WasapiContext() {
    // initialize COM for the main application thread
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
}

WasapiContext::~WasapiContext() {
    StopCapture();
    if (m_BufferEvent) CloseHandle(m_BufferEvent);
    if (m_StopEvent) CloseHandle(m_StopEvent);
    if (m_ActiveFormat) CoTaskMemFree(m_ActiveFormat);
    CoUninitialize();
}

std::vector<AudioDeviceInfo> WasapiContext::EnumerateInputDevices() {
    std::vector<AudioDeviceInfo> devices;
    m_DeviceEndpointIds.clear();

    ComPtr<IMMDeviceEnumerator> enumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
    if (FAILED(hr)) return devices;

    ComPtr<IMMDeviceCollection> collection;
    // eCapture = Inputs only, DEVICE_STATE_ACTIVE = plugged in and enabled
    enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &collection);

    UINT count;
    collection->GetCount(&count);

    // common sample rates
    const std::vector<uint32_t> ratesToTest = { 44100, 48000, 88200, 96000, 176400, 192000 };

    for (UINT i = 0; i < count; ++i) {
        ComPtr<IMMDevice> device;
        collection->Item(i, &device);

        // get the internal Windows ID
        LPWSTR wstrId = nullptr;
        device->GetId(&wstrId);
        m_DeviceEndpointIds.push_back(wstrId);

        // get the human-readable name
        ComPtr<IPropertyStore> props;
        device->OpenPropertyStore(STGM_READ, &props);
        PROPVARIANT varName;
        PropVariantInit(&varName);
        props->GetValue(PKEY_Device_FriendlyName, &varName);

        // convert UTF-16 wstring to std::string
        std::wstring ws(varName.pwszVal);
        std::string friendlyName(ws.begin(), ws.end());
        PropVariantClear(&varName);
        CoTaskMemFree(wstrId);

        AudioDeviceInfo info;
        info.id = i;
        info.name = friendlyName;
        info.isDefault = (i == 0); // temporary simplified default check

        // query supported sample rates
        ComPtr<IAudioClient> testClient;
        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&testClient);
        if (SUCCEEDED(hr)) {
            // query the device's native channel layout
            WAVEFORMATEX* mixFormat = nullptr;
            uint32_t nativeChannels = 2;
            if (SUCCEEDED(testClient->GetMixFormat(&mixFormat))) {
                nativeChannels = mixFormat->nChannels;
                CoTaskMemFree(mixFormat);
            }

            // build our channel test list dynamically
            std::vector<uint32_t> channelsToTest = { 1, 2 };
            if (nativeChannels > 2) {
                channelsToTest.push_back(nativeChannels);
            }

            // query supported rates against the dynamic channel list
            for (uint32_t rate : ratesToTest) {
                bool rateSupported = false;

                for (uint32_t channels : channelsToTest) {
                    WAVEFORMATEXTENSIBLE fmt = BuildFloatFormat(rate, channels);

                    // test exclusive mode
                    HRESULT hrExcl = testClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &fmt.Format, nullptr);

                    if (hrExcl == S_OK) {
                        rateSupported = true;
                        break;
                    }

                    // check if the shared mode engine can handle/resample this rate
                    WAVEFORMATEX* closestMatch = nullptr;
                    HRESULT hrShared = testClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &fmt.Format, &closestMatch);
                    if (closestMatch) CoTaskMemFree(closestMatch);

                    if (hrShared == S_OK || hrShared == S_FALSE) { // S_FALSE means engine accepts it with internal resampling
                        rateSupported = true;
                        break;
                    }
                }

                if (rateSupported) {
                    info.supportedSampleRates.push_back(rate);
                }
            }
        }

        devices.push_back(info);
    }

    return devices;
}

ComPtr<IMMDevice> WasapiContext::GetDeviceById(int deviceId) {
    ComPtr<IMMDeviceEnumerator> enumerator;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));

    ComPtr<IMMDevice> device;
    enumerator->GetDevice(m_DeviceEndpointIds[deviceId].c_str(), &device);
    return device;
}

WAVEFORMATEXTENSIBLE WasapiContext::BuildFloatFormat(uint32_t sampleRate, uint32_t channels) {
    WAVEFORMATEXTENSIBLE wfext = {};
    wfext.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfext.Format.nChannels = (WORD)channels;
    wfext.Format.nSamplesPerSec = sampleRate;
    wfext.Format.wBitsPerSample = 32; // 32-bit float
    wfext.Format.nBlockAlign = (wfext.Format.nChannels * wfext.Format.wBitsPerSample) / 8;
    wfext.Format.nAvgBytesPerSec = wfext.Format.nSamplesPerSec * wfext.Format.nBlockAlign;
    wfext.Format.cbSize = 22; // size of the extensible payload

    wfext.Samples.wValidBitsPerSample = 32;

    // handle multi - channel arrays cleanly
    if (channels == 1) {
        wfext.dwChannelMask = SPEAKER_FRONT_CENTER;
    }
    else if (channels == 2) {
        wfext.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    }
    else {
        wfext.dwChannelMask = 0; // Explicitly tells Windows to use native microphone array routing
    };

    wfext.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    return wfext;
}

bool WasapiContext::InitializeCapture(int deviceId, uint32_t sampleRate, uint32_t channels, size_t internalBufferSize) {
    if (deviceId < 0 || deviceId >= m_DeviceEndpointIds.size()) return false;

    ComPtr<IMMDevice> device = GetDeviceById(deviceId);
    if (!device) return false;

    HRESULT hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_AudioClient);
    if (FAILED(hr)) return false;

    if (m_ActiveFormat) {
        CoTaskMemFree(m_ActiveFormat);
        m_ActiveFormat = nullptr;
    }

    m_ActiveFormat = NegotiateFormat(m_AudioClient.Get(), sampleRate, channels);

    REFERENCE_TIME hnsRequestedDuration = 100000; // 10ms
    bool useExclusive = (m_ActiveFormat != nullptr);

    if (useExclusive) {
        // attempt EXCLUSIVE mode initialization
        hr = m_AudioClient->Initialize(
            AUDCLNT_SHAREMODE_EXCLUSIVE,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
            hnsRequestedDuration,
            hnsRequestedDuration,
            m_ActiveFormat,
            nullptr
        );
    }
    else {
        // if negotiation failed entirely, force fallback evaluation
        hr = 0x8889000A;
    }

    // 3. if exclusive mode is rejected or device is busy, pivot to SHARED mode
    if (hr == 0x8889000A) { // AUDCLNT_E_DEVICE_IN_USE
        std::cout << "[WASAPI] Exclusive mode blocked (Device In Use). Falling back to Shared Mode...\n";

        if (m_ActiveFormat) {
            CoTaskMemFree(m_ActiveFormat);
            m_ActiveFormat = nullptr;
        }

        // shared mode strictly requires using the Windows mix format engine
        hr = m_AudioClient->GetMixFormat(&m_ActiveFormat);
        if (FAILED(hr)) return false;

        // force engine alignment to whatever layout the shared engine is running
        m_CurrentChannels = m_ActiveFormat->nChannels;

        hr = m_AudioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
            0, // Must be 0 for Shared Mode event-driven operations
            0, // Must be 0 for Shared Mode event-driven operations
            m_ActiveFormat,
            nullptr
        );
    }

    if (FAILED(hr)) {
        std::cerr << "[WASAPI] Initialization failed completely with HRESULT: 0x" << std::hex << hr << std::dec << "\n";
        return false;
    }

    // update sample rate and channels for the processing worker thread
    m_CurrentChannels = m_ActiveFormat->nChannels;
    m_CurrentSampleRate = m_ActiveFormat->nSamplesPerSec;

    // create the threading loop synchronization handles
    m_BufferEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    m_StopEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    hr = m_AudioClient->SetEventHandle(m_BufferEvent);
    if (FAILED(hr)) return false;

    hr = m_AudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&m_CaptureClient);
    return SUCCEEDED(hr);
}

bool WasapiContext::StartCapture(Ref<RingBuffer<float>> targetBuffer) {
    if (m_IsCapturing.load()) return false;

    m_TargetBuffer = targetBuffer;
    m_IsCapturing.store(true);
    ResetEvent(m_StopEvent);

    m_CaptureThread = std::thread(&WasapiContext::CaptureThreadWorker, this);

    HRESULT hr = m_AudioClient->Start();
    if (FAILED(hr)) {
        StopCapture();
        return false;
    }

    return true;
}

void WasapiContext::StopCapture() {
    if (!m_IsCapturing.load()) return;

    m_IsCapturing.store(false);
    SetEvent(m_StopEvent);

    if (m_CaptureThread.joinable()) {
        m_CaptureThread.join();
    }

    if (m_AudioClient) {
        m_AudioClient->Stop();
        m_AudioClient->Reset();
    }
}

void WasapiContext::CaptureThreadWorker() {
    // new threads must initialize COM to use COM interfaces
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    // bump thread priority to real-time to prevent audio dropouts
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    HANDLE waitArray[2] = { m_StopEvent, m_BufferEvent };

    while (m_IsCapturing.load()) {
        // sleep until either the stop event or audio buffer event is fired
        DWORD waitResult = WaitForMultipleObjects(2, waitArray, FALSE, INFINITE);

        if (waitResult == WAIT_OBJECT_0) break;

        // if the buffer event fired, drain the hardware buffer
        if (waitResult == WAIT_OBJECT_0 + 1) {
            UINT32 packetLength = 0;
            HRESULT hr = m_CaptureClient->GetNextPacketSize(&packetLength);

            while (packetLength != 0 && SUCCEEDED(hr)) {
                BYTE* pData;
                UINT32 numFramesAvailable;
                DWORD flags;

                hr = m_CaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
                if (FAILED(hr)) break;

                // handle data extraction
                if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                    // hardware requested silence - write zeros to buffer
                    std::vector<float> silence(numFramesAvailable * m_CurrentChannels, 0.0f);
                    m_TargetBuffer->Write(silence.data(), silence.size());
                }
                else {
                    size_t totalSamples = (size_t)numFramesAvailable * m_CurrentChannels;

                    // check which format we negotiated
                    bool isFloat = false;
                    uint16_t bitsPerSample = m_ActiveFormat->wBitsPerSample;

                    if (m_ActiveFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
                        isFloat = true;
                    }
                    else if (m_ActiveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
                        WAVEFORMATEXTENSIBLE* pEx = (WAVEFORMATEXTENSIBLE*)m_ActiveFormat;
                        if (pEx->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) isFloat = true;
                    }

                    if (isFloat) {
                        float* pSamples = reinterpret_cast<float*>(pData);
                        m_TargetBuffer->Write(pSamples, totalSamples);
                    }
                    else {
                        // hardware gave us PCM integer samples. Convert them to float in the
                        // range [-1.0f, 1.0f) based on the bit depth that was negotiated.
                        std::vector<float> convertedSamples(totalSamples);
                        constexpr float kInt32ToFloat = 1.0f / 2147483648.0f; // 2^31
                        constexpr float kInt24ToFloat = 1.0f / 8388608.0f;    // 2^23
                        constexpr float kInt16ToFloat = 1.0f / 32768.0f;      // 2^15

                        if (bitsPerSample == 32) {
                            int32_t* pIntSamples = reinterpret_cast<int32_t*>(pData);
                            for (size_t i = 0; i < totalSamples; i++) {
                                convertedSamples[i] = static_cast<float>(pIntSamples[i]) * kInt32ToFloat;
                            }
                        }
                        else if (bitsPerSample == 24) {
                            uint8_t* pByteData = reinterpret_cast<uint8_t*>(pData);
                            for (size_t i = 0; i < totalSamples; i++) {
                                size_t base = i * 3;
                                // reconstruct 24-bit little-endian signed integer.
                                // the highest byte is sign-extended via cast to int8_t.
                                int32_t sample = (pByteData[base] |
                                    (pByteData[base + 1] << 8) |
                                    (static_cast<int32_t>(static_cast<int8_t>(pByteData[base + 2])) << 16));
                                convertedSamples[i] = static_cast<float>(sample) * kInt24ToFloat;
                            }
                        }
                        else if (bitsPerSample == 16) {
                            int16_t* pIntSamples = reinterpret_cast<int16_t*>(pData);
                            for (size_t i = 0; i < totalSamples; i++) {
                                convertedSamples[i] = static_cast<float>(pIntSamples[i]) * kInt16ToFloat;
                            }
                        }

                        m_TargetBuffer->Write(convertedSamples.data(), totalSamples);
                    }
                }

                hr = m_CaptureClient->ReleaseBuffer(numFramesAvailable);
                hr = m_CaptureClient->GetNextPacketSize(&packetLength);
            }
        }
    }

    CoUninitialize();
}

WAVEFORMATEX* WasapiContext::NegotiateFormat(IAudioClient* client, uint32_t sampleRate, uint32_t channels) {
    DWORD channelMask = 0;
    if (channels == 1) channelMask = SPEAKER_FRONT_CENTER;
    else if (channels == 2) channelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    else channelMask = 0; // safe fallback for 4+ channel microphone arrays

    // 32-bit Float Extensible
    WAVEFORMATEXTENSIBLE wfxFloatExt = BuildFloatFormat(sampleRate, channels);
    if (client->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxFloatExt.Format, nullptr) == S_OK) {
        std::cout << "[WASAPI] Hardware accepted 32-bit Float Extensible.\n";
        WAVEFORMATEX* res = (WAVEFORMATEX*)CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
        memcpy(res, &wfxFloatExt, sizeof(WAVEFORMATEXTENSIBLE));
        return res;
    }

    // 24/32-bit PCM Integer Extensible
    WAVEFORMATEXTENSIBLE wfxIntExt = {};
    wfxIntExt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfxIntExt.Format.nChannels = (WORD)channels;
    wfxIntExt.Format.nSamplesPerSec = sampleRate;
    wfxIntExt.Format.wBitsPerSample = 32;
    wfxIntExt.Format.nBlockAlign = (channels * 32) / 8;
    wfxIntExt.Format.nAvgBytesPerSec = sampleRate * wfxIntExt.Format.nBlockAlign;
    wfxIntExt.Format.cbSize = 22;
    wfxIntExt.Samples.wValidBitsPerSample = 24;
    wfxIntExt.dwChannelMask = channelMask;
    wfxIntExt.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    if (client->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxIntExt.Format, nullptr) == S_OK) {
        std::cout << "[WASAPI] Hardware accepted 24/32-bit Integer PCM Extensible.\n";
        WAVEFORMATEX* res = (WAVEFORMATEX*)CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
        memcpy(res, &wfxIntExt, sizeof(WAVEFORMATEXTENSIBLE));
        return res;
    }

    // 16-bit PCM Integer Extensible
    WAVEFORMATEXTENSIBLE wfxInt16Ext = {};
    wfxInt16Ext.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfxInt16Ext.Format.nChannels = (WORD)channels;
    wfxInt16Ext.Format.nSamplesPerSec = sampleRate;
    wfxInt16Ext.Format.wBitsPerSample = 16;
    wfxInt16Ext.Format.nBlockAlign = (channels * 16) / 8;
    wfxInt16Ext.Format.nAvgBytesPerSec = sampleRate * wfxInt16Ext.Format.nBlockAlign;
    wfxInt16Ext.Format.cbSize = 22;
    wfxInt16Ext.Samples.wValidBitsPerSample = 16;
    wfxInt16Ext.dwChannelMask = channelMask;
    wfxInt16Ext.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    if (client->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxInt16Ext.Format, nullptr) == S_OK) {
        std::cout << "[WASAPI] Hardware accepted 16-bit Integer PCM Extensible.\n";
        WAVEFORMATEX* res = (WAVEFORMATEX*)CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
        memcpy(res, &wfxInt16Ext, sizeof(WAVEFORMATEXTENSIBLE));
        return res;
    }

    // fallbacks for legacy 1-2 channel devices below
    if (channels <= 2) {
        // standard 32-bit Float (Legacy)
        WAVEFORMATEX wfxFloatStd = {};
        wfxFloatStd.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        wfxFloatStd.nChannels = (WORD)channels;
        wfxFloatStd.nSamplesPerSec = sampleRate;
        wfxFloatStd.wBitsPerSample = 32;
        wfxFloatStd.nBlockAlign = (channels * 32) / 8;
        wfxFloatStd.nAvgBytesPerSec = sampleRate * wfxFloatStd.nBlockAlign;
        wfxFloatStd.cbSize = 0;

        if (client->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxFloatStd, nullptr) == S_OK) {
            std::cout << "[WASAPI] Hardware accepted Standard 32-bit Float.\n";
            WAVEFORMATEX* res = (WAVEFORMATEX*)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
            memcpy(res, &wfxFloatStd, sizeof(WAVEFORMATEX));
            return res;
        }

        // standard 16-bit Integer PCM (Legacy)
        WAVEFORMATEX wfxInt16Std = {};
        wfxInt16Std.wFormatTag = WAVE_FORMAT_PCM;
        wfxInt16Std.nChannels = (WORD)channels;
        wfxInt16Std.nSamplesPerSec = sampleRate;
        wfxInt16Std.wBitsPerSample = 16;
        wfxInt16Std.nBlockAlign = (channels * 16) / 8;
        wfxInt16Std.nAvgBytesPerSec = sampleRate * wfxInt16Std.nBlockAlign;
        wfxInt16Std.cbSize = 0;

        if (client->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &wfxInt16Std, nullptr) == S_OK) {
            std::cout << "[WASAPI] Hardware accepted Standard 16-bit Integer PCM.\n";
            WAVEFORMATEX* res = (WAVEFORMATEX*)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
            memcpy(res, &wfxInt16Std, sizeof(WAVEFORMATEX));
            return res;
        }
    }

    return nullptr;
}