#include "EditorLayer.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include "core/Application.h"
#include "core/Window.h"

EditorLayer::EditorLayer(Window& window) : Layer("EditorLayer"), m_Window(window) {
}

void EditorLayer::OnAttach() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    float xscale, yscale;
    glfwGetWindowContentScale(m_Window.GetNativeWindow(), &xscale, &yscale);

    io.Fonts->Clear();

    ImFontConfig fontConfig;
    fontConfig.SizePixels = 16.0f * xscale;
    fontConfig.OversampleH = 3;
    fontConfig.OversampleV = 3;

    io.Fonts->AddFontDefault(&fontConfig);
    // or load from ttf file

    ImGuiStyle style;
    ImGui::GetStyle() = style;

    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(xscale);

    ImGui_ImplGlfw_InitForOpenGL(m_Window.GetNativeWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    EventBus::Subscribe<WindowDpiChangedEvent>(
        [this](WindowDpiChangedEvent& e)
        {
            ApplyDpiScaling(e.GetXScale());
        }
    );
}

void EditorLayer::OnDetach() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorLayer::OnUpdate(float dt) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void EditorLayer::OnImGuiRender() {
    ImGui::Begin("Hello, Wave Tracer!");
    ImGui::Text("Welcome to the editor.");
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorLayer::ApplyDpiScaling(float scale) {
    ImGuiIO& io = ImGui::GetIO();

    // rebuild fonts 
    io.Fonts->Clear();

    ImFontConfig fontConfig;
    fontConfig.SizePixels = 16.0f * scale;
    fontConfig.OversampleH = 3;
    fontConfig.OversampleV = 3;

    io.Fonts->AddFontDefault(&fontConfig);

    // reset style before scaling
    ImGuiStyle style;
    ImGui::GetStyle() = style;

    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(scale);

    // tell renderer to rebuild font textures
    // ImGui_ImplOpenGL3_DestroyFontsTexture();
}