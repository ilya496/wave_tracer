#include "Application.h"
#include "Window.h"

#include "RenderLayer.h"

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

    // Subscribe to events from the EventBus
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