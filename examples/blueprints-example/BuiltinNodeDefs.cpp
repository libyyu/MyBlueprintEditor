// BuiltinNodeDefs.cpp -- 内置节点定义注册（入口）
//
// 委托给 Runtime 层的独立函数 RegisterBuiltinNodeDefinitions()，
// 避免代码重复。Editor 层和第三方程序共享同一份节点定义。
//
#include "BlueprintEditor.h"
#include "BuiltinNodeDefs.h"

void BlueprintEditor::RegisterBuiltinNodeDefinitions()
{
    // 委托给 Runtime 层的独立注册函数
    ::NodeEditor::Runtime::RegisterBuiltinNodeDefinitions(m_NodeRegistry);
}
