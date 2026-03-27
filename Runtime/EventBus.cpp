// Runtime/EventBus.cpp - 事件总线实现

#include "EventBus.h"
#include <algorithm>

namespace NodeEditor {
namespace Runtime {

EventBus& EventBus::Get()
{
    static EventBus instance;
    return instance;
}

int EventBus::Subscribe(const std::string& eventName, EventHandler handler)
{
    int id = m_nextId++;
    m_subscriptions[eventName].push_back({id, std::move(handler)});
    return id;
}

void EventBus::Unsubscribe(int subscriptionId)
{
    for (auto& kv : m_subscriptions)
    {
        auto& subs = kv.second;
        subs.erase(
            std::remove_if(subs.begin(), subs.end(),
                [subscriptionId](const Subscription& s) { return s.id == subscriptionId; }),
            subs.end());
    }
}

void EventBus::Fire(const std::string& eventName, const Variant& payload)
{
    auto it = m_subscriptions.find(eventName);
    if (it == m_subscriptions.end()) return;

    // 拷贝副本，防止回调中修改 m_subscriptions 导致迭代器失效
    auto subs = it->second;
    for (auto& sub : subs)
    {
        if (sub.handler)
            sub.handler(eventName, payload);
    }
}

void EventBus::ClearEvent(const std::string& eventName)
{
    m_subscriptions.erase(eventName);
}

void EventBus::ClearAll()
{
    m_subscriptions.clear();
}

std::vector<std::string> EventBus::GetRegisteredEvents() const
{
    std::vector<std::string> result;
    result.reserve(m_subscriptions.size());
    for (const auto& kv : m_subscriptions)
        result.push_back(kv.first);
    return result;
}

} // namespace Runtime
} // namespace NodeEditor
