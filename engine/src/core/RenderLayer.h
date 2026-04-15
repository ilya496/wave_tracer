#pragma once

#include "Layer.h"

class RenderLayer : public Layer
{
public:
    RenderLayer(uint32_t width, uint32_t height);

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(float dt) override;

private:
    uint32_t m_Width, m_Height;

};