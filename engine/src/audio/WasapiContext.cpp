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
            for (uint32_t rate : ratesToTest) {
                // test if the device supports this rate in mono or stereo
                WAVEFORMATEXTENSIBLE fmtMono = BuildFloatFormat(rate, 1);
                WAVEFORMATEXTENSIBLE fmtStereo = BuildFloatFormat(rate, 2);

                HRESULT hrMono = testClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &fmtMono.Format, nullptr);
                HRESULT hrStereo = testClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &fmtStereo.Format, nullptr);

                if (hrMono == S_OK || hrStereo == S_OK) {
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
    // default speaker mapping: mono or stereo
    wfext.dwChannelMask = (channels == 1) ? SPEAKER_FRONT_CENTER : (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT);
    wfext.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

    return wfext;
}

bool WasapiContext::InitializeCapture(int deviceId, uint32_t sampleRate, uint32_t channels, size_t internalBufferSize) {
    if (deviceId < 0 || deviceId >= m_DeviceEndpointIds.size()) return false;

    ComPtr<IMMDevice> device = GetDeviceById(deviceId);
    if (!device) return false;

    HRESULT hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_AudioClient);
    if (FAILED(hr)) return false;

    // --- DEBUG BLOCK: Ask Windows what the device natively wants ---
    WAVEFORMATEX* pwfx = nullptr;
    m_AudioClient->GetMixFormat(&pwfx);
    if (pwfx) {
        std::cout << "\n[WASAPI DEBUG] Device Native Format:\n";
        std::cout << "  Channels: " << pwfx->nChannels << "\n";
        std::cout << "  Sample Rate: " << pwfx->nSamplesPerSec << "\n";
        std::cout << "  Bits per Sample: " << pwfx->wBitsPerSample << "\n";
        if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
            WAVEFORMATEXTENSIBLE* pEx = (WAVEFORMATEXTENSIBLE*)pwfx;
            std::cout << "  Valid Bits per Sample: " << pEx->Samples.wValidBitsPerSample << "\n";
            if (pEx->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) std::cout << "  Format: 32-bit Float\n";
            else std::cout << "  Format: PCM Integer\n";
        }
        CoTaskMemFree(pwfx);
    }
    // ---------------------------------------------------------------

    WAVEFORMATEXTENSIBLE format = BuildFloatFormat(sampleRate, channels);
    m_CurrentChannels = channels;

    // REFERENCE_TIME is 100-nanosecond units. 
    // Example: 10 milliseconds = 100,000 units.
    REFERENCE_TIME hnsRequestedDuration = 100000;

    // initialize in EXCLUSIVE mode, requesting Event Driven buffering
    hr = m_AudioClient->Initialize(
        AUDCLNT_SHAREMODE_EXCLUSIVE,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        hnsRequestedDuration,
        hnsRequestedDuration,
        &format.Format,
        nullptr
    );

    if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
        std::cerr << "[WASAPI] Hardware rejected the format. Check if device sample rate matches requested rate.\n";
        return false;
    }
    if (FAILED(hr)) return false;

    // create the threading events
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
    // vital! new threads must initialize COM to use COM interfaces
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    // bump thread priority to Real-Time to prevent audio dropouts
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
                    // hardware requested silence (write zeros to ring buffer)
                    std::vector<float> silence(numFramesAvailable * m_CurrentChannels, 0.0f);
                    m_TargetBuffer->Write(silence.data(), silence.size());
                }
                else {
                    // we know we requested KSDATAFORMAT_SUBTYPE_IEEE_FLOAT (32-bit float)
                    // we can directly cast the byte array and push it to the ring buffer
                    float* pSamples = reinterpret_cast<float*>(pData);
                    size_t totalSamples = (size_t)numFramesAvailable * m_CurrentChannels;
                    m_TargetBuffer->Write(pSamples, totalSamples);
                }

                hr = m_CaptureClient->ReleaseBuffer(numFramesAvailable);
                hr = m_CaptureClient->GetNextPacketSize(&packetLength);
            }
        }
    }

    CoUninitialize();
}