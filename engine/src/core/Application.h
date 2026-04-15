#pragma once

#include "wtpch.h"

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

protected:
    virtual void Shutdown() {}

private:
    void OnEvent(Event& e);
    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);

    Scope<Window> m_Window;
    LayerStack m_LayerStack;
    // Timer m_Timer;
    bool m_Running = true;
    bool m_Minimized = false;
    float m_LastFrameTime = 0.0f;

private:
    static Application* s_Instance;
};