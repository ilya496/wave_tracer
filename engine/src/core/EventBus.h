#pragma once

#include "Event.h"
#include "wtpch.h"

class EventBus {
public:
    using EventCallback = std::function<void(Event&)>;

    template<typename T>
    static void Subscribe(std::function<void(T&)> callback) {
        Get().m_Subscribers[typeid(T)].push_back(
            [callback](Event& e) {
                callback(static_cast<T&>(e));
            }
        );
    }

    static void Publish(Event& event) {
        auto& subs = Get().m_Subscribers[typeid(event)];
        for (auto& cb : subs) {
            cb(event);
            if (event.Handled)
                break;
        }
    }

    static void Publish(Event&& event) {
        Publish(static_cast<Event&>(event));
    }

private:
    std::unordered_map<std::type_index, std::vector<EventCallback>> m_Subscribers;

    static EventBus& Get() {
        static EventBus instance;
        return instance;
    }
};