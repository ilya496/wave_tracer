#pragma once

#include "Event.h"
#include <functional>
#include <vector>
#include <unordered_map>

class EventBus {
public:
    using EventCallback = std::function<void(Event&)>;

    static EventBus& Get() {
        static EventBus instance;
        return instance;
    }

    // Subscribe to an event type
    template<typename T>
    static void Subscribe(EventCallback callback) {
        Get().SubscribeImpl(typeid(T), callback);
    }

    // Publish an event
    static void Publish(Event& event) {
        Get().PublishImpl(event);
    }

private:
    EventBus() = default;

    void SubscribeImpl(const std::type_info& type, EventCallback callback) {
        m_Subscribers[type.name()].push_back(callback);
    }

    void PublishImpl(Event& event) {
        auto it = m_Subscribers.find(typeid(event).name());
        if (it != m_Subscribers.end()) {
            for (auto& callback : it->second) {
                callback(event);
                if (event.Handled) break;
            }
        }
    }

    std::unordered_map<std::string, std::vector<EventCallback>> m_Subscribers;
};
