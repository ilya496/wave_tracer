#pragma once

#include "wtpch.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

struct WindowProps
{
    std::string title = "Wave Tracer";
    int width = 1920;
    int height = 1080;
    bool VSync = false;
    bool Maximized = true;
};

class Window
{
public:
    Window(const WindowProps& props = {});
    ~Window();

    void OnUpdate();

    void SetTitle(const std::string& title);
    bool IsVSync() const { return m_VSync; }
    void SetVSync(bool enabled);

    struct Vec2 { float x, y; };
    using ivec2 = Vec2;  // Simplified - treating as float for now

    ivec2 GetFramebufferSize() const;
    ivec2 GetPosition() const;
    void SetPosition(int x, int y);

    GLFWwindow* GetNativeWindow() const { return m_Window; }

    void Minimize();
    void Maximize();
    void Restore();

    bool IsFullscreen() const { return m_IsFullscreen; }
    void SetFullscreen(bool enabled);
    bool IsMaximized() const;

    void MakeContextCurrent();
    void DetachContext();
    void PollEvents();
    void SwapBuffers();

    void Close();

private:
    void Init();
    void InitCallbacks();

private:
    GLFWwindow* m_Window = nullptr;
    int m_Width, m_Height;
    std::string m_Title;
    bool m_VSync;

    bool m_IsFullscreen = false;
    GLFWmonitor* m_PreviousMonitor = nullptr;

    Vec2 m_LastMousePos;
};