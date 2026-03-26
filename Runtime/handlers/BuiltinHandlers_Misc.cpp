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
}

// ============================================================================
// 入口函数
// ============================================================================

} // namespace Runtime
} // namespace NodeEditor
