// Runtime/BuiltinNodeDefs.cpp -- 内置节点定义注册实现（独立于编辑器）
//
// 合并了以下 9 个分类的节点定义：
//   Flow, Action, Math, Debug, String, Array, Tree, Houdini, Misc
//
// 该文件完全不依赖 ImGui 或 BlueprintEditor，可在任意环境中使用。

#include "BuiltinNodeDefs.h"

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 辅助函数（本地）
// ============================================================================

static PinDefinition MakePin(const char* name, PinDataType dt, bool isExec = false)
{
    PinDefinition p;
    p.name = name;
    p.dataType = dt;
    p.isExec = isExec;
    return p;
}

static PinDefinition MakeFlowPin(const char* name = "")
{
    return MakePin(name, PinDataType::Unknown, true);
}

// 统一的节点定义注册辅助函数（原先在 9 个 RegisterNodeDefs_* 函数中各自以 lambda 形式重复定义）
static void RegisterNodeDef(INodeRegistry& registry,
    const char* id, const char* name, const char* category,
    std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
    const char* color = "", const char* edType = "", const char* icon = "")
{
    NodeDefinition d;
    d.id = id;
    d.name = name;
    d.category = category;
    d.inputPins = std::move(inputs);
    d.outputPins = std::move(outputs);
    if (color[0]) d.color = color;
    if (edType[0]) d.customProperties["editorType"] = edType;
    if (icon[0])  d.icon = icon;
    registry.registerNode(d);
}

// ============================================================================
// 各分类节点定义注册
// ============================================================================

static void RegisterNodeDefs_Flow(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    reg("Branch", "Branch", "Flow",
        { MakeFlowPin(""), MakePin("Condition", PinDataType::Boolean) },
        { MakeFlowPin("True"), MakeFlowPin("False") },
        "8B3DB8");
    

    reg("DoN", "Do N", "Flow",
        { MakeFlowPin("Enter"), MakePin("N", PinDataType::Integer), MakeFlowPin("Reset") },
        { MakeFlowPin("Exit"), MakePin("Counter", PinDataType::Integer) });
    

    // ExecuteBlueprint: 完全异步模式（无 Sync 选项）
    {
        NodeDefinition d;
        d.id = "ExecuteBlueprint";
        d.name = "Execute Blueprint";
        d.category = "Flow";
        d.inputPins = {
            MakeFlowPin(""),
            MakePin("File",   PinDataType::String),
            MakePin("Params", PinDataType::String),   // optional JSON object: {"key":"value",...}
        };
        d.outputPins = {
            MakeFlowPin("Done"),
            MakeFlowPin("Completed"),
            MakePin("Success", PinDataType::Boolean),
            MakePin("Output",  PinDataType::String),
        };
        d.color = "FFA040";
        registry.registerNode(d);
    }

    reg("ForLoop", "For Loop", "Flow",
        { MakeFlowPin(""), MakePin("First Index", PinDataType::Integer),
          MakePin("Last Index", PinDataType::Integer) },
        { MakeFlowPin("Loop Body"), MakePin("Index", PinDataType::Integer),
          MakeFlowPin("Completed") },
        "2A7FAA");
    

    reg("WhileLoop", "While Loop", "Flow",
        { MakeFlowPin(""), MakePin("Condition", PinDataType::Boolean) },
        { MakeFlowPin("Loop Body"), MakeFlowPin("Completed") },
        "2A7FAA");
    

    reg("Delay", "Delay", "Flow",
        { MakeFlowPin(""), MakePin("Duration", PinDataType::Float) },
        { MakeFlowPin("Exec"), MakeFlowPin("Completed"), MakePin("TimerHandle", PinDataType::Integer) },
        "6A4AAA");
    

    reg("FlipFlop", "Flip Flop", "Flow",
        { MakeFlowPin("") },
        { MakeFlowPin("A"), MakeFlowPin("B"), MakePin("Is A", PinDataType::Boolean) },
        "2A7FAA");

    reg("Gate", "Gate", "Flow",
        { MakeFlowPin("Enter"), MakeFlowPin("Open"), MakeFlowPin("Close"),
          MakeFlowPin("Toggle") },
        { MakeFlowPin("Exit") },
        "2A7FAA");
    

    reg("DoOnce", "Do Once", "Flow",
        { MakeFlowPin(""), MakeFlowPin("Reset") },
        { MakeFlowPin("Completed") },
        "2A7FAA");
    

    // Sequence (Flow) — 按顺序执行多个 exec 输出
    {
        NodeDefinition d;
        d.id = "FlowSequence";
        d.name = "Sequence";
        d.category = "Flow";
        d.inputPins = { MakeFlowPin("") };
        d.outputPins = { MakeFlowPin("Then 0"), MakeFlowPin("Then 1"), MakeFlowPin("Then 2"), MakeFlowPin("Then 3") };
        registry.registerNode(d);
    }

    // Select — 三元选择节点
    reg("Select", "Select", "Flow",
        { MakePin("Condition", PinDataType::Boolean), MakePin("A", PinDataType::Any), MakePin("B", PinDataType::Any) },
        { MakePin("Result", PinDataType::Any) },
        "80C3F8", "Simple");

    // Switch on Int — 多分支选择
    {
        NodeDefinition d;
        d.id = "SwitchOnInt";
        d.name = "Switch on Int";
        d.category = "Flow";
        d.inputPins = { MakeFlowPin(""), MakePin("Selection", PinDataType::Integer) };
        d.outputPins = { MakeFlowPin("Default"), MakeFlowPin("0"), MakeFlowPin("1"), MakeFlowPin("2"), MakeFlowPin("3") };
        registry.registerNode(d);
    }

    // MultiGate — 依次激活多个输出（或随机）
    {
        NodeDefinition d;
        d.id = "MultiGate";
        d.name = "Multi Gate";
        d.category = "Flow";
        d.inputPins = { MakeFlowPin(""), MakeFlowPin("Reset"), MakePin("Loop", PinDataType::Boolean), MakePin("Random", PinDataType::Boolean) };
        d.outputPins = { MakeFlowPin("Out 0"), MakeFlowPin("Out 1"), MakeFlowPin("Out 2"), MakeFlowPin("Out 3") };
        registry.registerNode(d);
    }

    // ForLoopWithBreak — 可中断的 For Loop
    reg("ForLoopWithBreak", "For Loop with Break", "Flow",
        { MakeFlowPin(""), MakePin("First Index", PinDataType::Integer),
          MakePin("Last Index", PinDataType::Integer), MakeFlowPin("Break") },
        { MakeFlowPin("Loop Body"), MakePin("Index", PinDataType::Integer),
          MakeFlowPin("Completed") },
        "2A7FAA");
    

    // Switch on Bool — 根据 Bool 值二选一
    reg("SwitchOnBool", "Switch on Bool", "Flow",
        { MakeFlowPin(""), MakePin("Condition", PinDataType::Boolean) },
        { MakeFlowPin("True"), MakeFlowPin("False") },
        "8B3DB8");

    // Switch on String — 字符串多分支
    reg("SwitchOnString", "Switch on String", "Flow",
        { MakeFlowPin(""), MakePin("Selection", PinDataType::String),
          MakePin("Case 0", PinDataType::String), MakePin("Case 1", PinDataType::String),
          MakePin("Case 2", PinDataType::String), MakePin("Case 3", PinDataType::String),
          MakePin("Case 4", PinDataType::String), MakePin("Case 5", PinDataType::String) },
        { MakeFlowPin("Case 0"), MakeFlowPin("Case 1"),
          MakeFlowPin("Case 2"), MakeFlowPin("Case 3"),
          MakeFlowPin("Case 4"), MakeFlowPin("Case 5"), MakeFlowPin("Default") });
}

static void RegisterNodeDefs_Action(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    reg("InputActionFire", "InputAction Fire", "Action",
        {},
        { MakePin("", PinDataType::Custom, false), MakeFlowPin("Pressed"), MakeFlowPin("Released") },
        "FF8080");
    {
        auto* d = const_cast<NodeDefinition*>(registry.getNodeDefinition("InputActionFire"));
        if (d && !d->outputPins.empty())
            d->outputPins[0].customProperties["pinType"] = "Delegate";
    }

    reg("OutputAction", "OutputAction", "Action",
        { MakePin("Sample", PinDataType::Float), MakePin("Event", PinDataType::Custom, false) },
        { MakePin("Condition", PinDataType::Boolean) });
    {
        auto* d = const_cast<NodeDefinition*>(registry.getNodeDefinition("OutputAction"));
        if (d && d->inputPins.size() >= 2)
            d->inputPins[1].customProperties["pinType"] = "Delegate";
    }

    reg("SetTimer", "Set Timer", "Action",
        { MakeFlowPin(""), MakePin("Object", PinDataType::Object),
          MakePin("Function Name", PinDataType::Custom, false),
          MakePin("Time", PinDataType::Float), MakePin("Looping", PinDataType::Boolean) },
        { MakeFlowPin("Exec"), MakePin("TimerHandle", PinDataType::Integer) },
        "80C3F8");
    {
        auto* d = const_cast<NodeDefinition*>(registry.getNodeDefinition("SetTimer"));
        if (d && d->inputPins.size() >= 3)
            d->inputPins[2].customProperties["pinType"] = "Function";
    }

    reg("RemoveTimer", "Remove Timer", "Action",
        { MakeFlowPin(""), MakePin("TimerHandle", PinDataType::Integer) },
        { MakeFlowPin("Exec"), MakePin("Success", PinDataType::Boolean) },
        "FF6060");

    reg("PauseTimer", "Pause Timer", "Action",
        { MakeFlowPin(""), MakePin("TimerHandle", PinDataType::Integer) },
        { MakeFlowPin("Exec"), MakePin("Success", PinDataType::Boolean) },
        "FFA040");

    reg("ResumeTimer", "Resume Timer", "Action",
        { MakeFlowPin(""), MakePin("TimerHandle", PinDataType::Integer) },
        { MakeFlowPin("Exec"), MakePin("Success", PinDataType::Boolean) },
        "40C080");

    reg("CustomEvent", "Custom Event", "Action",
        {},
        { MakePin("Event", PinDataType::Custom, false), MakeFlowPin("Exec") },
        "A080F8");
    {
        auto* d = const_cast<NodeDefinition*>(registry.getNodeDefinition("CustomEvent"));
        if (d && !d->outputPins.empty())
            d->outputPins[0].customProperties["pinType"] = "Function";
    }

    reg("TraceByChannel", "Single Line Trace by Channel", "Action",
        { MakeFlowPin(""), MakeFlowPin("Start"), MakePin("End", PinDataType::Integer),
          MakePin("Trace Channel", PinDataType::Float), MakePin("Trace Complex", PinDataType::Boolean),
          MakePin("Actors to Ignore", PinDataType::Integer), MakePin("Draw Debug Type", PinDataType::Boolean),
          MakePin("Ignore Self", PinDataType::Boolean) },
        { MakeFlowPin(""), MakePin("Out Hit", PinDataType::Float), MakePin("Return Value", PinDataType::Boolean) },
        "FF8040");
}

static void RegisterNodeDefs_Math(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    // --- Arithmetic (Math/Arithmetic) ---
    reg("Add", "+", "Math/Arithmetic",
        { MakePin("A", PinDataType::Float), MakePin("B", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Subtract", "-", "Math/Arithmetic",
        { MakePin("A", PinDataType::Float), MakePin("B", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Multiply", "*", "Math/Arithmetic",
        { MakePin("A", PinDataType::Float), MakePin("B", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Divide", "/", "Math/Arithmetic",
        { MakePin("A", PinDataType::Float), MakePin("B", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    // --- Comparison (Math/Comparison) ---
    reg("Less", "<", "Math/Comparison",
        { MakePin("", PinDataType::Float), MakePin("", PinDataType::Float) },
        { MakePin("", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Greater", ">", "Math/Comparison",
        { MakePin("", PinDataType::Float), MakePin("", PinDataType::Float) },
        { MakePin("", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Equal", "==", "Math/Comparison",
        { MakePin("A", PinDataType::Float), MakePin("B", PinDataType::Float) },
        { MakePin("Result", PinDataType::Boolean) },
        "80C3F8", "Simple");

    // --- Logic (Math/Logic) ---
    reg("And", "AND", "Math/Logic",
        { MakePin("A", PinDataType::Boolean), MakePin("B", PinDataType::Boolean) },
        { MakePin("Result", PinDataType::Boolean) },
        "80C3F8", "Simple");

    reg("Or", "OR", "Math/Logic",
        { MakePin("A", PinDataType::Boolean), MakePin("B", PinDataType::Boolean) },
        { MakePin("Result", PinDataType::Boolean) },
        "80C3F8", "Simple");

    reg("Not", "NOT", "Math/Logic",
        { MakePin("Value", PinDataType::Boolean) },
        { MakePin("Result", PinDataType::Boolean) },
        "80C3F8", "Simple");

    // --- Conversion (Math/Conversion) ---
    reg("IntToFloat", "Int to Float", "Math/Conversion",
        { MakePin("Value", PinDataType::Integer) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("FloatToInt", "Float to Int", "Math/Conversion",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::Integer) },
        "80C3F8", "Simple");

    reg("FloatToBool", "Float to Bool", "Math/Conversion",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::Boolean) },
        "80C3F8", "Simple");

    reg("IntToString", "Int to String", "Math/Conversion",
        { MakePin("Value", PinDataType::Integer) },
        { MakePin("Result", PinDataType::String) },
        "80C3F8", "Simple");

    reg("FloatToString", "Float to String", "Math/Conversion",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::String) },
        "80C3F8", "Simple");

    reg("BoolToInt", "Bool to Int", "Math/Conversion",
        { MakePin("Value", PinDataType::Boolean) },
        { MakePin("Result", PinDataType::Integer) },
        "80C3F8", "Simple");

    reg("IntToBool", "Int to Bool", "Math/Conversion",
        { MakePin("Value", PinDataType::Integer) },
        { MakePin("Result", PinDataType::Boolean) },
        "80C3F8", "Simple");

    reg("BoolToFloat", "Bool to Float", "Math/Conversion",
        { MakePin("Value", PinDataType::Boolean) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    // --- Functions (Math/Functions) ---
    reg("Abs", "Abs", "Math/Functions",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Clamp", "Clamp", "Math/Functions",
        { MakePin("Value", PinDataType::Float), MakePin("Min", PinDataType::Float),
          MakePin("Max", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Min", "Min", "Math/Functions",
        { MakePin("A", PinDataType::Float), MakePin("B", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Max", "Max", "Math/Functions",
        { MakePin("A", PinDataType::Float), MakePin("B", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    // --- Random (Math/Random) ---
    reg("Random", "Random Float", "Math/Random",
        {},
        { MakePin("Value", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("RandomInRange", "Random In Range", "Math/Random",
        { MakePin("Min", PinDataType::Float), MakePin("B", PinDataType::Float) },
        { MakePin("Value", PinDataType::Float) },
        "80C3F8", "Simple");

    // --- More Arithmetic ---
    reg("Modulo", "%", "Math/Arithmetic",
        { MakePin("A", PinDataType::Float), MakePin("B", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Power", "Power", "Math/Arithmetic",
        { MakePin("Base", PinDataType::Float), MakePin("Exponent", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Negate", "Negate", "Math/Arithmetic",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    // --- More Comparison ---
    reg("NotEqual", "!=", "Math/Comparison",
        { MakePin("A", PinDataType::Float), MakePin("B", PinDataType::Float) },
        { MakePin("Result", PinDataType::Boolean) },
        "80C3F8", "Simple");

    reg("LessEqual", "<=", "Math/Comparison",
        { MakePin("A", PinDataType::Float), MakePin("B", PinDataType::Float) },
        { MakePin("Result", PinDataType::Boolean) },
        "80C3F8", "Simple");

    reg("GreaterEqual", ">=", "Math/Comparison",
        { MakePin("A", PinDataType::Float), MakePin("B", PinDataType::Float) },
        { MakePin("Result", PinDataType::Boolean) },
        "80C3F8", "Simple");

    // --- More Logic ---
    reg("Nand", "NAND", "Math/Logic",
        { MakePin("A", PinDataType::Boolean), MakePin("B", PinDataType::Boolean) },
        { MakePin("Result", PinDataType::Boolean) },
        "80C3F8", "Simple");

    reg("Nor", "NOR", "Math/Logic",
        { MakePin("A", PinDataType::Boolean), MakePin("B", PinDataType::Boolean) },
        { MakePin("Result", PinDataType::Boolean) },
        "80C3F8", "Simple");

    reg("Xor", "XOR", "Math/Logic",
        { MakePin("A", PinDataType::Boolean), MakePin("B", PinDataType::Boolean) },
        { MakePin("Result", PinDataType::Boolean) },
        "80C3F8", "Simple");

    // --- More Conversion ---
    reg("StringToInt", "String to Int", "Math/Conversion",
        { MakePin("Value", PinDataType::String) },
        { MakePin("Result", PinDataType::Integer), MakePin("Valid", PinDataType::Boolean) },
        "80C3F8", "Simple");

    reg("StringToFloat", "String to Float", "Math/Conversion",
        { MakePin("Value", PinDataType::String) },
        { MakePin("Result", PinDataType::Float), MakePin("Valid", PinDataType::Boolean) },
        "80C3F8", "Simple");

    reg("BoolToString", "Bool to String", "Math/Conversion",
        { MakePin("Value", PinDataType::Boolean) },
        { MakePin("Result", PinDataType::String) },
        "80C3F8", "Simple");

    // --- More Functions ---
    reg("Sqrt", "Sqrt", "Math/Functions",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Sin", "Sin", "Math/Functions",
        { MakePin("Value (Rad)", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Cos", "Cos", "Math/Functions",
        { MakePin("Value (Rad)", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Tan", "Tan", "Math/Functions",
        { MakePin("Value (Rad)", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Atan2", "Atan2", "Math/Functions",
        { MakePin("Y", PinDataType::Float), MakePin("X", PinDataType::Float) },
        { MakePin("Result (Rad)", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Asin", "Asin", "Math/Functions",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result (Rad)", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Acos", "Acos", "Math/Functions",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result (Rad)", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Atan", "Atan", "Math/Functions",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result (Rad)", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Hypot", "Hypot", "Math/Functions",
        { MakePin("A", PinDataType::Float), MakePin("B", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("DegreesToRadians", "Degrees to Radians", "Math/Functions",
        { MakePin("Degrees", PinDataType::Float) },
        { MakePin("Radians", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("RadiansToDegrees", "Radians to Degrees", "Math/Functions",
        { MakePin("Radians", PinDataType::Float) },
        { MakePin("Degrees", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Lerp", "Lerp", "Math/Functions",
        { MakePin("A", PinDataType::Float), MakePin("B", PinDataType::Float), MakePin("Alpha", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("MapRange", "Map Range", "Math/Functions",
        { MakePin("Value", PinDataType::Float), MakePin("InMin", PinDataType::Float), MakePin("InMax", PinDataType::Float),
          MakePin("OutMin", PinDataType::Float), MakePin("OutMax", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Ceil", "Ceil", "Math/Functions",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Floor", "Floor", "Math/Functions",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Round", "Round", "Math/Functions",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Sign", "Sign", "Math/Functions",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    // --- Constants (Math/Constants) ---
    reg("PI", "PI", "Math/Constants",
        {},
        { MakePin("Value", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("E", "E", "Math/Constants",
        {},
        { MakePin("Value", PinDataType::Float) },
        "80C3F8", "Simple");

    // --- Advanced Functions (Math/Functions) ---
    reg("InverseLerp", "Inverse Lerp", "Math/Functions",
        { MakePin("A", PinDataType::Float), MakePin("B", PinDataType::Float), MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Remap01", "Remap 0-1", "Math/Functions",
        { MakePin("Value", PinDataType::Float), MakePin("InMin", PinDataType::Float), MakePin("InMax", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("DegreesToRadians", "Degrees to Radians", "Math/Conversion",
        { MakePin("Degrees", PinDataType::Float) },
        { MakePin("Radians", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("RadiansToDegrees", "Radians to Degrees", "Math/Conversion",
        { MakePin("Radians", PinDataType::Float) },
        { MakePin("Degrees", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Wrap", "Wrap", "Math/Functions",
        { MakePin("Value", PinDataType::Float), MakePin("Min", PinDataType::Float), MakePin("Max", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Snap", "Snap to Grid", "Math/Functions",
        { MakePin("Value", PinDataType::Float), MakePin("GridSize", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Log2", "Log2", "Math/Functions",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Log10", "Log10", "Math/Functions",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    reg("Exp", "Exp", "Math/Functions",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");

    // --- Misc (直接在 Math 顶级) ---
    reg("Weird", "o.O", "Math",
        { MakePin("", PinDataType::Float) },
        { MakePin("", PinDataType::Float), MakePin("", PinDataType::Float) },
        "80C3F8", "Simple");
}

static void RegisterNodeDefs_Debug(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    reg("PrintString", "Print String", "Debug",
        { MakeFlowPin(""), MakePin("In String", PinDataType::String) },
        { MakeFlowPin("") },
        "3D8B45");
    

    reg("Log", "Log", "Debug",
        { MakeFlowPin(""), MakePin("Message", PinDataType::String) },
        { MakeFlowPin("") },
        "3D8B45");
    

    // Assert — 断言条件为真，否则输出错误
    reg("Assert", "Assert", "Debug",
        { MakeFlowPin(""), MakePin("Condition", PinDataType::Boolean), MakePin("Message", PinDataType::String) },
        { MakeFlowPin("") },
        "FF4040");

    // InspectValue — 透传任意值，同时 Print 该值
    reg("InspectValue", "Inspect Value", "Debug",
        { MakePin("Value", PinDataType::Any) },
        { MakePin("Value", PinDataType::Any) },
        "", "Simple");

    // BreakOnCondition — Condition 为 true 时暂停 runner
    reg("BreakOnCondition", "Break On Condition", "Debug",
        { MakeFlowPin(""), MakePin("Condition", PinDataType::Boolean) },
        { MakeFlowPin("") },
        "FF8040");

    // FormatLog — 带格式化的日志输出
    reg("FormatLog", "Format Log", "Debug",
        { MakeFlowPin(""), MakePin("Format", PinDataType::String),
          MakePin("Arg 0", PinDataType::Any), MakePin("Arg 1", PinDataType::Any) },
        { MakeFlowPin("") });
    {
        auto* d = const_cast<NodeDefinition*>(registry.getNodeDefinition("FormatLog"));
        if (d) d->customProperties["dynamicInputs"] = "Any";
    }
}

static void RegisterNodeDefs_String(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    reg("MakeString", "Make String", "Misc/String",
        { MakePin("Value", PinDataType::String) },
        { MakePin("String", PinDataType::String) },
        "", "Simple");

    reg("AppendString", "Append String", "Misc/String",
        { MakePin("A", PinDataType::String), MakePin("B", PinDataType::String) },
        { MakePin("Result", PinDataType::String) },
        "", "Simple");
    {
        auto* d = const_cast<NodeDefinition*>(registry.getNodeDefinition("AppendString"));
        if (d) d->customProperties["dynamicInputs"] = "String";
    }

    reg("StringLength", "String Length", "Misc/String",
        { MakePin("String", PinDataType::String) },
        { MakePin("Length", PinDataType::Integer) },
        "", "Simple");

    reg("StringEquals", "String Equals", "Misc/String",
        { MakePin("A", PinDataType::String), MakePin("B", PinDataType::String) },
        { MakePin("Result", PinDataType::Boolean) },
        "", "Simple");

    reg("StringEqualsIgnoreCase", "String Equals (Ignore Case)", "Misc/String",
        { MakePin("A", PinDataType::String), MakePin("B", PinDataType::String) },
        { MakePin("Result", PinDataType::Boolean) },
        "", "Simple");

    reg("StringContains", "String Contains", "Misc/String",
        { MakePin("String", PinDataType::String), MakePin("Substring", PinDataType::String) },
        { MakePin("Result", PinDataType::Boolean) },
        "", "Simple");

    reg("StringStartsWith", "String Starts With", "Misc/String",
        { MakePin("String", PinDataType::String), MakePin("Prefix", PinDataType::String) },
        { MakePin("Result", PinDataType::Boolean) },
        "", "Simple");

    reg("StringEndsWith", "String Ends With", "Misc/String",
        { MakePin("String", PinDataType::String), MakePin("Suffix", PinDataType::String) },
        { MakePin("Result", PinDataType::Boolean) },
        "", "Simple");

    reg("StringReplace", "String Replace", "Misc/String",
        { MakePin("String", PinDataType::String), MakePin("From", PinDataType::String),
          MakePin("To", PinDataType::String) },
        { MakePin("Result", PinDataType::String) },
        "", "Simple");

    reg("StringToUpper", "String To Upper", "Misc/String",
        { MakePin("String", PinDataType::String) },
        { MakePin("Result", PinDataType::String) },
        "", "Simple");

    reg("StringToLower", "String To Lower", "Misc/String",
        { MakePin("String", PinDataType::String) },
        { MakePin("Result", PinDataType::String) },
        "", "Simple");

    reg("StringSubstring", "String Substring", "Misc/String",
        { MakePin("String", PinDataType::String), MakePin("Start", PinDataType::Integer),
          MakePin("Count", PinDataType::Integer) },
        { MakePin("Result", PinDataType::String) },
        "", "Simple");

    reg("StringFind", "String Find", "Misc/String",
        { MakePin("String", PinDataType::String), MakePin("Substring", PinDataType::String) },
        { MakePin("Index", PinDataType::Integer), MakePin("Found", PinDataType::Boolean) },
        "", "Simple");

    reg("StringSplit", "String Split", "Misc/String",
        { MakePin("String", PinDataType::String), MakePin("Delimiter", PinDataType::String) },
        { MakePin("Array", PinDataType::Array), MakePin("Count", PinDataType::Integer) },
        "", "Simple");

    reg("StringTrimmed", "String Trimmed", "Misc/String",
        { MakePin("String", PinDataType::String) },
        { MakePin("Result", PinDataType::String) },
        "", "Simple");

    // FormatString — 格式化字符串 ({0}, {1}, ... 占位符)
    reg("FormatString", "Format String", "Misc/String",
        { MakePin("Format", PinDataType::String) },
        { MakePin("Result", PinDataType::String) },
        "", "Simple");
    {
        auto* d = const_cast<NodeDefinition*>(registry.getNodeDefinition("FormatString"));
        if (d) d->customProperties["dynamicInputs"] = "Any";
    }

    // StringJoin — 用分隔符连接数组中的字符串
    reg("StringJoin", "String Join", "Misc/String",
        { MakePin("Array", PinDataType::Array), MakePin("Separator", PinDataType::String) },
        { MakePin("Result", PinDataType::String) },
        "", "Simple");

    // StringRepeat — 重复字符串 N 次
    reg("StringRepeat", "String Repeat", "Misc/String",
        { MakePin("String", PinDataType::String), MakePin("Count", PinDataType::Integer) },
        { MakePin("Result", PinDataType::String) },
        "", "Simple");

    // StringPadLeft — 左侧填充
    reg("StringPadLeft", "String Pad Left", "Misc/String",
        { MakePin("String", PinDataType::String), MakePin("TotalWidth", PinDataType::Integer), MakePin("PadChar", PinDataType::String) },
        { MakePin("Result", PinDataType::String) },
        "", "Simple");

    // StringPadRight — 右侧填充
    reg("StringPadRight", "String Pad Right", "Misc/String",
        { MakePin("String", PinDataType::String), MakePin("TotalWidth", PinDataType::Integer), MakePin("PadChar", PinDataType::String) },
        { MakePin("Result", PinDataType::String) },
        "", "Simple");

    // CharAt — 获取指定位置字符
    reg("CharAt", "Char At", "Misc/String",
        { MakePin("String", PinDataType::String), MakePin("Index", PinDataType::Integer) },
        { MakePin("Char", PinDataType::String), MakePin("Valid", PinDataType::Boolean) },
        "", "Simple");

    // StringReverse — 反转字符串
    reg("StringReverse", "String Reverse", "Misc/String",
        { MakePin("String", PinDataType::String) },
        { MakePin("Result", PinDataType::String) },
        "", "Simple");

    // ── 新增字符串节点 ──────────────────────────────────────────────────────
    reg("StringCount", "String Count", "Misc/String",
        { MakePin("String", PinDataType::String), MakePin("Substring", PinDataType::String) },
        { MakePin("Count", PinDataType::Integer) },
        "", "Simple");

    reg("StringIsEmpty", "String Is Empty", "Misc/String",
        { MakePin("String", PinDataType::String) },
        { MakePin("Result", PinDataType::Boolean) },
        "", "Simple");

    reg("StringInsert", "String Insert", "Misc/String",
        { MakePin("String", PinDataType::String), MakePin("Position", PinDataType::Integer), MakePin("Insert", PinDataType::String) },
        { MakePin("Result", PinDataType::String) },
        "", "Simple");

    reg("StringRemove", "String Remove", "Misc/String",
        { MakePin("String", PinDataType::String), MakePin("Position", PinDataType::Integer), MakePin("Length", PinDataType::Integer) },
        { MakePin("Result", PinDataType::String) },
        "", "Simple");
}

static void RegisterNodeDefs_Array(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    reg("ArrayLength", "Array Length", "Misc/Array",
        { MakePin("Array", PinDataType::Array) },
        { MakePin("Length", PinDataType::Integer) },
        "", "Simple");

    reg("ArrayGet", "Array Get", "Misc/Array",
        { MakePin("Array", PinDataType::Array), MakePin("Index", PinDataType::Integer) },
        { MakePin("Element", PinDataType::Any), MakePin("Valid", PinDataType::Boolean) },
        "", "Simple");

    reg("ArraySet", "Array Set", "Misc/Array",
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array), MakePin("Index", PinDataType::Integer), MakePin("Element", PinDataType::Any) },
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array) });

    reg("ArrayRemoveAt", "Array Remove At", "Misc/Array",
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array), MakePin("Index", PinDataType::Integer) },
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array), MakePin("Valid", PinDataType::Boolean) });

    reg("ArrayClear", "Array Clear", "Misc/Array",
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array) },
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array) });

    reg("ForEachLoop", "For Each Loop", "Misc/Array",
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array) },
        { MakeFlowPin("Loop Body"), MakePin("Array Element", PinDataType::Any),
          MakePin("Array Index", PinDataType::Integer), MakeFlowPin("Completed") });

    reg("ArrayAdd", "Array Add", "Misc/Array",
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array), MakePin("Element", PinDataType::Any) },
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array), MakePin("New Length", PinDataType::Integer) });

    reg("ArrayInsert", "Array Insert", "Misc/Array",
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array), MakePin("Index", PinDataType::Integer), MakePin("Element", PinDataType::Any) },
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array) });

    reg("ArrayContains", "Array Contains", "Misc/Array",
        { MakePin("Array", PinDataType::Array), MakePin("Element", PinDataType::Any) },
        { MakePin("Found", PinDataType::Boolean), MakePin("Index", PinDataType::Integer) },
        "", "Simple");

    reg("ArrayReverse", "Array Reverse", "Misc/Array",
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array) },
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array) });

    reg("MakeArray", "Make Array", "Misc/Array",
        { MakePin("Element 0", PinDataType::Any) },
        { MakePin("Array", PinDataType::Array) },
        "", "Simple");
    {
        auto* d = const_cast<NodeDefinition*>(registry.getNodeDefinition("MakeArray"));
        if (d) d->customProperties["dynamicInputs"] = "Any";
    }

    // Array Find — 查找第一个匹配元素的索引（纯查询，不修改数组）
    reg("ArrayFind", "Array Find", "Misc/Array",
        { MakePin("Array", PinDataType::Array), MakePin("Element", PinDataType::Any) },
        { MakePin("Index", PinDataType::Integer), MakePin("Found", PinDataType::Boolean) },
        "", "Simple");

    // Array Slice — 返回子数组 [Start, End)（不含 End）
    reg("ArraySlice", "Array Slice", "Misc/Array",
        { MakePin("Array", PinDataType::Array),
          MakePin("Start", PinDataType::Integer), MakePin("End", PinDataType::Integer) },
        { MakePin("Result", PinDataType::Array) },
        "", "Simple");

    // Array Concat — 拼接两个数组
    reg("ArrayConcat", "Array Concat", "Misc/Array",
        { MakePin("Array A", PinDataType::Array), MakePin("Array B", PinDataType::Array) },
        { MakePin("Result", PinDataType::Array) },
        "", "Simple");

    // Array Unique — 去重（保留第一次出现顺序）
    reg("ArrayUnique", "Array Unique", "Misc/Array",
        { MakePin("Array", PinDataType::Array) },
        { MakePin("Result", PinDataType::Array) },
        "", "Simple");

    // Array Sort — 排序（仅支持同质数组：全 Int / 全 Float / 全 String）
    reg("ArraySort", "Array Sort", "Misc/Array",
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array),
          MakePin("Descending", PinDataType::Boolean) },
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array) });

    // Array Last — 获取最后一个元素
    reg("ArrayLast", "Array Last", "Misc/Array",
        { MakePin("Array", PinDataType::Array) },
        { MakePin("Element", PinDataType::Any), MakePin("Valid", PinDataType::Boolean) },
        "", "Simple");

    // Array First — 获取第一个元素
    reg("ArrayFirst", "Array First", "Misc/Array",
        { MakePin("Array", PinDataType::Array) },
        { MakePin("Element", PinDataType::Any), MakePin("Valid", PinDataType::Boolean) },
        "", "Simple");

    // Array Remove — 删除第一个匹配的元素（按值查找）
    reg("ArrayRemove", "Array Remove", "Misc/Array",
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array), MakePin("Element", PinDataType::Any) },
        { MakeFlowPin(""), MakePin("Array", PinDataType::Array), MakePin("Removed", PinDataType::Boolean) });
}

static void RegisterNodeDefs_Map(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    reg("MakeMap", "Make Map", "Misc/Map",
        { MakePin("Key 0", PinDataType::Any), MakePin("Value 0", PinDataType::Any) },
        { MakePin("Map", PinDataType::Map) },
        "", "Simple");
    {
        auto* d = const_cast<NodeDefinition*>(registry.getNodeDefinition("MakeMap"));
        if (d) d->customProperties["dynamicInputs"] = "Any";
    }

    reg("MapGet", "Map Get", "Misc/Map",
        { MakePin("Map", PinDataType::Map), MakePin("Key", PinDataType::Any) },
        { MakePin("Value", PinDataType::Any), MakePin("Found", PinDataType::Boolean) },
        "20CCDD", "Simple");
    

    reg("MapSet", "Map Set", "Misc/Map",
        { MakeFlowPin(""), MakePin("Map", PinDataType::Map), MakePin("Key", PinDataType::Any), MakePin("Value", PinDataType::Any) },
        { MakeFlowPin(""), MakePin("Result", PinDataType::Map) },
        "20CCDD");
    

    reg("MapRemove", "Map Remove", "Misc/Map",
        { MakeFlowPin(""), MakePin("Map", PinDataType::Map), MakePin("Key", PinDataType::Any) },
        { MakeFlowPin(""), MakePin("Result", PinDataType::Map) },
        "20CCDD");
    

    reg("MapHasKey", "Map Has Key", "Misc/Map",
        { MakePin("Map", PinDataType::Map), MakePin("Key", PinDataType::Any) },
        { MakePin("Result", PinDataType::Boolean) },
        "20CCDD", "Simple");
    

    reg("MapLength", "Map Length", "Misc/Map",
        { MakePin("Map", PinDataType::Map) },
        { MakePin("Length", PinDataType::Integer) },
        "20CCDD", "Simple");
    

    // Keep legacy MapSize for backward compatibility
    reg("MapSize", "Map Size", "Misc/Map",
        { MakePin("Map", PinDataType::Map) },
        { MakePin("Size", PinDataType::Integer) },
        "20CCDD", "Simple");

    reg("MapKeys", "Map Keys", "Misc/Map",
        { MakePin("Map", PinDataType::Map) },
        { MakePin("Keys", PinDataType::Array) },
        "20CCDD", "Simple");
    

    reg("MapValues", "Map Values", "Misc/Map",
        { MakePin("Map", PinDataType::Map) },
        { MakePin("Values", PinDataType::Array) },
        "20CCDD", "Simple");
    

    reg("MapClear", "Map Clear", "Misc/Map",
        { MakeFlowPin(""), MakePin("Map", PinDataType::Map) },
        { MakeFlowPin(""), MakePin("Result", PinDataType::Map) },
        "20CCDD");
    

    reg("MapMerge", "Map Merge", "Misc/Map",
        { MakeFlowPin(""), MakePin("Map A", PinDataType::Map), MakePin("Map B", PinDataType::Map) },
        { MakeFlowPin(""), MakePin("Map", PinDataType::Map) },
        "20CCDD");

    reg("ForEachMapLoop", "For Each Map", "Misc/Map",
        { MakeFlowPin(""), MakePin("Map", PinDataType::Map) },
        { MakeFlowPin("Loop Body"), MakePin("Key", PinDataType::Any),
          MakePin("Value", PinDataType::Any), MakeFlowPin("Completed") });

    // ── Set 节点定义 ──────────────────────────────────────────────────────
    reg("MakeSet", "Make Set", "Misc/Set",
        { MakePin("Value 0", PinDataType::Any) },
        { MakePin("Set", PinDataType::Set) },
        "B464F0", "Simple");
    {
        auto* d = const_cast<NodeDefinition*>(registry.getNodeDefinition("MakeSet"));
        if (d) d->customProperties["dynamicInputs"] = "Any";
    }

    reg("SetAdd", "Set Add", "Misc/Set",
        { MakeFlowPin(""), MakePin("Set", PinDataType::Set), MakePin("Value", PinDataType::Any) },
        { MakeFlowPin(""), MakePin("Set", PinDataType::Set), MakePin("Added", PinDataType::Boolean) },
        "B464F0");

    reg("SetRemove", "Set Remove", "Misc/Set",
        { MakeFlowPin(""), MakePin("Set", PinDataType::Set), MakePin("Value", PinDataType::Any) },
        { MakeFlowPin(""), MakePin("Set", PinDataType::Set), MakePin("Removed", PinDataType::Boolean) },
        "B464F0");

    reg("SetContains", "Set Contains", "Misc/Set",
        { MakePin("Set", PinDataType::Set), MakePin("Value", PinDataType::Any) },
        { MakePin("Result", PinDataType::Boolean) },
        "B464F0", "Simple");

    reg("SetSize", "Set Size", "Misc/Set",
        { MakePin("Set", PinDataType::Set) },
        { MakePin("Size", PinDataType::Integer) },
        "B464F0", "Simple");

    reg("SetLength", "Set Length", "Misc/Set",
        { MakePin("Set", PinDataType::Set) },
        { MakePin("Size", PinDataType::Integer) },
        "B464F0", "Simple");

    reg("SetClear", "Set Clear", "Misc/Set",
        { MakeFlowPin(""), MakePin("Set", PinDataType::Set) },
        { MakeFlowPin(""), MakePin("Set", PinDataType::Set) },
        "B464F0");

    reg("SetToArray", "Set To Array", "Misc/Set",
        { MakePin("Set", PinDataType::Set) },
        { MakePin("Array", PinDataType::Array) },
        "B464F0", "Simple");

    reg("SetFromArray", "Set From Array", "Misc/Set",
        { MakePin("Array", PinDataType::Array) },
        { MakePin("Set", PinDataType::Set) },
        "B464F0", "Simple");

    reg("ForEachSetLoop", "For Each Set", "Misc/Set",
        { MakeFlowPin(""), MakePin("Set", PinDataType::Set) },
        { MakeFlowPin("Loop Body"), MakePin("Value", PinDataType::Any),
          MakePin("Index", PinDataType::Integer), MakeFlowPin("Completed") },
        "B464F0");
}

static void RegisterNodeDefs_Tree(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    reg("Sequence", "Sequence", "Tree",
        { MakeFlowPin("") },
        { MakeFlowPin("") },
        "", "Tree");

    reg("MoveTo", "Move To", "Tree",
        { MakeFlowPin("") }, {},
        "", "Tree");

    reg("RandomWait", "Random Wait", "Tree",
        { MakeFlowPin("") }, {},
        "", "Tree");
}

static void RegisterNodeDefs_Houdini(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    reg("HoudiniTransform", "Transform", "Houdini",
        { MakeFlowPin("") },
        { MakeFlowPin("") },
        "", "Houdini");

    reg("HoudiniGroup", "Group", "Houdini",
        { MakeFlowPin(""), MakeFlowPin("") },
        { MakeFlowPin("") },
        "", "Houdini");
}

static void RegisterNodeDefs_Time(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    // GetTime — 获取当前时间（秒，自 epoch）
    reg("GetTime", "Get Time", "Time",
        {},
        { MakePin("Seconds", PinDataType::Float) },
        "40C0C0", "Simple");

    // DeltaTime — 获取帧间隔时间
    reg("DeltaTime", "Delta Time", "Time",
        {},
        { MakePin("Seconds", PinDataType::Float) },
        "40C0C0", "Simple");

    // TimeSince — 计算距某时间戳过去了多少秒
    reg("TimeSince", "Time Since", "Time",
        { MakePin("Timestamp", PinDataType::Float) },
        { MakePin("Elapsed", PinDataType::Float) },
        "40C0C0", "Simple");

    // FormatTime — 格式化时间为可读字符串
    reg("FormatTime", "Format Time", "Time",
        { MakePin("Seconds", PinDataType::Float) },
        { MakePin("Formatted", PinDataType::String) },
        "40C0C0", "Simple");

    // TimerInfo — 查询计时器状态
    reg("TimerInfo", "Timer Info", "Time",
        { MakePin("TimerHandle", PinDataType::Integer) },
        { MakePin("IsActive", PinDataType::Boolean), MakePin("IsPaused", PinDataType::Boolean),
          MakePin("Elapsed", PinDataType::Float), MakePin("Remaining", PinDataType::Float) },
        "40C0C0", "Simple");
}

static void RegisterNodeDefs_Data(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    // ToJSON — 将值序列化为 JSON 字符串
    reg("ToJSON", "To JSON", "Data",
        { MakePin("Value", PinDataType::Any) },
        { MakePin("JSON", PinDataType::String) },
        "C080F8", "Simple");

    // FromJSON — 从 JSON 字符串解析值
    reg("FromJSON", "From JSON", "Data",
        { MakePin("JSON", PinDataType::String) },
        { MakePin("Value", PinDataType::Any), MakePin("Valid", PinDataType::Boolean) },
        "C080F8", "Simple");

    // HasKey — 检查 JSON 对象中是否含有指定键
    reg("HasKey", "Has Key", "Data",
        { MakePin("JSON", PinDataType::String), MakePin("Key", PinDataType::String) },
        { MakePin("Result", PinDataType::Boolean) },
        "C080F8", "Simple");

    // GetField — 从 JSON 字符串中获取指定字段
    reg("GetField", "Get Field", "Data",
        { MakePin("JSON", PinDataType::String), MakePin("Key", PinDataType::String) },
        { MakePin("Value", PinDataType::String), MakePin("Found", PinDataType::Boolean) },
        "C080F8", "Simple");

    // ArrayToString — 将数组转为字符串表示
    reg("ArrayToString", "Array to String", "Data",
        { MakePin("Array", PinDataType::Array) },
        { MakePin("String", PinDataType::String) },
        "C080F8", "Simple");
}

static void RegisterNodeDefs_Misc(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    reg("Message", "Message", "Misc",
        {},
        { MakePin("Message", PinDataType::String) },
        "80C3F8", "Simple");

    // Comment (special)
    {
        NodeDefinition d;
        d.id = "Comment";
        d.name = "Comment";
        d.category = "Misc";
        d.customProperties["editorType"] = "Comment";
        d.defaultSize = {300, 200};
        registry.registerNode(d);
    }

    reg("GetVariable", "Get Variable", "Misc",
        { MakePin("Name", PinDataType::String) },
        { MakePin("Value", PinDataType::String) },
        "", "Simple");

    reg("SetVariable", "Set Variable", "Misc",
        { MakeFlowPin(""), MakePin("Name", PinDataType::String),
          MakePin("Value", PinDataType::String) },
        { MakeFlowPin("") });

    reg("IsValid", "Is Valid", "Misc",
        { MakePin("Value", PinDataType::Any) },
        { MakePin("Is Valid", PinDataType::Boolean) },
        "", "Simple");

    reg("MakeLiteralBool", "Make Literal Bool", "Misc",
        { MakePin("Value", PinDataType::Boolean) },
        { MakePin("Result", PinDataType::Boolean) },
        "", "Simple");

    reg("MakeLiteralInt", "Make Literal Int", "Misc",
        { MakePin("Value", PinDataType::Integer) },
        { MakePin("Result", PinDataType::Integer) },
        "", "Simple");

    reg("MakeLiteralFloat", "Make Literal Float", "Misc",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::Float) },
        "", "Simple");

    reg("MakeLiteralString", "Make Literal String", "Misc",
        { MakePin("Value", PinDataType::String) },
        { MakePin("Result", PinDataType::String) },
        "", "Simple");

    // ── Object 引脚 Demo 节点（Object/Demo 分类）─────────────────────────
    // 演示 PinDataType::Object 类型引脚的使用场景
    // Object 值在运行时以字符串存储（对象引用 ID / 路径 / 名称）

    // MakeObject — 用字符串 ID 构造一个对象引用
    reg("Object.Make", "Make Object Ref", "Object",
        { MakePin("ObjectId", PinDataType::String) },
        { MakePin("Object", PinDataType::Object) },
        "C86428", "Simple");

    // GetObjectId — 从对象引用中提取 ID 字符串
    reg("Object.GetId", "Get Object Id", "Object",
        { MakePin("Object", PinDataType::Object) },
        { MakePin("ObjectId", PinDataType::String) },
        "C86428", "Simple");

    // IsObjectValid — 检查对象引用是否非空
    reg("Object.IsValid", "Is Object Valid", "Object",
        { MakePin("Object", PinDataType::Object) },
        { MakePin("Is Valid", PinDataType::Boolean) },
        "C86428", "Simple");

    // SetObjectProperty — 在全局属性表中为对象设置一个命名属性
    reg("Object.SetProperty", "Set Object Property", "Object",
        { MakeFlowPin(""),
          MakePin("Object",   PinDataType::Object),
          MakePin("Key",      PinDataType::String),
          MakePin("Value",    PinDataType::String) },
        { MakeFlowPin("") },
        "C86428");

    // GetObjectProperty — 读取对象命名属性
    reg("Object.GetProperty", "Get Object Property", "Object",
        { MakePin("Object",   PinDataType::Object),
          MakePin("Key",      PinDataType::String) },
        { MakePin("Value",    PinDataType::String),
          MakePin("Found",    PinDataType::Boolean) },
        "C86428", "Simple");

    // PrintObject — 打印对象 ID 及其所有属性
    reg("Object.Print", "Print Object", "Object",
        { MakeFlowPin(""),
          MakePin("Object",   PinDataType::Object),
          MakePin("Label",    PinDataType::String) },
        { MakeFlowPin("") },
        "C86428");

    // EqualObjects — 比较两个对象引用是否相同（按 ID）
    reg("Object.Equal", "Equal Objects", "Object",
        { MakePin("A", PinDataType::Object),
          MakePin("B", PinDataType::Object) },
        { MakePin("Result", PinDataType::Boolean) },
        "C86428", "Simple");

    // EventBus.Emit — 广播一个具名事件（携带可选 payload）
    reg("EventBusEmit", "EventBus: Emit", "Misc/EventBus",
        { MakeFlowPin(""), MakePin("Event", PinDataType::String),
          MakePin("Payload", PinDataType::Any) },
        { MakeFlowPin("") },
        "F5A623");

    // EventBus.Subscribe — 监听具名事件（每次触发时激活 On Event 输出）
    reg("EventBusSubscribe", "EventBus: Subscribe", "Misc/EventBus",
        { MakeFlowPin("Enable"), MakeFlowPin("Disable"),
          MakePin("Event", PinDataType::String) },
        { MakeFlowPin("On Event"), MakePin("Payload", PinDataType::Any) },
        "F5A623");

    // EventBus.Clear — 移除某个事件的全部监听者
    reg("EventBusClear", "EventBus: Clear", "Misc/EventBus",
        { MakeFlowPin(""), MakePin("Event", PinDataType::String) },
        { MakeFlowPin("") },
        "F5A623");
}

// ============================================================================
// 入口函数
// ============================================================================

// Conversion 类型转换节点
static void RegisterNodeDefs_Conversion(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    reg("IntToFloat", "Int To Float", "Conversion",
        { MakePin("Value", PinDataType::Integer) },
        { MakePin("Result", PinDataType::Float) },
        "80C3F8", "Simple");
    

    reg("FloatToInt", "Float To Int", "Conversion",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::Integer) },
        "80C3F8", "Simple");
    

    reg("IntToString", "Int To String", "Conversion",
        { MakePin("Value", PinDataType::Integer) },
        { MakePin("Result", PinDataType::String) },
        "C880F8", "Simple");
    

    reg("FloatToString", "Float To String", "Conversion",
        { MakePin("Value", PinDataType::Float) },
        { MakePin("Result", PinDataType::String) },
        "C880F8", "Simple");
    

    reg("StringToInt", "String To Int", "Conversion",
        { MakePin("Value", PinDataType::String) },
        { MakePin("Result", PinDataType::Integer) },
        "F8A030", "Simple");
    

    reg("StringToFloat", "String To Float", "Conversion",
        { MakePin("Value", PinDataType::String) },
        { MakePin("Result", PinDataType::Float) },
        "F8A030", "Simple");
    

    reg("BoolToInt", "Bool To Int", "Conversion",
        { MakePin("Value", PinDataType::Boolean) },
        { MakePin("Result", PinDataType::Integer) },
        "", "Simple");
    

    reg("IntToBool", "Int To Bool", "Conversion",
        { MakePin("Value", PinDataType::Integer) },
        { MakePin("Result", PinDataType::Boolean) },
        "", "Simple");
    

    reg("ToString", "To String", "Conversion",
        { MakePin("Value", PinDataType::Any) },
        { MakePin("Result", PinDataType::String) },
        "C880F8", "Simple");
    
}

// Event 节点
static void RegisterNodeDefs_Event(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    reg("OnBeginPlay", "On Begin Play", "Event",
        {},
        { MakeFlowPin("") },
        "FF6060");
    

    reg("OnTick", "On Tick", "Event",
        {},
        { MakeFlowPin(""), MakePin("DeltaTime", PinDataType::Float) },
        "FF6060");
    

    reg("CustomEventNode", "Custom Event", "Event",
        {},
        { MakeFlowPin(""), MakePin("EventName", PinDataType::String) },
        "FF8040");

    // FireEvent：在运行时通过名字触发一个 Custom Event（相当于 DispatchEvent）
    // 用于在异步回调里触发下一轮事件，避免在 exec 图里形成 cycle。
    reg("FireEvent", "Fire Event", "Event",
        {
            MakeFlowPin(""),
            MakePin("EventName", PinDataType::String),
        },
        {
            MakeFlowPin(""),
        },
        "FF8040");
    
}

static void RegisterNodeDefs_Function(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    reg("Function.Entry", "Function Entry", "Function",
        {},
        { MakeFlowPin("") },
        "60C060");

    reg("Function.Return", "Function Return", "Function",
        { MakeFlowPin("") },
        {},
        "60C060");

    reg("Function.Call", "Call Function", "Function",
        { MakeFlowPin(""), MakePin("FunctionId", PinDataType::String) },
        { MakeFlowPin("") },
        "40A0FF");

    reg("Function.CallLibrary", "Call Library Function", "Function",
        { MakeFlowPin(""), MakePin("LibraryPath", PinDataType::String), MakePin("FunctionId", PinDataType::String) },
        { MakeFlowPin("") },
        "40A0FF");
}

static void RegisterNodeDefs_EventBus(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    reg("Event.Fire", "Fire Event", "Event",
        { MakeFlowPin(""), MakePin("EventName", PinDataType::String), MakePin("Payload", PinDataType::Any) },
        { MakeFlowPin("") },
        "FF6040");

    reg("Event.Subscribe", "Bind Event", "Event",
        { MakeFlowPin(""), MakePin("EventName", PinDataType::String) },
        { MakeFlowPin(""), MakePin("SubscriptionId", PinDataType::Integer) },
        "FF6040");

    reg("Event.Unsubscribe", "Unbind Event", "Event",
        { MakeFlowPin(""), MakePin("SubscriptionId", PinDataType::Integer) },
        { MakeFlowPin("") },
        "FF6040");

    reg("Event.OnEvent", "On Event", "Event",
        {},
        { MakeFlowPin(""), MakePin("EventName", PinDataType::String), MakePin("Payload", PinDataType::Any) },
        "FF8060");
}

// ============================================================================
// File I/O 节点定义
// 颜色：4CAF50（绿色，区别于 Network 蓝 / AI 紫）
// ============================================================================
// ============================================================================
// Retry 节点定义
// ============================================================================
static void RegisterNodeDefs_Retry(INodeRegistry& registry)
{
    // MakePin / MakeFlowPin 使用文件顶部的自由函数
    auto MakePinDef = [](const char* n, PinDataType t, Variant dv) {
        PinDefinition p = MakePin(n, t);
        p.defaultValue = dv;
        return p;
    };

    // Retry.Backoff — 同步重试循环，支持指数退避（退避为元数据，实际 Delay 需配合 Delay 节点）
    {
        NodeDefinition d;
        d.id       = "Retry.Backoff";
        d.name     = "Retry Backoff";
        d.category = "Flow/Retry";
        d.color    = "E67E22";  // 橙色

        d.inputPins = {
            MakeFlowPin(""),
            MakePinDef("MaxRetries",        PinDataType::Integer, Variant((int64_t)3)),
            MakePinDef("InitialDelayMs",    PinDataType::Float,   Variant(500.0)),
            MakePinDef("BackoffMultiplier", PinDataType::Float,   Variant(2.0)),
            MakePinDef("MaxDelayMs",        PinDataType::Float,   Variant(10000.0)),
        };
        d.outputPins = {
            MakeFlowPin("onTry"),
            MakePin("AttemptIndex", PinDataType::Integer),
            MakePin("DelayMs",      PinDataType::Float),
            MakeFlowPin("onSuccess"),
            MakeFlowPin("onExceeded"),
        };
        d.description =
            "Retry loop with exponential backoff. "
            "Each attempt activates onTry. Set blueprint variable "
            "'__retry_succeeded'=true inside the onTry body to exit early "
            "via onSuccess. All retries exhausted → onExceeded.";
        registry.registerNode(d);
    }
}

static void RegisterNodeDefs_File(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    const char* kColor = "4CAF50";  // 绿色

    // ── File.ReadText ──────────────────────────────────────────────────
    reg("File.ReadText", "File Read Text", "File",
        {
            MakeFlowPin(""),
            MakePin("Path", PinDataType::String),
        },
        {
            MakeFlowPin("onSuccess"),
            MakeFlowPin("onError"),
            MakePin("Content",      PinDataType::String),
            MakePin("ErrorMessage", PinDataType::String),
        },
        kColor);

    // ── File.WriteText ─────────────────────────────────────────────────
    reg("File.WriteText", "File Write Text", "File",
        {
            MakeFlowPin(""),
            MakePin("Path",    PinDataType::String),
            MakePin("Content", PinDataType::String),
            MakePin("Append",  PinDataType::Boolean),   // false=覆盖, true=追加
        },
        {
            MakeFlowPin("onSuccess"),
            MakeFlowPin("onError"),
            MakePin("ErrorMessage", PinDataType::String),
        },
        kColor);

    // ── File.AppendText ────────────────────────────────────────────────
    reg("File.AppendText", "File Append Text", "File",
        {
            MakeFlowPin(""),
            MakePin("Path",    PinDataType::String),
            MakePin("Content", PinDataType::String),
        },
        {
            MakeFlowPin("onSuccess"),
            MakeFlowPin("onError"),
            MakePin("ErrorMessage", PinDataType::String),
        },
        kColor);

    // ── File.Exists (纯数据) ───────────────────────────────────────────
    reg("File.Exists", "File Exists", "File",
        { MakePin("Path", PinDataType::String) },
        { MakePin("Exists", PinDataType::Boolean) },
        kColor);

    // ── File.Delete ────────────────────────────────────────────────────
    reg("File.Delete", "File Delete", "File",
        {
            MakeFlowPin(""),
            MakePin("Path",           PinDataType::String),
            MakePin("IgnoreNotFound", PinDataType::Boolean),
        },
        {
            MakeFlowPin("onSuccess"),
            MakeFlowPin("onError"),
            MakePin("ErrorMessage", PinDataType::String),
        },
        kColor);

    // ── File.ListDir ───────────────────────────────────────────────────
    reg("File.ListDir", "File List Dir", "File",
        {
            MakeFlowPin(""),
            MakePin("Path",    PinDataType::String),
            MakePin("Pattern", PinDataType::String),  // 子串过滤（空=全部）
        },
        {
            MakeFlowPin("onSuccess"),
            MakeFlowPin("onError"),
            MakePin("Files",        PinDataType::String),   // JSON 数组字符串
            MakePin("ErrorMessage", PinDataType::String),
        },
        kColor);

    // ── File.MakeDir ───────────────────────────────────────────────────
    reg("File.MakeDir", "File Make Dir", "File",
        {
            MakeFlowPin(""),
            MakePin("Path", PinDataType::String),
        },
        {
            MakeFlowPin("onSuccess"),
            MakeFlowPin("onError"),
            MakePin("ErrorMessage", PinDataType::String),
        },
        kColor);

    // ── File.GetBaseName (纯数据) ──────────────────────────────────────
    reg("File.GetBaseName", "File Get Base Name", "File",
        { MakePin("Path", PinDataType::String) },
        { MakePin("BaseName", PinDataType::String) },
        kColor);

    // ── File.GetDirName (纯数据) ───────────────────────────────────────
    reg("File.GetDirName", "File Get Dir Name", "File",
        { MakePin("Path", PinDataType::String) },
        { MakePin("DirName", PinDataType::String) },
        kColor);

    // ── File.JoinPath (纯数据) ─────────────────────────────────────────
    reg("File.JoinPath", "File Join Path", "File",
        {
            MakePin("Base", PinDataType::String),
            MakePin("Part", PinDataType::String),
        },
        { MakePin("Result", PinDataType::String) },
        kColor);

    // ── File.GetSize (纯数据) ──────────────────────────────────────────
    reg("File.GetSize", "File Get Size", "File",
        { MakePin("Path", PinDataType::String) },
        { MakePin("Size", PinDataType::Integer) },
        kColor);
}

static void RegisterNodeDefs_Network(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    // HTTP.Request
    reg("HTTP.Request", "HTTP Request", "Network",
        {
            MakeFlowPin(""),
            MakePin("URL",            PinDataType::String),
            MakePin("Method",         PinDataType::String),  // GET / POST / PUT / DELETE
            MakePin("Body",           PinDataType::String),
            MakePin("Headers",        PinDataType::String),  // JSON 格式 {"Key":"Value"}
            MakePin("TimeoutSeconds", PinDataType::Integer),
        },
        {
            MakeFlowPin("onSuccess"),
            MakeFlowPin("onError"),
            MakePin("StatusCode",   PinDataType::Integer),
            MakePin("ResponseBody", PinDataType::String),
            MakePin("ErrorMessage", PinDataType::String),
        },
        "2E86AB");  // 蓝色

    // JSON.GetPath — 支持 choices[0].message.content 路径
    reg("JSON.GetPath", "JSON Get Path", "Network",
        {
            MakePin("JSON", PinDataType::String),
            MakePin("Path", PinDataType::String),   // e.g. "choices[0].message.content"
        },
        {
            MakePin("Value", PinDataType::String),
            MakePin("Found", PinDataType::Boolean),
        },
        "2E86AB");
}

// ============================================================================
// AI / LLM 节点定义
// 颜色：6A0572（紫色，区别于网络蓝）
// ============================================================================
static void RegisterNodeDefs_AI(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    { RegisterNodeDef(registry, id, name, category, std::move(inputs), std::move(outputs), color, edType); };

    // JSON.Build — 从 Keys/Values 数组构建 JSON 对象字符串
    reg("JSON.Build", "JSON Build", "AI/JSON",
        {
            MakePin("Keys",   PinDataType::Array),   // Array<String>
            MakePin("Values", PinDataType::Array),   // Array<Any>
        },
        {
            MakePin("JSON", PinDataType::String),
        },
        "6A0572", "Simple");

    // JSON.SetPath — 向 JSON 对象设置嵌套路径
    reg("JSON.SetPath", "JSON Set Path", "AI/JSON",
        {
            MakeFlowPin(""),
            MakePin("JSON",  PinDataType::String),
            MakePin("Path",  PinDataType::String),   // e.g. "choices[0].message.content"
            MakePin("Value", PinDataType::Any),
        },
        {
            MakeFlowPin(""),
            MakePin("JSON",  PinDataType::String),
        },
        "6A0572");

    // JSON.ArrayPush — 向 JSON 数组追加元素
    reg("JSON.ArrayPush", "JSON Array Push", "AI/JSON",
        {
            MakeFlowPin(""),
            MakePin("JSON",    PinDataType::String),
            MakePin("Element", PinDataType::Any),
        },
        {
            MakeFlowPin(""),
            MakePin("JSON",   PinDataType::String),
            MakePin("Length", PinDataType::Integer),
        },
        "6A0572");

    // JSON.MakeMessage — 构造 {"role":"...","content":"..."} 消息对象
    reg("JSON.MakeMessage", "Make Message", "AI/JSON",
        {
            MakePin("Role",       PinDataType::String),   // "user" | "assistant" | "system" | "tool"
            MakePin("Content",    PinDataType::String),
            MakePin("ToolCallId", PinDataType::String),   // optional; for role="tool" responses
        },
        {
            MakePin("Message", PinDataType::String),
        },
        "6A0572", "Simple");

    // String.Template — {{key}} 占位符替换
    reg("String.Template", "String Template", "AI/String",
        {
            MakePin("Template", PinDataType::String),
            MakePin("Keys",     PinDataType::Array),   // Array<String>
            MakePin("Values",   PinDataType::Array),   // Array<String>
        },
        {
            MakePin("Result", PinDataType::String),
        },
        "6A0572", "Simple");

    // LLM.Chat — OpenAI 兼容 Chat 调用（支持 tool_calls）
    reg("LLM.Chat", "LLM Chat", "AI/LLM",
        {
            MakeFlowPin(""),
            MakePin("BaseURL",      PinDataType::String),   // 默认 https://api.openai.com/v1
            MakePin("ApiKey",       PinDataType::String),
            MakePin("Model",        PinDataType::String),   // 默认 gpt-4o
            MakePin("Messages",     PinDataType::String),   // JSON 数组字符串
            MakePin("SystemPrompt", PinDataType::String),   // 可选，自动插到首条
            MakePin("MaxTokens",    PinDataType::Integer),  // 默认 1024
            MakePin("Temperature",  PinDataType::Float),    // 默认 0.7
            MakePin("Tools",        PinDataType::String),   // function schema JSON 数组，可选
        },
        {
            MakeFlowPin("onReply"),                          // finish_reason=stop
            MakeFlowPin("onToolCall"),                       // finish_reason=tool_calls
            MakeFlowPin("onError"),
            MakePin("Reply",         PinDataType::String),   // 文本回复
            MakePin("ToolCallsJSON", PinDataType::String),   // tool_calls 数组 JSON
            MakePin("FinishReason",  PinDataType::String),   // "stop"|"tool_calls"|"length"|...
            MakePin("FullResponse",  PinDataType::String),
            MakePin("ErrorMessage",  PinDataType::String),
        },
        "6A0572");

    // ── LLM.StreamChat ──────────────────────────────────────────────────────
    // 流式 OpenAI 兼容 Chat（SSE 批量聚合）
    reg("LLM.StreamChat", "LLM Stream Chat", "AI/LLM",
        {
            MakeFlowPin(""),
            MakePin("BaseURL",      PinDataType::String),
            MakePin("ApiKey",       PinDataType::String),
            MakePin("Model",        PinDataType::String),
            MakePin("Messages",     PinDataType::String),
            MakePin("SystemPrompt", PinDataType::String),
            MakePin("MaxTokens",    PinDataType::Integer),
            MakePin("Temperature",  PinDataType::Float),
            MakePin("Tools",        PinDataType::String),
        },
        {
            MakeFlowPin("onChunk"),                          // 每个 token 激活一次
            MakeFlowPin("onDone"),                           // 全部 chunk 完毕
            MakeFlowPin("onError"),
            MakePin("Token",         PinDataType::String),   // 当前 token（onChunk 时）
            MakePin("FullText",      PinDataType::String),   // 完整文本（onDone 时）
            MakePin("ErrorMessage",  PinDataType::String),
        },
        "6A0572");

    // ── JSON.ParseToolCall ──────────────────────────────────────────────────
    // 从 tool_calls[Index] 中提取 name 和 arguments
    //   in:  ToolCallsJSON(String) — LLM.Chat 输出的 ToolCallsJSON
    //        Index(Integer)        — 要取第几个（默认 0）
    //   out: Name(String)          — function name
    //        ArgumentsJSON(String) — arguments 对象 JSON 字符串
    //        ID(String)            — tool_call id（回传给 LLM 时需要）
    reg("JSON.ParseToolCall", "Parse Tool Call", "AI/JSON",
        {
            MakePin("ToolCallsJSON", PinDataType::String),
            MakePin("Index",         PinDataType::Integer),
        },
        {
            MakePin("Name",          PinDataType::String),
            MakePin("ArgumentsJSON", PinDataType::String),
            MakePin("ID",            PinDataType::String),
        },
        "2E86AB");

    // ── JSON.MakeToolResult ─────────────────────────────────────────────────
    // 构造 tool role 消息，把工具结果反馈给 LLM
    //   in:  ToolCallID(String), Content(String)
    //   out: JSON(String) — {"role":"tool","tool_call_id":"...","content":"..."}
    reg("JSON.MakeToolResult", "Make Tool Result", "AI/JSON",
        {
            MakePin("ToolCallID", PinDataType::String),
            MakePin("Content",    PinDataType::String),
        },
        {
            MakePin("JSON", PinDataType::String),
        },
        "2E86AB");

    // ── JSON.ToolCallCount ──────────────────────────────────────────────────
    // 返回 tool_calls 数组长度（用于并行工具调用循环）
    reg("JSON.ToolCallCount", "Tool Call Count", "AI/JSON",
        { MakePin("ToolCallsJSON", PinDataType::String), },
        { MakePin("Count",         PinDataType::Integer), },
        "2E86AB");

    // 从文件加载对话历史 JSON 数组
    // WebGL：直接 onNew + 空数组
    reg("Memory.LoadHistory", "Memory Load History", "AI/Memory",
        {
            MakeFlowPin("In"),
            MakePin("Path",        PinDataType::String),   // 历史文件路径
            MakePin("MaxMessages", PinDataType::Integer),  // 0=不限，截断最旧的
        },
        {
            MakeFlowPin("onSuccess"),                      // 文件存在且解析成功
            MakeFlowPin("onNew"),                          // 文件不存在（首次启动）
            MakeFlowPin("onError"),
            MakePin("Messages",     PinDataType::String),  // JSON 数组字符串
            MakePin("Count",        PinDataType::Integer),
            MakePin("ErrorMessage", PinDataType::String),
        },
        "6A0572");

    // ── Memory.SaveHistory ──────────────────────────────────────────────────
    // 把 messages 数组写回文件（含可选截断）
    // WebGL：no-op，直接 onSuccess
    reg("Memory.SaveHistory", "Memory Save History", "AI/Memory",
        {
            MakeFlowPin("In"),
            MakePin("Path",        PinDataType::String),   // 历史文件路径
            MakePin("Messages",    PinDataType::String),   // JSON 数组字符串
            MakePin("MaxMessages", PinDataType::Integer),  // 0=不限，写入前截断
        },
        {
            MakeFlowPin("onSuccess"),
            MakeFlowPin("onError"),
            MakePin("ErrorMessage", PinDataType::String),
        },
        "6A0572");

    // ── Tool.ForEach ────────────────────────────────────────────────────────
    reg("Tool.ForEach", "Tool For Each", "AI/Tool",
        {
            MakeFlowPin(""),
            MakePin("ToolCallsJSON", PinDataType::String),
        },
        {
            MakeFlowPin("onTool"),     // 每个 tool call 触发一次
            MakeFlowPin("onDone"),     // 全部遍历完毕
            MakePin("ToolName",   PinDataType::String),
            MakePin("Arguments",  PinDataType::String),
            MakePin("ToolCallId", PinDataType::String),
            MakePin("Index",      PinDataType::Integer),
        },
        "6A0572");

    // ── Tool.Match ──────────────────────────────────────────────────────────
    reg("Tool.Match", "Tool Match", "AI/Tool",
        {
            MakeFlowPin(""),
            MakePin("ToolName", PinDataType::String),
            MakePin("Case0",    PinDataType::String),
            MakePin("Case1",    PinDataType::String),
            MakePin("Case2",    PinDataType::String),
            MakePin("Case3",    PinDataType::String),
            MakePin("Case4",    PinDataType::String),
            MakePin("Case5",    PinDataType::String),
            MakePin("Case6",    PinDataType::String),
            MakePin("Case7",    PinDataType::String),
        },
        {
            MakeFlowPin("Match0"),
            MakeFlowPin("Match1"),
            MakeFlowPin("Match2"),
            MakeFlowPin("Match3"),
            MakeFlowPin("Match4"),
            MakeFlowPin("Match5"),
            MakeFlowPin("Match6"),
            MakeFlowPin("Match7"),
            MakeFlowPin("Default"),
            MakePin("MatchedIndex", PinDataType::Integer),
        },
        "6A0572");

    // ── JSON.Extract ────────────────────────────────────────────────────────
    reg("JSON.Extract", "JSON Extract", "AI/JSON",
        {
            MakePin("Text", PinDataType::String),
        },
        {
            MakePin("JSON",  PinDataType::String),
            MakePin("Found", PinDataType::Boolean),
        },
        "6A0572", "Simple");

    // ── JSON.Validate ───────────────────────────────────────────────────────
    reg("JSON.Validate", "JSON Validate", "AI/JSON",
        {
            MakeFlowPin(""),
            MakePin("JSON",         PinDataType::String),
            MakePin("RequiredKeys", PinDataType::String),
        },
        {
            MakeFlowPin("onValid"),
            MakeFlowPin("onInvalid"),
            MakePin("IsValid",      PinDataType::Boolean),
            MakePin("ErrorMessage", PinDataType::String),
        },
        "6A0572");

}

void RegisterBuiltinNodeDefinitions(INodeRegistry& registry)
{
    // --- 注册分类 ---
    auto addCat = [&registry](const char* id, const char* name) {
        NodeCategory cat;
        cat.id = id;
        cat.name = name;
        registry.registerCategory(cat);
    };
    addCat("Flow",       "Flow Control");
    addCat("Action",     "Actions");
    addCat("Math",       "Math");
    addCat("Debug",      "Debug");
    addCat("Time",       "Time");
    addCat("Data",       "Data");
    addCat("Tree",       "Behavior Tree");
    addCat("Houdini",    "Houdini");
    addCat("Misc",       "Misc");
    addCat("Conversion", "Conversion");
    addCat("Event",      "Events");
    addCat("Function",   "Functions");
    addCat("Network",    "Network");
    addCat("AI",         "AI / LLM");
    addCat("AI/JSON",    "AI / JSON");
    addCat("AI/LLM",     "AI / LLM");
    addCat("AI/String",  "AI / String");
    addCat("AI/Memory",  "AI / Memory");
    addCat("File",       "File I/O");

    // --- 注册各分类的节点定义 ---
    RegisterNodeDefs_Flow(registry);
    RegisterNodeDefs_Action(registry);
    RegisterNodeDefs_Math(registry);
    RegisterNodeDefs_Debug(registry);
    RegisterNodeDefs_String(registry);
    RegisterNodeDefs_Array(registry);
    RegisterNodeDefs_Map(registry);
    RegisterNodeDefs_Time(registry);
    RegisterNodeDefs_Data(registry);
    RegisterNodeDefs_Tree(registry);
    RegisterNodeDefs_Houdini(registry);
    RegisterNodeDefs_Misc(registry);
    RegisterNodeDefs_Conversion(registry);
    RegisterNodeDefs_Event(registry);
    RegisterNodeDefs_Function(registry);
    RegisterNodeDefs_EventBus(registry);
    RegisterNodeDefs_Network(registry);
    RegisterNodeDefs_AI(registry);
    RegisterNodeDefs_File(registry);
    RegisterNodeDefs_Retry(registry);
}

} // namespace Runtime
} // namespace NodeEditor
