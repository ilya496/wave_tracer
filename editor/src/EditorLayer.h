#pragma once

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
    void ApplyDpiScaling(float scale);

public:
    Window& m_Window;
};