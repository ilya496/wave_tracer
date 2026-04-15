#include "EditorLayer.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "core/Application.h"
#include "core/Window.h"

EditorLayer::EditorLayer() : Layer("EditorLayer") {}

void EditorLayer::OnAttach() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)Application::Get().GetWindow()->GetNativeWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");
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
