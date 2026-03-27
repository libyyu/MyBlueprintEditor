// Runtime/BuiltinHandlers_Math.cpp -- Math 数学运算节点处理器
#include "BuiltinHandlers_Math.h"
#include <cmath>
#include <cstdlib>

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Math(std::unordered_map<std::string, NodeHandler>& handlers)
{
    // --- Arithmetic ---
    // 类型提升规则：
    //   - 两个操作数都是 Integer → 结果 Integer（保留整数精度，避免 float 格式化噪音）
    //   - 任一是 Float → 结果 Float
    //   - Unknown/Any 类型（如 GetVariable Any 输出）跟随另一操作数类型；
    //     两者都是 Unknown 时回退到 Integer（整数是更常见的默认期望）
    auto resolveArithType = [](const Variant& a, const Variant& b) -> PinDataType {
        if (a.type == PinDataType::Float || b.type == PinDataType::Float)
            return PinDataType::Float;
        if (a.type == PinDataType::Integer || b.type == PinDataType::Integer)
            return PinDataType::Integer;
        // 两者都是 Unknown/Any：默认 Integer
        return PinDataType::Integer;
    };

    handlers["Add"] = [resolveArithType](ExecutionContext& ctx) {
        const Variant& va = ctx.GetInputValue("A");
        const Variant& vb = ctx.GetInputValue("B");
        if (resolveArithType(va, vb) == PinDataType::Integer)
            ctx.SetOutputValue("Result", Variant(va.asInt() + vb.asInt()));
        else
            ctx.SetOutputValue("Result", Variant(va.asFloat() + vb.asFloat()));
        return true;
    };

    handlers["Subtract"] = [resolveArithType](ExecutionContext& ctx) {
        const Variant& va = ctx.GetInputValue("A");
        const Variant& vb = ctx.GetInputValue("B");
        if (resolveArithType(va, vb) == PinDataType::Integer)
            ctx.SetOutputValue("Result", Variant(va.asInt() - vb.asInt()));
        else
            ctx.SetOutputValue("Result", Variant(va.asFloat() - vb.asFloat()));
        return true;
    };

    handlers["Multiply"] = [resolveArithType](ExecutionContext& ctx) {
        const Variant& va = ctx.GetInputValue("A");
        const Variant& vb = ctx.GetInputValue("B");
        if (resolveArithType(va, vb) == PinDataType::Integer)
            ctx.SetOutputValue("Result", Variant(va.asInt() * vb.asInt()));
        else
            ctx.SetOutputValue("Result", Variant(va.asFloat() * vb.asFloat()));
        return true;
    };

    handlers["Divide"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        if (b == 0.0) { ctx.Log("  [WARN] Division by zero!"); b = 1.0; }
        ctx.SetOutputValue("Result", Variant(a / b));
        return true;
    };

    // --- Comparison ---
    handlers["Less"] = [](ExecutionContext& ctx) {
        auto node = ctx.GetCurrentNode();
        if (node && node->pins.size() >= 3)
        {
            double a = ctx.GetInputValue(node->pins[0].id).asFloat();
            double b = ctx.GetInputValue(node->pins[1].id).asFloat();
            double result = (a < b) ? 1.0 : 0.0;
            ctx.SetOutputValue(node->pins[2].id, Variant(result));
            ctx.Log("  " + std::to_string(a) + " < " + std::to_string(b) + " = " + std::to_string(result));
        }
        return true;
    };

    handlers["Greater"] = [](ExecutionContext& ctx) {
        auto node = ctx.GetCurrentNode();
        if (node && node->pins.size() >= 3)
        {
            double a = ctx.GetInputValue(node->pins[0].id).asFloat();
            double b = ctx.GetInputValue(node->pins[1].id).asFloat();
            double result = (a > b) ? 1.0 : 0.0;
            ctx.SetOutputValue(node->pins[2].id, Variant(result));
        }
        return true;
    };

    handlers["Equal"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", Variant(a == b));
        return true;
    };

    // --- Logic ---
    handlers["And"] = [](ExecutionContext& ctx) {
        bool a = ctx.GetInputValue("A").asBool();
        bool b = ctx.GetInputValue("B").asBool();
        ctx.SetOutputValue("Result", Variant(a && b));
        return true;
    };

    handlers["Or"] = [](ExecutionContext& ctx) {
        bool a = ctx.GetInputValue("A").asBool();
        bool b = ctx.GetInputValue("B").asBool();
        ctx.SetOutputValue("Result", Variant(a || b));
        return true;
    };

    handlers["Not"] = [](ExecutionContext& ctx) {
        bool v = ctx.GetInputValue("Value").asBool();
        ctx.SetOutputValue("Result", Variant(!v));
        return true;
    };

    // --- Type Conversion ---
    handlers["IntToFloat"] = [](ExecutionContext& ctx) {
        int64_t v = ctx.GetInputValue("Value").asInt();
        ctx.SetOutputValue("Result", Variant(static_cast<double>(v)));
        return true;
    };

    handlers["FloatToInt"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(static_cast<int64_t>(v)));
        return true;
    };

    handlers["FloatToBool"] = [](ExecutionContext& ctx) {
        bool v = ctx.GetInputValue("Value").asBool();
        ctx.SetOutputValue("Result", Variant(v));
        return true;
    };

    handlers["IntToString"] = [](ExecutionContext& ctx) {
        int64_t v = ctx.GetInputValue("Value").asInt();
        ctx.SetOutputValue("Result", Variant(std::to_string(v)));
        return true;
    };

    handlers["FloatToString"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(std::to_string(v)));
        return true;
    };

    // --- Math Functions ---
    handlers["Abs"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(v < 0 ? -v : v));
        return true;
    };

    handlers["Clamp"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        double lo = ctx.GetInputValue("Min").asFloat();
        double hi = ctx.GetInputValue("Max").asFloat();
        if (v < lo) v = lo;
        if (v > hi) v = hi;
        ctx.SetOutputValue("Result", Variant(v));
        return true;
    };

    handlers["Min"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", Variant(a < b ? a : b));
        return true;
    };

    handlers["Max"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", Variant(a > b ? a : b));
        return true;
    };

    // --- Random ---
    handlers["Random"] = [](ExecutionContext& ctx) {
        double v = static_cast<double>(rand()) / RAND_MAX;
        ctx.SetOutputValue("Value", Variant(v));
        return true;
    };

    handlers["RandomInRange"] = [](ExecutionContext& ctx) {
        double lo = ctx.GetInputValue("Min").asFloat();
        double hi = ctx.GetInputValue("Max").asFloat();
        double v = lo + (static_cast<double>(rand()) / RAND_MAX) * (hi - lo);
        ctx.SetOutputValue("Value", Variant(v));
        return true;
    };

    // --- Misc Math ---
    handlers["Weird"] = [](ExecutionContext& ctx) {
        auto node = ctx.GetCurrentNode();
        if (node && node->pins.size() >= 3)
        {
            double input = ctx.GetInputValue(node->pins[0].id).asFloat();
            ctx.SetOutputValue(node->pins[1].id, Variant(input));
            ctx.SetOutputValue(node->pins[2].id, Variant(input));
        }
        return true;
    };

    // --- More Arithmetic ---
    handlers["Modulo"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        if (b == 0.0) { ctx.Log("  [WARN] Modulo by zero!"); b = 1.0; }
        ctx.SetOutputValue("Result", Variant(std::fmod(a, b)));
        return true;
    };

    handlers["Power"] = [](ExecutionContext& ctx) {
        double base = ctx.GetInputValue("Base").asFloat();
        double exp = ctx.GetInputValue("Exponent").asFloat();
        ctx.SetOutputValue("Result", Variant(std::pow(base, exp)));
        return true;
    };

    handlers["Negate"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(-v));
        return true;
    };

    // --- More Comparison ---
    handlers["NotEqual"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", Variant(a != b));
        return true;
    };

    handlers["LessEqual"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", Variant(a <= b));
        return true;
    };

    handlers["GreaterEqual"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", Variant(a >= b));
        return true;
    };

    // --- More Logic ---
    handlers["Nand"] = [](ExecutionContext& ctx) {
        bool a = ctx.GetInputValue("A").asBool();
        bool b = ctx.GetInputValue("B").asBool();
        ctx.SetOutputValue("Result", Variant(!(a && b)));
        return true;
    };

    handlers["Nor"] = [](ExecutionContext& ctx) {
        bool a = ctx.GetInputValue("A").asBool();
        bool b = ctx.GetInputValue("B").asBool();
        ctx.SetOutputValue("Result", Variant(!(a || b)));
        return true;
    };

    handlers["Xor"] = [](ExecutionContext& ctx) {
        bool a = ctx.GetInputValue("A").asBool();
        bool b = ctx.GetInputValue("B").asBool();
        ctx.SetOutputValue("Result", Variant(a != b));
        return true;
    };

    // --- More Conversion ---
    handlers["StringToInt"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("Value").asString();
        bool valid = false;
        int64_t result = 0;
        if (!str.empty())
        {
            char* end = nullptr;
            result = std::strtoll(str.c_str(), &end, 10);
            valid = (end != str.c_str() && *end == '\0');
        }
        ctx.SetOutputValue("Result", Variant(result));
        ctx.SetOutputValue("Valid", Variant(valid));
        return true;
    };

    handlers["StringToFloat"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("Value").asString();
        bool valid = false;
        double result = 0.0;
        if (!str.empty())
        {
            char* end = nullptr;
            result = std::strtod(str.c_str(), &end);
            valid = (end != str.c_str() && *end == '\0');
        }
        ctx.SetOutputValue("Result", Variant(result));
        ctx.SetOutputValue("Valid", Variant(valid));
        return true;
    };

    handlers["BoolToString"] = [](ExecutionContext& ctx) {
        bool v = ctx.GetInputValue("Value").asBool();
        ctx.SetOutputValue("Result", Variant(std::string(v ? "True" : "False")));
        return true;
    };

    // --- More Functions ---
    handlers["Sqrt"] = [](ExecutionContext& ctx) {
        auto* node = ctx.GetCurrentNode();
        double v = 0.0;
        if (node && !node->pins.empty())
            v = ctx.GetInputValue(node->pins[0].id).asFloat();
        ctx.SetOutputValue("Result", Variant(std::sqrt(std::abs(v))));
        return true;
    };

    handlers["Sin"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value (Rad)").asFloat();
        ctx.SetOutputValue("Result", Variant(std::sin(v)));
        return true;
    };

    handlers["Cos"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value (Rad)").asFloat();
        ctx.SetOutputValue("Result", Variant(std::cos(v)));
        return true;
    };

    handlers["Tan"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value (Rad)").asFloat();
        ctx.SetOutputValue("Result", Variant(std::tan(v)));
        return true;
    };

    handlers["Atan2"] = [](ExecutionContext& ctx) {
        double y = ctx.GetInputValue("Y").asFloat();
        double x = ctx.GetInputValue("X").asFloat();
        ctx.SetOutputValue("Result (Rad)", Variant(std::atan2(y, x)));
        return true;
    };

    handlers["Lerp"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        double alpha = ctx.GetInputValue("Alpha").asFloat();
        ctx.SetOutputValue("Result", Variant(a + (b - a) * alpha));
        return true;
    };

    handlers["MapRange"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        double inMin = ctx.GetInputValue("InMin").asFloat();
        double inMax = ctx.GetInputValue("InMax").asFloat();
        double outMin = ctx.GetInputValue("OutMin").asFloat();
        double outMax = ctx.GetInputValue("OutMax").asFloat();
        double range = inMax - inMin;
        if (range == 0.0) range = 1.0;
        double t = (v - inMin) / range;
        ctx.SetOutputValue("Result", Variant(outMin + (outMax - outMin) * t));
        return true;
    };

    handlers["Ceil"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(std::ceil(v)));
        return true;
    };

    handlers["Floor"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(std::floor(v)));
        return true;
    };

    handlers["Round"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(std::round(v)));
        return true;
    };

    handlers["Sign"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        double result = (v > 0.0) ? 1.0 : (v < 0.0 ? -1.0 : 0.0);
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    // --- Constants ---
    handlers["PI"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("Value", Variant(3.14159265358979323846));
        return true;
    };

    handlers["E"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("Value", Variant(2.71828182845904523536));
        return true;
    };

    // --- Advanced Functions ---
    handlers["InverseLerp"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        double v = ctx.GetInputValue("Value").asFloat();
        double range = b - a;
        if (range == 0.0) range = 1.0;
        ctx.SetOutputValue("Result", Variant((v - a) / range));
        return true;
    };

    handlers["Remap01"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        double inMin = ctx.GetInputValue("InMin").asFloat();
        double inMax = ctx.GetInputValue("InMax").asFloat();
        double range = inMax - inMin;
        if (range == 0.0) range = 1.0;
        double result = (v - inMin) / range;
        // Clamp to [0, 1]
        if (result < 0.0) result = 0.0;
        if (result > 1.0) result = 1.0;
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    handlers["DegreesToRadians"] = [](ExecutionContext& ctx) {
        double deg = ctx.GetInputValue("Degrees").asFloat();
        ctx.SetOutputValue("Radians", Variant(deg * 3.14159265358979323846 / 180.0));
        return true;
    };

    handlers["RadiansToDegrees"] = [](ExecutionContext& ctx) {
        double rad = ctx.GetInputValue("Radians").asFloat();
        ctx.SetOutputValue("Degrees", Variant(rad * 180.0 / 3.14159265358979323846));
        return true;
    };

    handlers["Wrap"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        double lo = ctx.GetInputValue("Min").asFloat();
        double hi = ctx.GetInputValue("Max").asFloat();
        double range = hi - lo;
        if (range <= 0.0) { ctx.SetOutputValue("Result", Variant(lo)); return true; }
        double result = lo + std::fmod(std::fmod(v - lo, range) + range, range);
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    handlers["Snap"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        double grid = ctx.GetInputValue("GridSize").asFloat();
        if (grid <= 0.0) grid = 1.0;
        ctx.SetOutputValue("Result", Variant(std::round(v / grid) * grid));
        return true;
    };

    handlers["Log2"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(v > 0.0 ? std::log2(v) : 0.0));
        return true;
    };

    handlers["Log10"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(v > 0.0 ? std::log10(v) : 0.0));
        return true;
    };

    handlers["Exp"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(std::exp(v)));
        return true;
    };

    // ── Conversion 补充 ───────────────────────────────────────────────────
    handlers["BoolToInt"] = [](ExecutionContext& ctx) {
        bool v = ctx.GetInputValue("Value").asBool();
        ctx.SetOutputValue("Result", Variant(static_cast<int64_t>(v ? 1 : 0)));
        return true;
    };

    handlers["IntToBool"] = [](ExecutionContext& ctx) {
        int64_t v = ctx.GetInputValue("Value").asInt();
        ctx.SetOutputValue("Result", Variant(v != 0));
        return true;
    };

    handlers["BoolToFloat"] = [](ExecutionContext& ctx) {
        bool v = ctx.GetInputValue("Value").asBool();
        ctx.SetOutputValue("Result", Variant(v ? 1.0 : 0.0));
        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor
