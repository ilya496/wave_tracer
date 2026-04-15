#pragma once

#include "core/Layer.h"

class EditorLayer : public Layer {
public:
    EditorLayer();
    ~EditorLayer() = default;

    void OnImGuiRender() override;
    void OnAttach() override;
    void OnUpdate(float dt) override;
    void OnDetach() override;
};