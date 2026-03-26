// Runtime/BuiltinHandlers_Misc.cpp -- Misc 节点处理器
#include "BuiltinHandlers_Misc.h"

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
}

// ============================================================================
// 入口函数
// ============================================================================

} // namespace Runtime
} // namespace NodeEditor
