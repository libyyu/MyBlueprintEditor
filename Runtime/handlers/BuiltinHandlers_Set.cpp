// Runtime/handlers/BuiltinHandlers_Set.cpp -- Set 节点处理器
#include "BuiltinHandlers_Set.h"

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Set(std::unordered_map<std::string, NodeHandler>& handlers)
{
    // MakeSet：从输入引脚 Value0/Value1/... 收集，返回 Set（旧 id SetMake 保留兼容）
    handlers["MakeSet"] = handlers["SetMake"] = [](ExecutionContext& ctx) {
        const auto* node = ctx.GetCurrentNode();
        Variant result;
        result.type = PinDataType::Set;
        if (node)
        {
            for (const auto& pin : node->pins)
            {
                if (pin.kind == PinKind::Input && !pin.isExec)
                {
                    Variant val = ctx.GetInputValue(pin.id);
                    result.setAdd(val);
                }
            }
        }
        ctx.SetOutputValue("Set", result);
        return true;
    };

    // SetAdd：Set + Value → Set（含 Added bool 输出）
    handlers["SetAdd"] = [](ExecutionContext& ctx) {
        auto set = ctx.GetInputValue("Set");
        auto value = ctx.GetInputValue("Value");
        bool wasPresent = set.setContains(value);
        set.setAdd(value);
        bool added = !wasPresent;
        ctx.SetOutputValue("Set", set);
        ctx.SetOutputValue("Added", Variant(added));
        ctx.Log("  [SetAdd] Value=" + value.asString() + (added ? " added" : " already exists"));
        return true;
    };

    // SetRemove：Set + Value → Set（含 Removed bool 输出）
    handlers["SetRemove"] = [](ExecutionContext& ctx) {
        auto set = ctx.GetInputValue("Set");
        auto value = ctx.GetInputValue("Value");
        bool removed = set.setRemove(value);
        ctx.SetOutputValue("Set", set);
        ctx.SetOutputValue("Removed", Variant(removed));
        ctx.Log("  [SetRemove] Value=" + value.asString() + (removed ? " removed" : " not found"));
        return true;
    };

    // SetContains：Set + Value → bool
    handlers["SetContains"] = [](ExecutionContext& ctx) {
        auto set = ctx.GetInputValue("Set");
        auto value = ctx.GetInputValue("Value");
        ctx.SetOutputValue("Result", Variant(set.setContains(value)));
        return true;
    };

    // SetSize / SetLength：Set → int（两个名字都注册）
    handlers["SetSize"] = handlers["SetLength"] = [](ExecutionContext& ctx) {
        auto set = ctx.GetInputValue("Set");
        ctx.SetOutputValue("Size", Variant(static_cast<int64_t>(set.setSize())));
        return true;
    };

    // SetClear：Set → Set
    handlers["SetClear"] = [](ExecutionContext& ctx) {
        auto set = ctx.GetInputValue("Set");
        ctx.Log("  [SetClear] Cleared set (was size " + std::to_string(set.setSize()) + ")");
        set.setClear();
        ctx.SetOutputValue("Set", set);
        return true;
    };

    // SetToArray：Set → Array
    handlers["SetToArray"] = [](ExecutionContext& ctx) {
        auto set = ctx.GetInputValue("Set");
        auto arr = set.setToArray();
        ctx.SetOutputValue("Array", Variant(std::move(arr)));
        return true;
    };

    // SetFromArray：Array → Set（去重）
    handlers["SetFromArray"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        Variant result;
        result.type = PinDataType::Set;
        for (size_t i = 0; i < arr.arraySize(); ++i)
            result.setAdd(arr.arrayGet(i));
        ctx.SetOutputValue("Set", result);
        return true;
    };

    // ForEachSetLoop：遍历 Set，Loop Body + Completed 执行流，Value 输出
    handlers["ForEachSetLoop"] = [](ExecutionContext& ctx) {
        auto set = ctx.GetInputValue("Set");
        auto elements = set.setToArray();
        ctx.Log("  [ForEachSetLoop] Set size = " + std::to_string(elements.size()));
        for (size_t i = 0; i < elements.size(); ++i)
        {
            ctx.SetOutputValue("Value", elements[i]);
            ctx.SetOutputValue("Index", Variant(static_cast<int64_t>(i)));
            if (!ctx.ActivateOutputFlow("Loop Body"))
                return false;
        }
        ctx.ActivateOutputFlow("Completed");
        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor
