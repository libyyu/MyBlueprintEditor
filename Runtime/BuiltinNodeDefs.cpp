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

// ============================================================================
// 注册辅助 lambda 类型
// ============================================================================

static void RegisterNodeDefs_Flow(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    {
        NodeDefinition d;
        d.id = id;
        d.name = name;
        d.category = category;
        d.inputPins = std::move(inputs);
        d.outputPins = std::move(outputs);
        if (color[0]) d.color = color;
        if (edType[0]) d.customProperties["editorType"] = edType;
        registry.registerNode(d);
    };

    reg("Branch", "Branch", "Flow",
        { MakeFlowPin(""), MakePin("Condition", PinDataType::Boolean) },
        { MakeFlowPin("True"), MakeFlowPin("False") });

    reg("DoN", "Do N", "Flow",
        { MakeFlowPin("Enter"), MakePin("N", PinDataType::Integer), MakeFlowPin("Reset") },
        { MakeFlowPin("Exit"), MakePin("Counter", PinDataType::Integer) });

    // ExecuteBlueprint: Completed 引脚在同步模式下隐藏
    {
        NodeDefinition d;
        d.id = "ExecuteBlueprint";
        d.name = "Execute Blueprint";
        d.category = "Flow";
        d.inputPins = { MakeFlowPin(""), MakePin("File", PinDataType::String), MakePin("Sync", PinDataType::Boolean) };

        auto completedPin = MakeFlowPin("Completed");
        completedPin.customProperties["hiddenWhen"] = "Sync==true";

        d.outputPins = { MakeFlowPin("Done"), completedPin, MakePin("Success", PinDataType::Boolean), MakePin("Output", PinDataType::String) };
        d.color = "FFA040";
        registry.registerNode(d);
    }

    reg("ForLoop", "For Loop", "Flow",
        { MakeFlowPin(""), MakePin("First Index", PinDataType::Integer),
          MakePin("Last Index", PinDataType::Integer) },
        { MakeFlowPin("Loop Body"), MakePin("Index", PinDataType::Integer),
          MakeFlowPin("Completed") });

    reg("WhileLoop", "While Loop", "Flow",
        { MakeFlowPin(""), MakePin("Condition", PinDataType::Boolean) },
        { MakeFlowPin("Loop Body"), MakeFlowPin("Completed") });

    reg("Delay", "Delay", "Flow",
        { MakeFlowPin(""), MakePin("Duration", PinDataType::Float) },
        { MakeFlowPin("Exec"), MakeFlowPin("Completed"), MakePin("TimerHandle", PinDataType::Integer) });

    reg("FlipFlop", "Flip Flop", "Flow",
        { MakeFlowPin("") },
        { MakeFlowPin("A"), MakeFlowPin("B"), MakePin("Is A", PinDataType::Boolean) });

    reg("Gate", "Gate", "Flow",
        { MakeFlowPin("Enter"), MakeFlowPin("Open"), MakeFlowPin("Close"),
          MakeFlowPin("Toggle") },
        { MakeFlowPin("Exit") });
}

static void RegisterNodeDefs_Action(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    {
        NodeDefinition d;
        d.id = id;
        d.name = name;
        d.category = category;
        d.inputPins = std::move(inputs);
        d.outputPins = std::move(outputs);
        if (color[0]) d.color = color;
        if (edType[0]) d.customProperties["editorType"] = edType;
        registry.registerNode(d);
    };

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
    {
        NodeDefinition d;
        d.id = id;
        d.name = name;
        d.category = category;
        d.inputPins = std::move(inputs);
        d.outputPins = std::move(outputs);
        if (color[0]) d.color = color;
        if (edType[0]) d.customProperties["editorType"] = edType;
        registry.registerNode(d);
    };

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
    {
        NodeDefinition d;
        d.id = id;
        d.name = name;
        d.category = category;
        d.inputPins = std::move(inputs);
        d.outputPins = std::move(outputs);
        if (color[0]) d.color = color;
        if (edType[0]) d.customProperties["editorType"] = edType;
        registry.registerNode(d);
    };

    reg("PrintString", "Print String", "Debug",
        { MakeFlowPin(""), MakePin("In String", PinDataType::String) },
        { MakeFlowPin("") });

    reg("Log", "Log", "Debug",
        { MakeFlowPin(""), MakePin("Message", PinDataType::String) },
        { MakeFlowPin("") });
}

static void RegisterNodeDefs_String(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    {
        NodeDefinition d;
        d.id = id;
        d.name = name;
        d.category = category;
        d.inputPins = std::move(inputs);
        d.outputPins = std::move(outputs);
        if (color[0]) d.color = color;
        if (edType[0]) d.customProperties["editorType"] = edType;
        registry.registerNode(d);
    };

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
}

static void RegisterNodeDefs_Array(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    {
        NodeDefinition d;
        d.id = id;
        d.name = name;
        d.category = category;
        d.inputPins = std::move(inputs);
        d.outputPins = std::move(outputs);
        if (color[0]) d.color = color;
        if (edType[0]) d.customProperties["editorType"] = edType;
        registry.registerNode(d);
    };

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
}

static void RegisterNodeDefs_Tree(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    {
        NodeDefinition d;
        d.id = id;
        d.name = name;
        d.category = category;
        d.inputPins = std::move(inputs);
        d.outputPins = std::move(outputs);
        if (color[0]) d.color = color;
        if (edType[0]) d.customProperties["editorType"] = edType;
        registry.registerNode(d);
    };

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
    {
        NodeDefinition d;
        d.id = id;
        d.name = name;
        d.category = category;
        d.inputPins = std::move(inputs);
        d.outputPins = std::move(outputs);
        if (color[0]) d.color = color;
        if (edType[0]) d.customProperties["editorType"] = edType;
        registry.registerNode(d);
    };

    reg("HoudiniTransform", "Transform", "Houdini",
        { MakeFlowPin("") },
        { MakeFlowPin("") },
        "", "Houdini");

    reg("HoudiniGroup", "Group", "Houdini",
        { MakeFlowPin(""), MakeFlowPin("") },
        { MakeFlowPin("") },
        "", "Houdini");
}

static void RegisterNodeDefs_Misc(INodeRegistry& registry)
{
    auto reg = [&registry](const char* id, const char* name, const char* category,
                  std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
                  const char* color = "", const char* edType = "")
    {
        NodeDefinition d;
        d.id = id;
        d.name = name;
        d.category = category;
        d.inputPins = std::move(inputs);
        d.outputPins = std::move(outputs);
        if (color[0]) d.color = color;
        if (edType[0]) d.customProperties["editorType"] = edType;
        registry.registerNode(d);
    };

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
}

// ============================================================================
// 入口函数
// ============================================================================

void RegisterBuiltinNodeDefinitions(INodeRegistry& registry)
{
    // --- 注册分类 ---
    auto addCat = [&registry](const char* id, const char* name) {
        NodeCategory cat;
        cat.id = id;
        cat.name = name;
        registry.registerCategory(cat);
    };
    addCat("Flow",     "Flow Control");
    addCat("Action",   "Actions");
    addCat("Math",     "Math");
    addCat("Debug",    "Debug");
    addCat("Tree",     "Behavior Tree");
    addCat("Houdini",  "Houdini");
    addCat("Misc",     "Misc");

    // --- 注册各分类的节点定义 ---
    RegisterNodeDefs_Flow(registry);
    RegisterNodeDefs_Action(registry);
    RegisterNodeDefs_Math(registry);
    RegisterNodeDefs_Debug(registry);
    RegisterNodeDefs_String(registry);
    RegisterNodeDefs_Array(registry);
    RegisterNodeDefs_Tree(registry);
    RegisterNodeDefs_Houdini(registry);
    RegisterNodeDefs_Misc(registry);
}

} // namespace Runtime
} // namespace NodeEditor
