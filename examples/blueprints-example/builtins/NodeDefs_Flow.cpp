// NodeDefs_Flow.cpp -- Flow Control 节点定义注册
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterNodeDefs_Flow()
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
}
