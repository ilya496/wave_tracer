#include "core/Entrypoint.h"
#include "EditorLayer.h"

class Editor : public Application
{
public:
    Editor() : Application()
    {
        PushLayer(new EditorLayer());
    }

    virtual void Shutdown() override
    {
        Application::Shutdown();
    }

    ~Editor() {}
};

Application* CreateApplication()
{
    return new Editor();
}