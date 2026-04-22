#pragma once

enum class EventType {
    WindowClose, WindowResize, WindowDpiChanged,
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

// Window Events
class WindowCloseEvent : public Event {
public:
    WindowCloseEvent() = default;
    static EventType GetStaticType() { return EventType::WindowClose; }
    EventType GetEventType() const override { return GetStaticType(); }
    const char* GetName() const override { return "WindowClose"; }
};

class WindowResizeEvent : public Event {
public:
    WindowResizeEvent(unsigned int width, unsigned int height)
        : m_Width(width), m_Height(height) {
    }
    static EventType GetStaticType() { return EventType::WindowResize; }
    EventType GetEventType() const override { return GetStaticType(); }
    const char* GetName() const override { return "WindowResize"; }
    unsigned int GetWidth() const { return m_Width; }
    unsigned int GetHeight() const { return m_Height; }
private:
    unsigned int m_Width, m_Height;
};

class WindowDpiChangedEvent : public Event {
public:
    WindowDpiChangedEvent(float xscale, float yscale)
        : m_XScale(xscale), m_YScale(yscale) {
    }
    static EventType GetStaticType() { return EventType::WindowDpiChanged; }
    EventType GetEventType() const override { return GetStaticType(); }
    const char* GetName() const override { return "WindowDpiChanged"; }
    float GetXScale() const { return m_XScale; }
    float GetYScale() const { return m_YScale; }
private:
    float m_XScale, m_YScale;
};

// Key Events
class KeyEvent : public Event {
public:
    int GetKeyCode() const { return m_KeyCode; }
protected:
    KeyEvent(int keycode) : m_KeyCode(keycode) {}
    int m_KeyCode;
};

class KeyPressedEvent : public KeyEvent {
public:
    KeyPressedEvent(int keycode, int repeatCount)
        : KeyEvent(keycode), m_RepeatCount(repeatCount) {
    }
    static EventType GetStaticType() { return EventType::KeyPressed; }
    EventType GetEventType() const override { return GetStaticType(); }
    const char* GetName() const override { return "KeyPressed"; }
    int GetRepeatCount() const { return m_RepeatCount; }
private:
    int m_RepeatCount;
};

class KeyReleasedEvent : public KeyEvent {
public:
    KeyReleasedEvent(int keycode) : KeyEvent(keycode) {}
    static EventType GetStaticType() { return EventType::KeyReleased; }
    EventType GetEventType() const override { return GetStaticType(); }
    const char* GetName() const override { return "KeyReleased"; }
};

class KeyTypedEvent : public Event {
public:
    KeyTypedEvent(unsigned int keycode) : m_KeyCode(keycode) {}
    static EventType GetStaticType() { return EventType::KeyTyped; }
    EventType GetEventType() const override { return GetStaticType(); }
    const char* GetName() const override { return "KeyTyped"; }
    unsigned int GetKeyCode() const { return m_KeyCode; }
private:
    unsigned int m_KeyCode;
};

// Mouse Events
class MouseMovedEvent : public Event {
public:
    MouseMovedEvent(float x, float y) : m_MouseX(x), m_MouseY(y) {}
    static EventType GetStaticType() { return EventType::MouseMoved; }
    EventType GetEventType() const override { return GetStaticType(); }
    const char* GetName() const override { return "MouseMoved"; }
    float GetX() const { return m_MouseX; }
    float GetY() const { return m_MouseY; }
private:
    float m_MouseX, m_MouseY;
};

class MouseScrolledEvent : public Event {
public:
    MouseScrolledEvent(float xOffset, float yOffset)
        : m_XOffset(xOffset), m_YOffset(yOffset) {
    }
    static EventType GetStaticType() { return EventType::MouseScrolled; }
    EventType GetEventType() const override { return GetStaticType(); }
    const char* GetName() const override { return "MouseScrolled"; }
    float GetXOffset() const { return m_XOffset; }
    float GetYOffset() const { return m_YOffset; }
private:
    float m_XOffset, m_YOffset;
};

class MouseButtonEvent : public Event {
public:
    int GetMouseButton() const { return m_Button; }
protected:
    MouseButtonEvent(int button) : m_Button(button) {}
    int m_Button;
};

class MouseButtonPressedEvent : public MouseButtonEvent {
public:
    MouseButtonPressedEvent(int button) : MouseButtonEvent(button) {}
    static EventType GetStaticType() { return EventType::MouseButtonPressed; }
    EventType GetEventType() const override { return GetStaticType(); }
    const char* GetName() const override { return "MouseButtonPressed"; }
};

class MouseButtonReleasedEvent : public MouseButtonEvent {
public:
    MouseButtonReleasedEvent(int button) : MouseButtonEvent(button) {}
    static EventType GetStaticType() { return EventType::MouseButtonReleased; }
    EventType GetEventType() const override { return GetStaticType(); }
    const char* GetName() const override { return "MouseButtonReleased"; }
};

// Audio Events
class AudioPlaybackStartedEvent : public Event {
public:
    AudioPlaybackStartedEvent() = default;
    static EventType GetStaticType() { return EventType::AudioPlaybackStarted; }
    EventType GetEventType() const override { return GetStaticType(); }
    const char* GetName() const override { return "AudioPlaybackStarted"; }
};

class AudioPlaybackStoppedEvent : public Event {
public:
    AudioPlaybackStoppedEvent() = default;
    static EventType GetStaticType() { return EventType::AudioPlaybackStopped; }
    EventType GetEventType() const override { return GetStaticType(); }
    const char* GetName() const override { return "AudioPlaybackStopped"; }
};

class AudioClipLoadedEvent : public Event {
public:
    AudioClipLoadedEvent() = default;
    static EventType GetStaticType() { return EventType::AudioClipLoaded; }
    EventType GetEventType() const override { return GetStaticType(); }
    const char* GetName() const override { return "AudioClipLoaded"; }
};