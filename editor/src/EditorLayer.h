#pragma once

#include "wtpch.h"

#include "core/Layer.h"
#include "core/Window.h"

#include "WaveformPanel.h"

class EditorLayer : public Layer {
public:
    EditorLayer(Window& window);
    ~EditorLayer() = default;

    void OnImGuiRender() override;
    void OnAttach() override;
    void OnUpdate(float dt) override;
    void OnDetach() override;

public:
    void HandleDroppedFile(const std::string& path);
    void ApplyDpiScaling(float scale);

public:
    Window& m_Window;
    WaveformPanel m_WaveformPanel;

    std::string m_PendingDropPath;
    bool m_HasPendingDrop = false;

};