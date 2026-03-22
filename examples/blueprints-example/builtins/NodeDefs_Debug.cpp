// NodeDefs_Debug.cpp -- Debug 节点定义注册（PrintString, Log）
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

    reg("Log", "Log", "Debug",
        { MakeFlowPin(""), MakePin("Message", RTPinDataType::String) },
        { MakeFlowPin("") });
}
