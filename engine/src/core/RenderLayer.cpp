#include "RenderLayer.h"

#include "wtpch.h"
#include "glad/glad.h"
#include <GLFW/glfw3.h>

RenderLayer::RenderLayer(uint32_t width, uint32_t height)
    : Layer("RenderLayer"), m_Width(width), m_Height(height)
{
}

void RenderLayer::OnAttach()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return;
    }
}

void RenderLayer::OnDetach()
{
}

void RenderLayer::OnUpdate(float dt)
{
    glViewport(0, 0, m_Width, m_Height);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}