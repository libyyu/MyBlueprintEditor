// Runtime/BuiltinHandlers_Debug.cpp -- Debug 调试节点处理器
#include "BuiltinHandlers_Debug.h"

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Debug(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["PrintString"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("In String").asString();
        ctx.Print(str);
        return true;
    };

    handlers["Log"] = [](ExecutionContext& ctx) {
        auto msg = ctx.GetInputValue("Message").asString();
        ctx.Print(msg);
        return true;
    };

    handlers["Assert"] = [](ExecutionContext& ctx) {
        bool condition = ctx.GetInputValue("Condition").asBool();
        auto message = ctx.GetInputValue("Message").asString();
        if (!condition)
        {
            ctx.PrintError(message.empty() ? "Assertion failed!" : message);
            return false;  // 中断执行
        }
        ctx.Log("  [Assert] Passed");
        ctx.ActivateOutputFlow("");
        return true;
    };

    // InspectValue — 透传并 Print 值
    handlers["InspectValue"] = [](ExecutionContext& ctx) {
        auto val = ctx.GetInputValue("Value");
        ctx.Print("[Inspect] " + val.asString());
        ctx.SetOutputValue("Value", val);
        return true;
    };

    // BreakOnCondition — Condition 为 true 时暂停 runner
    handlers["BreakOnCondition"] = [](ExecutionContext& ctx) {
        bool cond = ctx.GetInputValue("Condition").asBool();
        if (cond)
        {
            ctx.Log("  [BreakOnCondition] Condition met, pausing runner");
            ctx.PauseRunner();
        }
        ctx.ActivateOutputFlow("");
        return true;
    };

    handlers["FormatLog"] = [](ExecutionContext& ctx) {
        std::string fmt = ctx.GetInputValue("Format").asString();
        const auto* node = ctx.GetCurrentNode();
        if (node)
        {
            std::vector<std::string> args;
            bool skipFirst = true;
            for (const auto& pin : node->pins)
            {
                if (pin.kind == PinKind::Input && !pin.isExec)
                {
                    if (skipFirst) { skipFirst = false; continue; }
                    args.push_back(ctx.GetInputValue(pin.id).asString());
                }
            }
            for (size_t i = 0; i < args.size(); ++i)
            {
                std::string placeholder = "{" + std::to_string(i) + "}";
                size_t pos = 0;
                while ((pos = fmt.find(placeholder, pos)) != std::string::npos)
                {
                    fmt.replace(pos, placeholder.size(), args[i]);
                    pos += args[i].size();
                }
            }
        }
        ctx.Print(fmt);
        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor
