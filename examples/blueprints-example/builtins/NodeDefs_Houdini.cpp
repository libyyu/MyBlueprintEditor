// NodeDefs_Houdini.cpp -- Houdini 节点定义注册
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterNodeDefs_Houdini()
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

    reg("HoudiniTransform", "Transform", "Houdini",
        { MakeFlowPin("") },
        { MakeFlowPin("") },
        "", "Houdini");

    reg("HoudiniGroup", "Group", "Houdini",
        { MakeFlowPin(""), MakeFlowPin("") },
        { MakeFlowPin("") },
        "", "Houdini");
}
