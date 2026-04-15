#include "Window.h"

Window::Window(const std::string& title, uint32_t width, uint32_t height, bool vSync, bool maximized)
    : m_Title(title), m_Width(width), m_Height(height), m_VSync(vSync), m_Maximized(maximized) {
    Init();
}

Window::Window(const WindowProps& props)
    : m_Title(props.Title), m_Width(props.Width), m_Height(props.Height),
    m_VSync(props.VSync), m_Maximized(props.Maximized) {
    Init();
}