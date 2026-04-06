#pragma once

enum class EventType {
    WindowClose, WindowResize,
    KeyPressed, KeyReleased, KeyTyped,
    MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,
    // audio events
    AudioPlaybackStarted, AudioPlaybackStopped, AudioClipLoaded
};

class Event {
public:
    virtual EventType GetEventType() const = 0;
    virtual const char* GetName()    const = 0;
    bool Handled = false;
};

// Dispatcher lets layers handle only the types they care about
class EventDispatcher {
public:
    EventDispatcher(Event& e) : m_Event(e) {}
    template<typename T, typename F>
    bool Dispatch(const F& func) {
        if (m_Event.GetEventType() == T::GetStaticType()) {
            m_Event.Handled |= func(static_cast<T&>(m_Event));
            return true;
        }
        return false;
    }
private:
    Event& m_Event;
};