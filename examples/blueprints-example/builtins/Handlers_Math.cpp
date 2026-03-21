// Handlers_Math.cpp -- Math 节点处理器注册（算术、比较、逻辑、类型转换、数学函数、随机）
#include "../BlueprintEditor.h"
#include <cstdlib>

void BlueprintEditor::RegisterHandlers_Math()
{
    // --- Arithmetic ---
    m_HandlerRegistry["Add"] = [](RTContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", RTVariant(a + b));
        return true;
    };

    m_HandlerRegistry["Subtract"] = [](RTContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", RTVariant(a - b));
        return true;
    };

    m_HandlerRegistry["Multiply"] = [](RTContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", RTVariant(a * b));
        return true;
    };

    m_HandlerRegistry["Divide"] = [](RTContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        if (b == 0.0) { ctx.Log("  [WARN] Division by zero!"); b = 1.0; }
        ctx.SetOutputValue("Result", RTVariant(a / b));
        return true;
    };

    // --- Comparison ---
    m_HandlerRegistry["Less"] = [](RTContext& ctx) {
        auto node = ctx.GetCurrentNode();
        if (node && node->pins.size() >= 3)
        {
            double a = ctx.GetInputValue(node->pins[0].id).asFloat();
            double b = ctx.GetInputValue(node->pins[1].id).asFloat();
            double result = (a < b) ? 1.0 : 0.0;
            ctx.SetOutputValue(node->pins[2].id, RTVariant(result));
            ctx.Log("  " + std::to_string(a) + " < " + std::to_string(b) + " = " + std::to_string(result));
        }
        return true;
    };

    m_HandlerRegistry["Greater"] = [](RTContext& ctx) {
        auto node = ctx.GetCurrentNode();
        if (node && node->pins.size() >= 3)
        {
            double a = ctx.GetInputValue(node->pins[0].id).asFloat();
            double b = ctx.GetInputValue(node->pins[1].id).asFloat();
            double result = (a > b) ? 1.0 : 0.0;
            ctx.SetOutputValue(node->pins[2].id, RTVariant(result));
        }
        return true;
    };

    m_HandlerRegistry["Equal"] = [](RTContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", RTVariant(a == b));
        return true;
    };

    // --- Logic ---
    m_HandlerRegistry["And"] = [](RTContext& ctx) {
        bool a = ctx.GetInputValue("A").asBool();
        bool b = ctx.GetInputValue("B").asBool();
        ctx.SetOutputValue("Result", RTVariant(a && b));
        return true;
    };

    m_HandlerRegistry["Or"] = [](RTContext& ctx) {
        bool a = ctx.GetInputValue("A").asBool();
        bool b = ctx.GetInputValue("B").asBool();
        ctx.SetOutputValue("Result", RTVariant(a || b));
        return true;
    };

    m_HandlerRegistry["Not"] = [](RTContext& ctx) {
        bool v = ctx.GetInputValue("Value").asBool();
        ctx.SetOutputValue("Result", RTVariant(!v));
        return true;
    };

    // --- Type Conversion ---
    m_HandlerRegistry["IntToFloat"] = [](RTContext& ctx) {
        int64_t v = ctx.GetInputValue("Value").asInt();
        ctx.SetOutputValue("Result", RTVariant(static_cast<double>(v)));
        return true;
    };

    m_HandlerRegistry["FloatToInt"] = [](RTContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", RTVariant(static_cast<int64_t>(v)));
        return true;
    };

    m_HandlerRegistry["FloatToBool"] = [](RTContext& ctx) {
        double v = ctx.GetInputValue("Value").asBool();
        ctx.SetOutputValue("Result", RTVariant(v));
        return true;
    };

    m_HandlerRegistry["IntToString"] = [](RTContext& ctx) {
        int64_t v = ctx.GetInputValue("Value").asInt();
        ctx.SetOutputValue("Result", RTVariant(std::to_string(v)));
        return true;
    };

    m_HandlerRegistry["FloatToString"] = [](RTContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", RTVariant(std::to_string(v)));
        return true;
    };

    // --- Math Functions ---
    m_HandlerRegistry["Abs"] = [](RTContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", RTVariant(v < 0 ? -v : v));
        return true;
    };

    m_HandlerRegistry["Clamp"] = [](RTContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        double lo = ctx.GetInputValue("Min").asFloat();
        double hi = ctx.GetInputValue("Max").asFloat();
        if (v < lo) v = lo;
        if (v > hi) v = hi;
        ctx.SetOutputValue("Result", RTVariant(v));
        return true;
    };

    m_HandlerRegistry["Min"] = [](RTContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", RTVariant(a < b ? a : b));
        return true;
    };

    m_HandlerRegistry["Max"] = [](RTContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", RTVariant(a > b ? a : b));
        return true;
    };

    // --- Random ---
    m_HandlerRegistry["Random"] = [](RTContext& ctx) {
        double v = static_cast<double>(rand()) / RAND_MAX;
        ctx.SetOutputValue("Value", RTVariant(v));
        return true;
    };

    m_HandlerRegistry["RandomInRange"] = [](RTContext& ctx) {
        double lo = ctx.GetInputValue("Min").asFloat();
        double hi = ctx.GetInputValue("B").asFloat();
        double v = lo + (static_cast<double>(rand()) / RAND_MAX) * (hi - lo);
        ctx.SetOutputValue("Value", RTVariant(v));
        return true;
    };

    // --- Misc Math ---
    m_HandlerRegistry["Weird"] = [](RTContext& ctx) {
        auto node = ctx.GetCurrentNode();
        if (node && node->pins.size() >= 3)
        {
            double input = ctx.GetInputValue(node->pins[0].id).asFloat();
            ctx.SetOutputValue(node->pins[1].id, RTVariant(input));
            ctx.SetOutputValue(node->pins[2].id, RTVariant(input));
        }
        return true;
    };
}
