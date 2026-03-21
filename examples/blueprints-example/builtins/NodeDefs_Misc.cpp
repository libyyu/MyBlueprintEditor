// NodeDefs_Misc.cpp -- Misc 节点定义注册
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterNodeDefs_Misc()
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

    reg("GetVariable", "Get Variable", "Misc",
        { MakePin("Name", RTPinDataType::String) },
        { MakePin("Value", RTPinDataType::String) },
        "", "Simple");

    reg("SetVariable", "Set Variable", "Misc",
        { MakeFlowPin(""), MakePin("Name", RTPinDataType::String),
          MakePin("Value", RTPinDataType::String) },
        { MakeFlowPin("") });
}
