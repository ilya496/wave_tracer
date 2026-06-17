#pragma once

#include "wtpch.h"

#include "core/Layer.h"
#include "core/Window.h"

class EditorLayer : public Layer {
public:
    EditorLayer(Window& window);
    ~EditorLayer() = default;

    void OnImGuiRender() override;
    void OnAttach() override;
    void OnUpdate(float dt) override;
    void OnDetach() override;

public:
    void HandleDroppedFile(const std::filesystem::path& path);
    void ApplyDpiScaling(float scale);

public:
    Window& m_Window;

    std::filesystem::path m_PendingDropPath;
    bool m_HasPendingDrop = false;

};