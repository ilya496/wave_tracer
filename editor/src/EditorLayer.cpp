#include "EditorLayer.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <GLFW/glfw3.h>

#include "core/Application.h"
#include "core/Window.h"

EditorLayer::EditorLayer(Window& window) : Layer("EditorLayer"), m_Window(window) {
}

void EditorLayer::OnAttach() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

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

    bool dockspaceOpen = true;
    bool optFullscreen = true;

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    if (optFullscreen)
    {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);
        windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Dockspace Window", &dockspaceOpen, windowFlags);
    ImGui::PopStyleVar();

    ImGuiID dockspaceID = ImGui::GetID("MyDockspace");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Project", "Ctrl+S"))
            {
            }

            if (ImGui::MenuItem("Open Project...", "Ctrl+O"))
            {
            }

            if (ImGui::MenuItem("Close Project", "Ctrl+W"))
            {
            }

            if (ImGui::MenuItem("Export Simulation", "Ctrl+E"))
            {
            }

            if (ImGui::MenuItem("Exit"))
                EventBus::Publish(WindowCloseEvent{});

            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void EditorLayer::OnImGuiRender() {
    ImGui::Begin("Hello, Wave Tracer!");
    ImGui::Text("Welcome to the editor.");

    if (ImPlot::BeginPlot("Example")) {
        double x[] = { 1, 2, 3, 4, 5 };
        double y[] = { 1, 2, 3, 4, 5 };
        ImPlot::PlotLine("My Line", x, y, 5);
        ImPlot::EndPlot();
    }
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