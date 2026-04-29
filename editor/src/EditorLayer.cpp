#include "EditorLayer.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <GLFW/glfw3.h>

#include "core/Application.h"
#include "core/Window.h"

#include "import/AudioImporter.h"

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
    glfwSetDropCallback(m_Window.GetNativeWindow(), nullptr);

    ImPlot::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorLayer::OnUpdate(float dt)
{
    auto dropped = m_Window.ConsumeDroppedFiles();

    for (auto& path : dropped)
    {
        HandleDroppedFile(path);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    bool dockspaceOpen = true;
    ImGui::Begin("Dockspace Window", &dockspaceOpen, windowFlags);
    ImGui::PopStyleVar();

    ImGuiID dockspaceID = ImGui::GetID("MyDockspace");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open Audio File...", "Ctrl+O"))
            {
                // TODO: native file dialog (e.g. nfd / pfd)
            }
            if (ImGui::MenuItem("Save Project", "Ctrl+S")) {}
            if (ImGui::MenuItem("Close Project", "Ctrl+W")) {}
            if (ImGui::MenuItem("Export", "Ctrl+E")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
                EventBus::Publish(WindowCloseEvent{});
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}


void EditorLayer::OnImGuiRender() {
    m_WaveformPanel.OnImGuiRender();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorLayer::HandleDroppedFile(const std::filesystem::path& path)
{
    std::cout << "[EditorLayer] Importing: " << path << "\n";

    Ref<AudioClip> clip = AudioImporter::Import(path);
    if (!clip)
    {
        std::cerr << "[EditorLayer] Import failed for: " << path << "\n";
        return;
    }

    std::cout << "[EditorLayer] Loaded "
        << clip->GetFrameCount() << " frames @ "
        << clip->GetSampleRate() << " Hz, "
        << clip->GetChannels() << " ch, "
        << clip->GetDuration() << "s\n";

    m_WaveformPanel.LoadClip(clip);
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
}