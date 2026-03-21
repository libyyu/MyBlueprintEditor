// NodeDefs_Math.cpp -- Math 节点定义注册（按子分类拆分）
//
// 子分类结构:
//   Math/Arithmetic   - 四则运算 (+, -, *, /)
//   Math/Comparison   - 比较运算 (<, >, ==)
//   Math/Logic        - 逻辑运算 (AND, OR, NOT)
//   Math/Conversion   - 类型转换 (IntToFloat, FloatToInt, FloatToString)
//   Math/Functions    - 数学函数 (Abs, Clamp, Min, Max)
//   Math/Random       - 随机数 (Random, RandomInRange)
//   Math              - 其他 (o.O)
//
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterNodeDefs_Math()
{
    auto reg = [this](const char* id, const char* name, const char* category,
                      std::vector<RTPinDef> inputs, std::vector<RTPinDef> outputs,
                      const char* color = "", const char* edType = "")
    {
        RTNodeDef d;
        d.id = id;
        d.name = name;
        d.category = category;
        d.inputPins = std::move(inputs);
        d.outputPins = std::move(outputs);
        if (color[0]) d.color = color;
        if (edType[0]) d.customProperties["editorType"] = edType;
        m_NodeRegistry.registerNode(d);
    };

    // --- Arithmetic (Math/Arithmetic) ---
    reg("Add", "+", "Math/Arithmetic",
        { MakePin("A", RTPinDataType::Float), MakePin("B", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("AddInteger", "+", "Math/Arithmetic",
        { MakePin("A", RTPinDataType::Integer), MakePin("B", RTPinDataType::Integer) },
        { MakePin("Result", RTPinDataType::Integer) },
        "80C3F8", "Simple");

    reg("Subtract", "-", "Math/Arithmetic",
        { MakePin("A", RTPinDataType::Float), MakePin("B", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Multiply", "*", "Math/Arithmetic",
        { MakePin("A", RTPinDataType::Float), MakePin("B", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Divide", "/", "Math/Arithmetic",
        { MakePin("A", RTPinDataType::Float), MakePin("B", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    // --- Comparison (Math/Comparison) ---
    reg("Less", "<", "Math/Comparison",
        { MakePin("", RTPinDataType::Float), MakePin("", RTPinDataType::Float) },
        { MakePin("", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Greater", ">", "Math/Comparison",
        { MakePin("", RTPinDataType::Float), MakePin("", RTPinDataType::Float) },
        { MakePin("", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Equal", "==", "Math/Comparison",
        { MakePin("A", RTPinDataType::Float), MakePin("B", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Boolean) },
        "80C3F8", "Simple");

    // --- Logic (Math/Logic) ---
    reg("And", "AND", "Math/Logic",
        { MakePin("A", RTPinDataType::Boolean), MakePin("B", RTPinDataType::Boolean) },
        { MakePin("Result", RTPinDataType::Boolean) },
        "80C3F8", "Simple");

    reg("Or", "OR", "Math/Logic",
        { MakePin("A", RTPinDataType::Boolean), MakePin("B", RTPinDataType::Boolean) },
        { MakePin("Result", RTPinDataType::Boolean) },
        "80C3F8", "Simple");

    reg("Not", "NOT", "Math/Logic",
        { MakePin("Value", RTPinDataType::Boolean) },
        { MakePin("Result", RTPinDataType::Boolean) },
        "80C3F8", "Simple");

    // --- Conversion (Math/Conversion) ---
    reg("IntToFloat", "Int to Float", "Math/Conversion",
        { MakePin("Value", RTPinDataType::Integer) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("FloatToInt", "Float to Int", "Math/Conversion",
        { MakePin("Value", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Integer) },
        "80C3F8", "Simple");

    reg("FloatToString", "Float to String", "Math/Conversion",
        { MakePin("Value", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::String) },
        "80C3F8", "Simple");

    // --- Functions (Math/Functions) ---
    reg("Abs", "Abs", "Math/Functions",
        { MakePin("Value", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Clamp", "Clamp", "Math/Functions",
        { MakePin("Value", RTPinDataType::Float), MakePin("Min", RTPinDataType::Float),
          MakePin("Max", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Min", "Min", "Math/Functions",
        { MakePin("A", RTPinDataType::Float), MakePin("B", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Max", "Max", "Math/Functions",
        { MakePin("A", RTPinDataType::Float), MakePin("B", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    // --- Random (Math/Random) ---
    reg("Random", "Random Float", "Math/Random",
        {},
        { MakePin("Value", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("RandomInRange", "Random In Range", "Math/Random",
        { MakePin("Min", RTPinDataType::Float), MakePin("B", RTPinDataType::Float) },
        { MakePin("Value", RTPinDataType::Float) },
        "80C3F8", "Simple");

    // --- Misc (直接在 Math 顶级) ---
    reg("Weird", "o.O", "Math",
        { MakePin("", RTPinDataType::Float) },
        { MakePin("", RTPinDataType::Float), MakePin("", RTPinDataType::Float) },
        "80C3F8", "Simple");
}
