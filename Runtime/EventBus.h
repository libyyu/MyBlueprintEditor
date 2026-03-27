// Runtime/EventBus.h - 蓝图事件总线（全局单例）
//
// 订阅/发布模式，允许跨蓝图事件传播。
// 事件发布是同步的：Fire() 立即调用所有订阅者。

#pragma once
#include "BlueprintExport.h"
#include "Types.h"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

namespace NodeEditor {
namespace Runtime {

using EventHandler = std::function<void(const std::string& eventName, const Variant& payload)>;

class BLUEPRINT_API EventBus
{
public:
    static EventBus& Get();

    // 订阅事件，返回订阅 ID（用于 Unsubscribe）
    int Subscribe(const std::string& eventName, EventHandler handler);

    // 取消订阅
    void Unsubscribe(int subscriptionId);

    // 发布事件（同步：立即调用所有订阅者）
    void Fire(const std::string& eventName, const Variant& payload = Variant());

    // 清除某事件的所有订阅
    void ClearEvent(const std::string& eventName);

    // 清除所有订阅
    void ClearAll();

    // 获取所有已注册事件名（用于编辑器显示）
    std::vector<std::string> GetRegisteredEvents() const;

private:
    EventBus() = default;

    struct Subscription
    {
        int id;
        EventHandler handler;
    };

    std::unordered_map<std::string, std::vector<Subscription>> m_subscriptions;
    int m_nextId = 1;
};

} // namespace Runtime
} // namespace NodeEditor

#ifdef _MSC_VER
#pragma warning(pop)
#endif
