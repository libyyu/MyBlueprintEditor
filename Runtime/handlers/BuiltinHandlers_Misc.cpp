// Runtime/BuiltinHandlers_Misc.cpp -- Misc 节点处理器
#include "BuiltinHandlers_Misc.h"
#include "../EventBus.h"

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Misc(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["GetVariable"] = [](ExecutionContext& ctx) {
        auto name = ctx.GetInputValue("Name").asString();
        ctx.SetOutputValue("Value", ctx.GetVariable(name));
        return true;
    };

    handlers["SetVariable"] = [](ExecutionContext& ctx) {
        auto name = ctx.GetInputValue("Name").asString();
        auto val = ctx.GetInputValue("Value");
        ctx.SetVariable(name, val);
        ctx.Log("  Set '" + name + "' = '" + val.asString() + "'");
        return true;
    };

    handlers["IsValid"] = [](ExecutionContext& ctx) {
        auto val = ctx.GetInputValue("Value");
        bool isValid = false;
        switch (val.type)
        {
        case PinDataType::String:  isValid = !val.stringValue.empty(); break;
        case PinDataType::Object:  isValid = !val.stringValue.empty(); break;
        case PinDataType::Array:   isValid = !val.arrayValue.empty(); break;
        case PinDataType::Integer: isValid = std::get<int64_t>(val.numericValue) != 0; break;
        case PinDataType::Float:   isValid = std::get<double>(val.numericValue) != 0.0; break;
        case PinDataType::Boolean: isValid = std::get<bool>(val.numericValue); break;
        default: break;
        }
        ctx.SetOutputValue("Is Valid", Variant(isValid));
        return true;
    };

    handlers["MakeLiteralBool"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("Result", Variant(ctx.GetInputValue("Value").asBool()));
        return true;
    };

    handlers["MakeLiteralInt"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("Result", Variant(ctx.GetInputValue("Value").asInt()));
        return true;
    };

    handlers["MakeLiteralFloat"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("Result", Variant(ctx.GetInputValue("Value").asFloat()));
        return true;
    };

    handlers["MakeLiteralString"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("Result", Variant(ctx.GetInputValue("Value").asString()));
        return true;
    };

    // Message 节点：无输入引脚，输出引脚 "Message" 的值来自 pin 的 defaultValue。
    // 由于 runtime 只把 Input 引脚的 defaultValue 注入 pinValues，
    // Output 引脚需要 handler 手动读取 defaultValue 并写入 pinValues。
    handlers["Message"] = [](ExecutionContext& ctx) {
        const auto* node = ctx.GetCurrentNode();
        if (!node) return true;
        for (const auto& pin : node->pins)
        {
            if (pin.kind == PinKind::Output && !pin.isExec)
            {
                if (pin.defaultValue.type != PinDataType::Unknown)
                    ctx.SetOutputValue(pin.id, pin.defaultValue);
                else
                    ctx.SetOutputValue(pin.id, Variant(std::string("")));
            }
        }
        return true;
    };

    // ── EventBus ──────────────────────────────────────────────────────────
    // 简化实现：Emit 将 payload 写入 "__eventbus_<Event>" 变量，
    // Subscribe 在收到 Enable 激活后持续监听（每帧 Poll 模式由 runner 调用）。
    // 当前版本：Emit 立即触发所有已激活的 Subscribe 节点（同步广播）。

    handlers["EventBusEmit"] = [](ExecutionContext& ctx) {
        std::string evtName = ctx.GetInputValue("Event").asString();
        if (evtName.empty()) { ctx.ActivateOutputFlow(""); return true; }
        Variant payload = ctx.GetInputValue("Payload");
        // 存储最新 payload 到全局变量（供 Subscribe 读取）
        ctx.SetVariable("__eventbus_payload_" + evtName, payload);
        // 写一个计数器作为"脉冲"标志，Subscribe 靠检测它变化来触发
        Variant counter = ctx.GetVariable("__eventbus_counter_" + evtName);
        int64_t cnt = counter.asInt() + 1;
        ctx.SetVariable("__eventbus_counter_" + evtName, Variant(cnt));
        ctx.Log("[EventBus] Emit: " + evtName + " (counter=" + std::to_string(cnt) + ")");
        ctx.ActivateOutputFlow("");
        return true;
    };

    handlers["EventBusSubscribe"] = [](ExecutionContext& ctx) {
        // 通过以节点 id 为前缀的变量记录上次看到的 counter，
        // 与全局 counter 对比判断是否有新事件。
        std::string evtName = ctx.GetInputValue("Event").asString();
        if (evtName.empty()) return true;
        const auto* node = ctx.GetCurrentNode();
        std::string nodeKey = "__sub_" + (node ? std::to_string(node->id) : "0") + "_" + evtName;
        int64_t globalCnt = ctx.GetVariable("__eventbus_counter_" + evtName).asInt();
        int64_t lastCnt   = ctx.GetVariable(nodeKey).asInt();
        if (globalCnt != lastCnt)
        {
            ctx.SetVariable(nodeKey, Variant(globalCnt));
            Variant payload = ctx.GetVariable("__eventbus_payload_" + evtName);
            ctx.SetOutputValue("Payload", payload);
            ctx.Log("[EventBus] Subscribe fired: " + evtName);
            ctx.ActivateOutputFlow("On Event");
        }
        return true;
    };

    handlers["EventBusClear"] = [](ExecutionContext& ctx) {
        std::string evtName = ctx.GetInputValue("Event").asString();
        if (!evtName.empty())
        {
            ctx.SetVariable("__eventbus_counter_" + evtName, Variant(int64_t(0)));
            ctx.SetVariable("__eventbus_payload_" + evtName, Variant());
        }
        ctx.ActivateOutputFlow("");
        return true;
    };

    // ── 新版 EventBus 节点（使用全局 EventBus 单例）─────────────────────────

    handlers["Event.Fire"] = [](ExecutionContext& ctx) {
        std::string evtName = ctx.GetInputValue("EventName").asString();
        Variant payload     = ctx.GetInputValue("Payload");
        if (!evtName.empty())
        {
            ctx.Log("[EventBus] Fire: " + evtName);
            EventBus::Get().Fire(evtName, payload);
        }
        ctx.ActivateOutputFlow("");
        return true;
    };

    handlers["Event.Subscribe"] = [](ExecutionContext& ctx) {
        std::string evtName = ctx.GetInputValue("EventName").asString();
        if (!evtName.empty())
        {
            int subId = EventBus::Get().Subscribe(evtName, [](const std::string& /*name*/, const Variant& /*pl*/) {
                // 订阅回调在节点执行中无法直接激活下游，记录为变量供 OnEvent 节点轮询
            });
            ctx.SetOutputValue("SubscriptionId", Variant(static_cast<int64_t>(subId)));
            ctx.Log("[EventBus] Subscribe: " + evtName + " id=" + std::to_string(subId));
        }
        ctx.ActivateOutputFlow("");
        return true;
    };

    handlers["Event.Unsubscribe"] = [](ExecutionContext& ctx) {
        int64_t subId = ctx.GetInputValue("SubscriptionId").asInt();
        EventBus::Get().Unsubscribe(static_cast<int>(subId));
        ctx.Log("[EventBus] Unsubscribe id=" + std::to_string(subId));
        ctx.ActivateOutputFlow("");
        return true;
    };

    handlers["Event.OnEvent"] = [](ExecutionContext& ctx) {
        // Event.OnEvent 是纯触发型：在 Execute() 前注册，事件触发时激活下游。
        // 运行时简化实现：通过变量 "__onevent_pending_<evtname>" 检测事件是否触发。
        const auto* node = ctx.GetCurrentNode();
        std::string nodeKey = node ? ("__onevent_node_" + std::to_string(node->id)) : "__onevent_node_0";
        std::string pendingEvt = ctx.GetVariable(nodeKey + "_evt").asString();
        if (!pendingEvt.empty())
        {
            Variant payload = ctx.GetVariable(nodeKey + "_payload");
            ctx.SetOutputValue("EventName", Variant(pendingEvt));
            ctx.SetOutputValue("Payload", payload);
            // 清除 pending 标志
            ctx.SetVariable(nodeKey + "_evt", Variant(std::string("")));
            ctx.ActivateOutputFlow("");
        }
        return true;
    };

    // ── Function 系统节点 ──────────────────────────────────────────────────
    // Function.Entry：函数子图的起点，直接激活下游 exec flow
    handlers["Function.Entry"] = [](ExecutionContext& ctx) {
        ctx.ActivateOutputFlow(std::string(""));
        return true;
    };

    // Function.Return：函数子图的终点，无任何操作（子 runner Execute 自然结束）
    handlers["Function.Return"] = [](ExecutionContext& ctx) {
        (void)ctx;
        return true;
    };

    // Function.Call 在 BlueprintRunner::executeNodeInternal 里内置处理，此处仅占位防止
    // "handler not found" 警告（当 runner 以非 blueprint 方式执行时不会命中此分支）
    // handlers["Function.Call"] — intentionally left to built-in path
}

// ============================================================================
// 入口函数
// ============================================================================

} // namespace Runtime
} // namespace NodeEditor
