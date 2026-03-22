// NodeDefs_Debug.cpp -- Debug 节点定义注册
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterNodeDefs_Debug()
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

    reg("PrintString", "Print String", "Debug",
        { MakeFlowPin(""), MakePin("In String", RTPinDataType::String) },
        { MakeFlowPin("") });

    reg("MakeString", "Make String", "Debug",
        { MakePin("Value", RTPinDataType::String) },
        { MakePin("String", RTPinDataType::String) },
        "", "Simple");

    reg("AppendString", "Append String", "Debug",
        { MakePin("A", RTPinDataType::String), MakePin("B", RTPinDataType::String) },
        { MakePin("Result", RTPinDataType::String) },
        "", "Simple");
    {
        // Mark as supporting dynamic String input pins (like UE's Append node)
        auto* d = const_cast<RTNodeDef*>(m_NodeRegistry.getNodeDefinition("AppendString"));
        if (d) d->customProperties["dynamicInputs"] = "String";
    }

    reg("StringLength", "String Length", "Debug",
        { MakePin("String", RTPinDataType::String) },
        { MakePin("Length", RTPinDataType::Integer) },
        "", "Simple");

    reg("Log", "Log", "Debug",
        { MakeFlowPin(""), MakePin("Message", RTPinDataType::String) },
        { MakeFlowPin("") });
}
