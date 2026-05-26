// Runtime/nodedefs/BuiltinNodeDefs_Internal.h
// 内部头文件：被各 nodedefs/BuiltinNodeDefs_XX.cpp 共用
//   - 共享辅助函数（MakePin / MakeFlowPin / MakePinEnum / RegisterNodeDef）
//   - 各分类的 RegisterNodeDefs_XX 前向声明（供 BuiltinNodeDefs.cpp 入口调用）
//
// 不对外暴露——仅用于 Runtime/nodedefs/*.cpp 与 Runtime/BuiltinNodeDefs.cpp 之间共享

#pragma once

#include "../NodeDefinition.h"
#include <string>
#include <utility>
#include <vector>

namespace NodeEditor {
namespace Runtime {
namespace nodedefs_internal {

// ----------------------------------------------------------------------------
// Pin 构造辅助
// ----------------------------------------------------------------------------

inline PinDefinition MakePin(const char* name, PinDataType dt, bool isExec = false)
{
    PinDefinition p;
    p.name = name;
    p.dataType = dt;
    p.isExec = isExec;
    return p;
}

inline PinDefinition MakeFlowPin(const char* name = "")
{
    return MakePin(name, PinDataType::Unknown, true);
}

// 枚举引脚辅助：String 引脚 + 编辑器渲染为 Combo 下拉
// values: 逗号分隔的枚举值，如 "GET,POST,PUT,DELETE,PATCH,HEAD"
// strict: true = 只能选，false = 选 + 可手输
// defaultVal: 可选默认值（默认取第一个选项）
inline PinDefinition MakePinEnum(const char* name,
                                 const char* values,
                                 const char* defaultVal = nullptr,
                                 bool strict = true)
{
    PinDefinition p;
    p.name = name;
    p.dataType = PinDataType::String;
    p.customProperties["enumValues"] = values;
    if (strict) p.customProperties["enumStrict"] = "true";
    std::string firstVal;
    if (defaultVal)
    {
        firstVal = defaultVal;
    }
    else
    {
        std::string v(values);
        auto comma = v.find(',');
        firstVal = (comma != std::string::npos) ? v.substr(0, comma) : v;
    }
    if (!firstVal.empty())
        p.defaultValue = Variant(firstVal);
    return p;
}

// 统一的节点定义注册辅助函数
inline void RegisterNodeDef(INodeRegistry& registry,
    const char* id, const char* name, const char* category,
    std::vector<PinDefinition> inputs, std::vector<PinDefinition> outputs,
    const char* color = "", const char* edType = "", const char* icon = "")
{
    NodeDefinition d;
    d.id = id;
    d.name = name;
    d.category = category;
    d.inputPins = std::move(inputs);
    d.outputPins = std::move(outputs);
    if (color[0]) d.color = color;
    if (edType[0]) d.customProperties["editorType"] = edType;
    if (icon[0])  d.icon = icon;
    registry.registerNode(d);
}

} // namespace nodedefs_internal

// ----------------------------------------------------------------------------
// 各分类前向声明（供 BuiltinNodeDefs.cpp 入口调用）
// ----------------------------------------------------------------------------

void RegisterNodeDefs_Flow(INodeRegistry& registry);
void RegisterNodeDefs_Action(INodeRegistry& registry);
void RegisterNodeDefs_Math(INodeRegistry& registry);
void RegisterNodeDefs_Debug(INodeRegistry& registry);
void RegisterNodeDefs_String(INodeRegistry& registry);
void RegisterNodeDefs_Array(INodeRegistry& registry);
void RegisterNodeDefs_Map(INodeRegistry& registry);
void RegisterNodeDefs_Time(INodeRegistry& registry);
void RegisterNodeDefs_Data(INodeRegistry& registry);
void RegisterNodeDefs_Tree(INodeRegistry& registry);
void RegisterNodeDefs_Houdini(INodeRegistry& registry);
void RegisterNodeDefs_Misc(INodeRegistry& registry);
void RegisterNodeDefs_Conversion(INodeRegistry& registry);
void RegisterNodeDefs_Event(INodeRegistry& registry);
void RegisterNodeDefs_Function(INodeRegistry& registry);
void RegisterNodeDefs_EventBus(INodeRegistry& registry);
void RegisterNodeDefs_Retry(INodeRegistry& registry);
void RegisterNodeDefs_File(INodeRegistry& registry);
void RegisterNodeDefs_Network(INodeRegistry& registry);
void RegisterNodeDefs_AI(INodeRegistry& registry);
void RegisterNodeDefs_Game(INodeRegistry& registry);
void RegisterNodeDefs_Save(INodeRegistry& registry);
void RegisterNodeDefs_GameMath(INodeRegistry& registry);
void RegisterNodeDefs_Socket(INodeRegistry& registry);
void RegisterNodeDefs_GameExtra(INodeRegistry& registry);

} // namespace Runtime
} // namespace NodeEditor
