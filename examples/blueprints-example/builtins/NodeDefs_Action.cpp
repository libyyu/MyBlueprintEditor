// NodeDefs_Action.cpp -- Action 节点定义注册
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterNodeDefs_Action()
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
        { MakeFlowPin("Exec"), MakePin("TimerHandle", RTPinDataType::Integer) },
        "80C3F8");
    {
        auto* d = const_cast<RTNodeDef*>(m_NodeRegistry.getNodeDefinition("SetTimer"));
        if (d && d->inputPins.size() >= 3)
            d->inputPins[2].customProperties["pinType"] = "Function";
    }

    reg("RemoveTimer", "Remove Timer", "Action",
        { MakeFlowPin(""), MakePin("TimerHandle", RTPinDataType::Integer) },
        { MakeFlowPin("Exec"), MakePin("Success", RTPinDataType::Boolean) },
        "FF6060");

    reg("PauseTimer", "Pause Timer", "Action",
        { MakeFlowPin(""), MakePin("TimerHandle", RTPinDataType::Integer) },
        { MakeFlowPin("Exec"), MakePin("Success", RTPinDataType::Boolean) },
        "FFA040");

    reg("ResumeTimer", "Resume Timer", "Action",
        { MakeFlowPin(""), MakePin("TimerHandle", RTPinDataType::Integer) },
        { MakeFlowPin("Exec"), MakePin("Success", RTPinDataType::Boolean) },
        "40C080");

    reg("CustomEvent", "Custom Event", "Action",
        {},
        { MakePin("Event", RTPinDataType::Custom, false), MakeFlowPin("Exec") },
        "A080F8");
    {
        auto* d = const_cast<RTNodeDef*>(m_NodeRegistry.getNodeDefinition("CustomEvent"));
        if (d && !d->outputPins.empty())
            d->outputPins[0].customProperties["pinType"] = "Function";
    }

    reg("TraceByChannel", "Single Line Trace by Channel", "Action",
        { MakeFlowPin(""), MakeFlowPin("Start"), MakePin("End", RTPinDataType::Integer),
          MakePin("Trace Channel", RTPinDataType::Float), MakePin("Trace Complex", RTPinDataType::Boolean),
          MakePin("Actors to Ignore", RTPinDataType::Integer), MakePin("Draw Debug Type", RTPinDataType::Boolean),
          MakePin("Ignore Self", RTPinDataType::Boolean) },
        { MakeFlowPin(""), MakePin("Out Hit", RTPinDataType::Float), MakePin("Return Value", RTPinDataType::Boolean) },
        "FF8040");
}
