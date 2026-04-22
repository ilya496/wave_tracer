#include "core/Entrypoint.h"
#include "EditorLayer.h"

class Editor : public Application
{
public:
    Editor() : Application()
    {
        Window* window = Application::GetWindow();
        PushLayer(new EditorLayer(*window));
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