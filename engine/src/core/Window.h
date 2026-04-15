#pragma once

#include "wtpch.h"
#include <GLFW/glfw3.h>

struct WindowProps
{
    std::string Title = "Wave Tracer";
    uint32_t Width = 1920;
    uint32_t Height = 1080;
    bool VSync = false;
    bool Maximized = true;
};

class Window
{
public:
    Window(const std::string& title, uint32_t width, uint32_t height, bool vSync = false, bool maximized = true);
    Window(const WindowProps& props = {});
    ~Window();

public:
    void OnUpdate();

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

private:
    void Init();

private:
    GLFWwindow* m_Window;
    uint32_t m_Width, m_Height;
    std::string m_Title;
    bool m_VSync;
    bool m_Maximized;
    bool m_Fullscreen = false;
};