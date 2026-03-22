// NodeDefs_Array.cpp -- Misc/Array 节点定义注册
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterNodeDefs_Array()
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

    reg("ArrayLength", "Array Length", "Misc/Array",
        { MakePin("Array", RTPinDataType::Array) },
        { MakePin("Length", RTPinDataType::Integer) },
        "", "Simple");

    reg("ArrayGet", "Array Get", "Misc/Array",
        { MakePin("Array", RTPinDataType::Array), MakePin("Index", RTPinDataType::Integer) },
        { MakePin("Element", RTPinDataType::Any), MakePin("Valid", RTPinDataType::Boolean) },
        "", "Simple");

    // ForEachLoop — 遍历数组的每一个元素（类似 UE 的 ForEachLoop）
    reg("ForEachLoop", "For Each Loop", "Misc/Array",
        { MakeFlowPin(""), MakePin("Array", RTPinDataType::Array) },
        { MakeFlowPin("Loop Body"), MakePin("Array Element", RTPinDataType::Any),
          MakePin("Array Index", RTPinDataType::Integer), MakeFlowPin("Completed") });
}
