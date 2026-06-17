#include "Application.h"
#include "Window.h"

#include "RenderLayer.h"
#include "audio/WasapiContext.h"

// Dummy file factory for platform generation - to be linked to WASAPI/ALSA units later
Scope<AudioDeviceContext> AudioDeviceContext::Create() {
#if defined(_WIN32)
#undef min
    // Return a std::make_unique<WasapiContext>();
    return CreateScope<WasapiContext>();
#elif defined(__linux__)
    // Return a std::make_unique<AlsaContext>();
    return nullptr;
#else
    return nullptr;
#endif
}

Application* Application::s_Instance = nullptr;

Application::Application() {
    s_Instance = this;

    WindowProps props;
    props.title = "Wave Tracer";
    props.width = 1920;
    props.height = 1080;

    m_Window = CreateScope<Window>(props);

    RenderLayer* renderLayer = new RenderLayer(props.width, props.height);
    PushLayer(renderLayer);

    // initialize platform-native hardware context
    m_AudioContext = AudioDeviceContext::Create();

    // subscribe to events from the EventBus
    EventBus::Subscribe<WindowCloseEvent>([this](Event& e) {
        OnWindowClose(static_cast<WindowCloseEvent&>(e));
        });
    EventBus::Subscribe<WindowResizeEvent>([this](Event& e) {
        OnWindowResize(static_cast<WindowResizeEvent&>(e));
        });
}

Application::~Application() {
    std::cout << "Shutting down application...\n";
}

void Application::Run() {
    std::cout << "Application running...\n";
    while (m_Running) {
        // float time = m_Timer.GetTime();
        // float deltaTime = time - m_LastFrameTime;
        // m_LastFrameTime = time;

        // if (!m_Minimized) {
        for (Layer* layer : m_LayerStack) {
            layer->OnUpdate(0.0f);  // TODO: deltaTime
        }

        for (Layer* layer : m_LayerStack)
            layer->OnImGuiRender();

        m_Window->OnUpdate();
    }
}

Application& Application::Get() {
    return *s_Instance;
}

Window* Application::GetWindow() {
    return m_Window.get();
}

void Application::PushLayer(Layer* layer) {
    m_LayerStack.push_back(layer);
    layer->OnAttach();
    std::cout << "Layer " << layer->GetName() << " attached\n";
}

void Application::PushOverlay(Layer* overlay) {
    m_LayerStack.push_back(overlay);  // For simplicity, overlays also at end, but reverse iteration
    overlay->OnAttach();
}

void Application::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>(std::bind(&Application::OnWindowClose, this, std::placeholders::_1));
    dispatcher.Dispatch<WindowResizeEvent>(std::bind(&Application::OnWindowResize, this, std::placeholders::_1));

    for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it) {
        (*it)->OnEvent(e);
        if (e.Handled)
            break;
    }
}

bool Application::OnWindowClose(WindowCloseEvent& e) {
    m_Running = false;
    return true;
}

bool Application::OnWindowResize(WindowResizeEvent& e) {
    // if (e.GetWidth() == 0 || e.GetHeight() == 0) {
    //     m_Minimized = true;
    //     return false;
    // }
    // m_Minimized = false;

    // TODO: Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());
    return false;
}

bool Application::StartSystemRecord(int deviceId, uint32_t requestedRate, uint32_t requestedChannels, const std::filesystem::path& targetPath) {
    if (m_IsRecording.load() || !m_AudioContext) return false;

    // 1. Initialize the hardware context FIRST so we can discover what Windows actually allowed
    if (!m_AudioContext->InitializeCapture(deviceId, requestedRate, requestedChannels, 512)) {
        std::cerr << "[Application]: Failed to initialize hardware capture lines.\n";
        return false;
    }

    // 2. Extract the actual reality of our hardware configuration
    uint32_t actualRate = m_AudioContext->GetActiveSampleRate();
    uint32_t actualChannels = m_AudioContext->GetActiveChannels();

    std::cout << "[Application] Initialized hardware. Requested: " << requestedRate << "Hz/" << requestedChannels
        << "ch -> Actual: " << actualRate << "Hz/" << actualChannels << "ch\n";

    // 3. Allocate safety cushion padding inside the ring buffer based on ACTUAL layout
    size_t ringBufferCapacity = static_cast<size_t>(actualRate) * actualChannels * 2;

    // Round to the next power of two for optimal performance
    size_t powerOfTwo = 1;
    while (powerOfTwo < ringBufferCapacity) powerOfTwo <<= 1;

    m_RecordingRingBuffer = CreateRef<RingBuffer<float>>(powerOfTwo);

    // 4. Record state using the ACTUAL values so WAV writers downstream get the proper settings
    m_ActiveOutputFilePath = targetPath;
    m_ActiveRecordSampleRate = actualRate;
    m_ActiveRecordChannels = actualChannels;
    m_IsRecording.store(true);

    // 5. Kickstart the background disk pipeline thread with ACTUAL hardware parameters
    m_StorageWriterThread = std::thread(&Application::StorageWriterThreadWorker, this, targetPath, actualRate, actualChannels);

    // 6. Safely open up the native hardware capture callbacks
    if (!m_AudioContext->StartCapture(m_RecordingRingBuffer)) {
        std::cerr << "[Application]: Failed to start hardware capture thread loop.\n";
        m_IsRecording.store(false);
        if (m_StorageWriterThread.joinable()) m_StorageWriterThread.join();
        return false;
    }

    return true;
}

void Application::StopSystemRecord(AudioTrack& targetTrackToFill) {
    if (!m_IsRecording.load()) return;

    // instantly silence incoming hardware delivery lines
    m_AudioContext->StopCapture();

    // flag background disk execution to clean up and exit
    m_IsRecording.store(false);

    if (m_StorageWriterThread.joinable()) {
        m_StorageWriterThread.join();
    }

    // since writing is complete, read back metadata from disk using the decoder to fill the UI track
    // targetTrackToFill.clip = AudioImporter::Import(m_ActiveOutputFilePath);
    std::cout << "[Application] Threaded capture session completed successfully.\n";
}

void Application::StorageWriterThreadWorker(std::filesystem::path outputPath, uint32_t sampleRate, uint32_t channels) {
    std::cout << "[Storage Thread] Disk serialization thread spawned successfully.\n";

    // // open the raw target recording layout using your engine's existing exporter tooling structures
    // // for raw prototyping or missing writer tools, we can construct standard linear file headers:
    // std::ofstream file(outputPath, std::ios::binary);
    // if (!file.is_open()) {
    //     m_IsRecording.store(false);
    //     return;
    // }

    // // Placeholder: Write mock wav structure headers or call direct dr_wav generation functions here
    // // drwav_init_file_write(), etc...

    // std::vector<float> syncProcessingBuffer(4096);

    // while (m_IsRecording.load() || m_RecordingRingBuffer->GetAvailableRead() > 0) {
    //     size_t readyFrames = m_RecordingRingBuffer->GetAvailableRead();
    //     if (readyFrames == 0) {
    //         // no data yet; yield CPU execution back to the OS scheduler for a millisecond
    //         std::this_thread::sleep_for(std::chrono::microseconds(1));
    //         continue;
    //     }

    //     size_t sampleToRead = std::min(syncProcessingBuffer.size(), readyFrames);
    //     size_t readCount = m_RecordingRingBuffer->Read(syncProcessingBuffer.data(), sampleToRead);

    //     if (readCount > 0) {
    //         // write raw PCM sample blocks directly to storage disk media
    //         file.write(reinterpret_cast<const char*>(syncProcessingBuffer.data()), readCount * sizeof(float));
    //     }
    // }

    // file.close();

    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) return;

    // 1. Write a placeholder WAV header (44 bytes)
    // We will come back and overwrite the file size at the end!
    uint32_t dataSize = 0;
    uint32_t chunkSize = 36; // 36 + dataSize
    uint32_t byteRate = sampleRate * channels * sizeof(float);
    uint16_t blockAlign = channels * sizeof(float);

    file.write("RIFF", 4);
    file.write((char*)&chunkSize, 4);
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    uint32_t fmtSize = 16; file.write((char*)&fmtSize, 4);
    uint16_t audioFormat = 3; file.write((char*)&audioFormat, 2); // 3 = IEEE Float
    file.write((char*)&channels, 2);
    file.write((char*)&sampleRate, 4);
    file.write((char*)&byteRate, 4);
    file.write((char*)&blockAlign, 2);
    uint16_t bitsPerSample = 32; file.write((char*)&bitsPerSample, 2);
    file.write("data", 4);
    file.write((char*)&dataSize, 4);

    // 2. The Capture Loop
    std::vector<float> syncBuffer(4096);
    size_t totalSamplesWritten = 0;

    while (m_IsRecording.load() || m_RecordingRingBuffer->GetAvailableRead() > 0) {
        size_t readyFrames = m_RecordingRingBuffer->GetAvailableRead();
        if (readyFrames == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        size_t toRead = std::min(syncBuffer.size(), readyFrames);
        size_t readCount = m_RecordingRingBuffer->Read(syncBuffer.data(), toRead);

        if (readCount > 0) {
            file.write(reinterpret_cast<const char*>(syncBuffer.data()), readCount * sizeof(float));
            totalSamplesWritten += readCount;
        }
    }

    // 3. Fix the header sizes now that we know how much we recorded
    dataSize = totalSamplesWritten * sizeof(float);
    chunkSize = 36 + dataSize;

    file.seekp(4, std::ios::beg); // Jump to ChunkSize
    file.write((char*)&chunkSize, 4);
    file.seekp(40, std::ios::beg); // Jump to DataSize
    file.write((char*)&dataSize, 4);

    file.close();
    std::cout << "[Storage Thread] Disk serialization thread terminated cleanly.\n";
}

void Application::Shutdown() {
    if (m_IsRecording.load()) {
        m_AudioContext->StopCapture();
        m_IsRecording.store(false);
        if (m_StorageWriterThread.joinable()) m_StorageWriterThread.join();
    }
}