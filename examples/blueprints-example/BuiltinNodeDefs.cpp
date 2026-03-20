// BuiltinNodeDefs.cpp -- 内置节点定义注册
#include "BlueprintEditor.h"

void BlueprintEditor::RegisterBuiltinNodeDefinitions()
{
    // --- Categories ---
    auto addCat = [this](const char* id, const char* name) {
        RTNodeCategory cat;
        cat.id = id;
        cat.name = name;
        m_NodeRegistry.registerCategory(cat);
    };
    addCat("Flow",     "Flow Control");
    addCat("Action",   "Actions");
    addCat("Math",     "Math");
    addCat("Debug",    "Debug");
    addCat("Tree",     "Behavior Tree");
    addCat("Houdini",  "Houdini");
    addCat("Misc",     "Misc");

    // Helper lambda to register a definition quickly
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

    // --- Flow Control ---
    reg("Branch", "Branch", "Flow",
        { MakeFlowPin(""), MakePin("Condition", RTPinDataType::Boolean) },
        { MakeFlowPin("True"), MakeFlowPin("False") });

    reg("DoN", "Do N", "Flow",
        { MakeFlowPin("Enter"), MakePin("N", RTPinDataType::Integer), MakeFlowPin("Reset") },
        { MakeFlowPin("Exit"), MakePin("Counter", RTPinDataType::Integer) });

    reg("ExecuteBlueprint", "Execute Blueprint", "Flow",
        { MakeFlowPin(""), MakePin("File", RTPinDataType::String) },
        { MakeFlowPin("Done"), MakePin("Success", RTPinDataType::Boolean), MakePin("Output", RTPinDataType::String) },
        "FFA040");

    reg("Sequence", "Sequence", "Tree",
        { MakeFlowPin("") },
        { MakeFlowPin("") },
        "", "Tree");

    // --- Actions ---
    reg("InputActionFire", "InputAction Fire", "Action",
        {},
        { MakePin("", RTPinDataType::Custom, false), MakeFlowPin("Pressed"), MakeFlowPin("Released") },
        "FF8080");
    // Mark first output pin as Delegate type via customProperty
    {
        auto* d = const_cast<RTNodeDef*>(m_NodeRegistry.getNodeDefinition("InputActionFire"));
        if (d && !d->outputPins.empty())
            d->outputPins[0].customProperties["pinType"] = "Delegate";
    }

    reg("OutputAction", "OutputAction", "Action",
        { MakePin("Sample", RTPinDataType::Float), MakePin("Event", RTPinDataType::Custom, false) },
        { MakePin("Condition", RTPinDataType::Boolean) });
    {
        auto* d = const_cast<RTNodeDef*>(m_NodeRegistry.getNodeDefinition("OutputAction"));
        if (d && d->inputPins.size() >= 2)
            d->inputPins[1].customProperties["pinType"] = "Delegate";
    }

    reg("SetTimer", "Set Timer", "Action",
        { MakeFlowPin(""), MakePin("Object", RTPinDataType::Object),
          MakePin("Function Name", RTPinDataType::Custom, false),
          MakePin("Time", RTPinDataType::Float), MakePin("Looping", RTPinDataType::Boolean) },
        { MakeFlowPin("") },
        "80C3F8");
    {
        auto* d = const_cast<RTNodeDef*>(m_NodeRegistry.getNodeDefinition("SetTimer"));
        if (d && d->inputPins.size() >= 3)
            d->inputPins[2].customProperties["pinType"] = "Function";
    }

    reg("TraceByChannel", "Single Line Trace by Channel", "Action",
        { MakeFlowPin(""), MakeFlowPin("Start"), MakePin("End", RTPinDataType::Integer),
          MakePin("Trace Channel", RTPinDataType::Float), MakePin("Trace Complex", RTPinDataType::Boolean),
          MakePin("Actors to Ignore", RTPinDataType::Integer), MakePin("Draw Debug Type", RTPinDataType::Boolean),
          MakePin("Ignore Self", RTPinDataType::Boolean) },
        { MakeFlowPin(""), MakePin("Out Hit", RTPinDataType::Float), MakePin("Return Value", RTPinDataType::Boolean) },
        "FF8040");

    reg("PrintString", "Print String", "Debug",
        { MakeFlowPin(""), MakePin("In String", RTPinDataType::String) },
        { MakeFlowPin("") });

    // --- Math / Simple Nodes ---
    reg("Less", "<", "Math",
        { MakePin("", RTPinDataType::Float), MakePin("", RTPinDataType::Float) },
        { MakePin("", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Weird", "o.O", "Math",
        { MakePin("", RTPinDataType::Float) },
        { MakePin("", RTPinDataType::Float), MakePin("", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Add", "+", "Math",
        { MakePin("A", RTPinDataType::Float), MakePin("B", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Subtract", "-", "Math",
        { MakePin("A", RTPinDataType::Float), MakePin("B", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Multiply", "*", "Math",
        { MakePin("A", RTPinDataType::Float), MakePin("B", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Divide", "/", "Math",
        { MakePin("A", RTPinDataType::Float), MakePin("B", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Greater", ">", "Math",
        { MakePin("", RTPinDataType::Float), MakePin("", RTPinDataType::Float) },
        { MakePin("", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Equal", "==", "Math",
        { MakePin("A", RTPinDataType::Float), MakePin("B", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Boolean) },
        "80C3F8", "Simple");

    reg("And", "AND", "Math",
        { MakePin("A", RTPinDataType::Boolean), MakePin("B", RTPinDataType::Boolean) },
        { MakePin("Result", RTPinDataType::Boolean) },
        "80C3F8", "Simple");

    reg("Or", "OR", "Math",
        { MakePin("A", RTPinDataType::Boolean), MakePin("B", RTPinDataType::Boolean) },
        { MakePin("Result", RTPinDataType::Boolean) },
        "80C3F8", "Simple");

    reg("Not", "NOT", "Math",
        { MakePin("Value", RTPinDataType::Boolean) },
        { MakePin("Result", RTPinDataType::Boolean) },
        "80C3F8", "Simple");

    reg("IntToFloat", "Int to Float", "Math",
        { MakePin("Value", RTPinDataType::Integer) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("FloatToInt", "Float to Int", "Math",
        { MakePin("Value", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Integer) },
        "80C3F8", "Simple");

    reg("FloatToString", "Float to String", "Math",
        { MakePin("Value", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::String) },
        "80C3F8", "Simple");

    // --- Behavior Tree ---
    reg("MoveTo", "Move To", "Tree",
        { MakeFlowPin("") }, {},
        "", "Tree");

    reg("RandomWait", "Random Wait", "Tree",
        { MakeFlowPin("") }, {},
        "", "Tree");

    // --- Houdini ---
    reg("HoudiniTransform", "Transform", "Houdini",
        { MakeFlowPin("") },
        { MakeFlowPin("") },
        "", "Houdini");

    reg("HoudiniGroup", "Group", "Houdini",
        { MakeFlowPin(""), MakeFlowPin("") },
        { MakeFlowPin("") },
        "", "Houdini");

    // --- Misc ---
    reg("Message", "Message", "Misc",
        {},
        { MakePin("Message", RTPinDataType::String) },
        "80C3F8", "Simple");

    // Comment (special)
    {
        RTNodeDef d;
        d.id = "Comment";
        d.name = "Comment";
        d.category = "Misc";
        d.customProperties["editorType"] = "Comment";
        d.defaultSize = {300, 200};
        m_NodeRegistry.registerNode(d);
    }

    // --- New: additional useful nodes ---
    reg("ForLoop", "For Loop", "Flow",
        { MakeFlowPin(""), MakePin("First Index", RTPinDataType::Integer),
          MakePin("Last Index", RTPinDataType::Integer) },
        { MakeFlowPin("Loop Body"), MakePin("Index", RTPinDataType::Integer),
          MakeFlowPin("Completed") });

    reg("WhileLoop", "While Loop", "Flow",
        { MakeFlowPin(""), MakePin("Condition", RTPinDataType::Boolean) },
        { MakeFlowPin("Loop Body"), MakeFlowPin("Completed") });

    reg("Delay", "Delay", "Flow",
        { MakeFlowPin(""), MakePin("Duration", RTPinDataType::Float) },
        { MakeFlowPin("Completed") });

    reg("FlipFlop", "Flip Flop", "Flow",
        { MakeFlowPin("") },
        { MakeFlowPin("A"), MakeFlowPin("B"), MakePin("Is A", RTPinDataType::Boolean) });

    reg("Gate", "Gate", "Flow",
        { MakeFlowPin("Enter"), MakeFlowPin("Open"), MakeFlowPin("Close"),
          MakeFlowPin("Toggle") },
        { MakeFlowPin("Exit") });

    reg("MakeString", "Make String", "Debug",
        { MakePin("Value", RTPinDataType::String) },
        { MakePin("String", RTPinDataType::String) },
        "", "Simple");

    reg("AppendString", "Append String", "Debug",
        { MakePin("A", RTPinDataType::String), MakePin("B", RTPinDataType::String) },
        { MakePin("Result", RTPinDataType::String) },
        "", "Simple");

    reg("StringLength", "String Length", "Debug",
        { MakePin("String", RTPinDataType::String) },
        { MakePin("Length", RTPinDataType::Integer) },
        "", "Simple");

    reg("Clamp", "Clamp", "Math",
        { MakePin("Value", RTPinDataType::Float), MakePin("Min", RTPinDataType::Float),
          MakePin("Max", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Abs", "Abs", "Math",
        { MakePin("Value", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Min", "Min", "Math",
        { MakePin("A", RTPinDataType::Float), MakePin("B", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Max", "Max", "Math",
        { MakePin("A", RTPinDataType::Float), MakePin("B", RTPinDataType::Float) },
        { MakePin("Result", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("Random", "Random Float", "Math",
        {},
        { MakePin("Value", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("RandomInRange", "Random In Range", "Math",
        { MakePin("Min", RTPinDataType::Float), MakePin("B", RTPinDataType::Float) },
        { MakePin("Value", RTPinDataType::Float) },
        "80C3F8", "Simple");

    reg("GetVariable", "Get Variable", "Misc",
        { MakePin("Name", RTPinDataType::String) },
        { MakePin("Value", RTPinDataType::String) },
        "", "Simple");

    reg("SetVariable", "Set Variable", "Misc",
        { MakeFlowPin(""), MakePin("Name", RTPinDataType::String),
          MakePin("Value", RTPinDataType::String) },
        { MakeFlowPin("") });

    reg("Log", "Log", "Debug",
        { MakeFlowPin(""), MakePin("Message", RTPinDataType::String) },
        { MakeFlowPin("") });
}
