// NodeDefs_Tree.cpp -- Behavior Tree 节点定义注册
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterNodeDefs_Tree()
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
