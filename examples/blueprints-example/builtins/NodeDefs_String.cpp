// NodeDefs_String.cpp -- Misc/String 节点定义注册
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterNodeDefs_String()
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

    reg("MakeString", "Make String", "Misc/String",
        { MakePin("Value", RTPinDataType::String) },
        { MakePin("String", RTPinDataType::String) },
        "", "Simple");

    reg("AppendString", "Append String", "Misc/String",
        { MakePin("A", RTPinDataType::String), MakePin("B", RTPinDataType::String) },
        { MakePin("Result", RTPinDataType::String) },
        "", "Simple");
    {
        // Mark as supporting dynamic String input pins (like UE's Append node)
        auto* d = const_cast<RTNodeDef*>(m_NodeRegistry.getNodeDefinition("AppendString"));
        if (d) d->customProperties["dynamicInputs"] = "String";
    }

    reg("StringLength", "String Length", "Misc/String",
        { MakePin("String", RTPinDataType::String) },
        { MakePin("Length", RTPinDataType::Integer) },
        "", "Simple");

    // ==================================================================
    // String 比较节点
    // ==================================================================

    reg("StringEquals", "String Equals", "Misc/String",
        { MakePin("A", RTPinDataType::String), MakePin("B", RTPinDataType::String) },
        { MakePin("Result", RTPinDataType::Boolean) },
        "", "Simple");

    reg("StringEqualsIgnoreCase", "String Equals (Ignore Case)", "Misc/String",
        { MakePin("A", RTPinDataType::String), MakePin("B", RTPinDataType::String) },
        { MakePin("Result", RTPinDataType::Boolean) },
        "", "Simple");

    reg("StringContains", "String Contains", "Misc/String",
        { MakePin("String", RTPinDataType::String), MakePin("Substring", RTPinDataType::String) },
        { MakePin("Result", RTPinDataType::Boolean) },
        "", "Simple");

    reg("StringStartsWith", "String Starts With", "Misc/String",
        { MakePin("String", RTPinDataType::String), MakePin("Prefix", RTPinDataType::String) },
        { MakePin("Result", RTPinDataType::Boolean) },
        "", "Simple");

    reg("StringEndsWith", "String Ends With", "Misc/String",
        { MakePin("String", RTPinDataType::String), MakePin("Suffix", RTPinDataType::String) },
        { MakePin("Result", RTPinDataType::Boolean) },
        "", "Simple");

    // ==================================================================
    // String 操作节点
    // ==================================================================

    reg("StringReplace", "String Replace", "Misc/String",
        { MakePin("String", RTPinDataType::String), MakePin("From", RTPinDataType::String),
          MakePin("To", RTPinDataType::String) },
        { MakePin("Result", RTPinDataType::String) },
        "", "Simple");

    reg("StringToUpper", "String To Upper", "Misc/String",
        { MakePin("String", RTPinDataType::String) },
        { MakePin("Result", RTPinDataType::String) },
        "", "Simple");

    reg("StringToLower", "String To Lower", "Misc/String",
        { MakePin("String", RTPinDataType::String) },
        { MakePin("Result", RTPinDataType::String) },
        "", "Simple");

    reg("StringSubstring", "String Substring", "Misc/String",
        { MakePin("String", RTPinDataType::String), MakePin("Start", RTPinDataType::Integer),
          MakePin("Count", RTPinDataType::Integer) },
        { MakePin("Result", RTPinDataType::String) },
        "", "Simple");

    reg("StringFind", "String Find", "Misc/String",
        { MakePin("String", RTPinDataType::String), MakePin("Substring", RTPinDataType::String) },
        { MakePin("Index", RTPinDataType::Integer), MakePin("Found", RTPinDataType::Boolean) },
        "", "Simple");

    reg("StringSplit", "String Split", "Misc/String",
        { MakePin("String", RTPinDataType::String), MakePin("Delimiter", RTPinDataType::String) },
        { MakePin("Array", RTPinDataType::Array), MakePin("Count", RTPinDataType::Integer) },
        "", "Simple");

    reg("StringTrimmed", "String Trimmed", "Misc/String",
        { MakePin("String", RTPinDataType::String) },
        { MakePin("Result", RTPinDataType::String) },
        "", "Simple");
}
