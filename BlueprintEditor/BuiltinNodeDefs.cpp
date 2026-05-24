// BuiltinNodeDefs.cpp -- 内置节点定义注册（入口）
//
// 委托给 Runtime 层的独立函数 RegisterBuiltinNodeDefinitions()，
// 避免代码重复。Editor 层和第三方程序共享同一份节点定义。
//
#include "BlueprintEditor.h"
#include "BuiltinNodeDefs.h"
#include "BpLogger.h"

void BlueprintEditor::RegisterBuiltinNodeDefinitions()
{
    // 委托给 Runtime 层的独立注册函数
    ::NodeEditor::Runtime::RegisterBuiltinNodeDefinitions(m_NodeRegistry);
}

void BlueprintEditor::SyncScriptedDefsToRegistry()
{
    // 将扩展脚本（如 Lua 后端）注册的节点定义同步到编辑器节点库
    // 无脚本后端时 GetScriptRegisteredNodeIds() 返回空集合，无副作用
    const auto& ids = m_extensionRunner.GetScriptRegisteredNodeIds();
    int synced = 0;
    for (const auto& id : ids)
    {
        const auto* def = m_extensionRunner.GetNodeDef(id);
        if (def)
        {
            m_NodeRegistry.registerNode(*def);
            ++synced;
        }
    }
    BpLogger::Get().Info("[SyncScriptedDefs] ids.size()=" + std::to_string(ids.size())
        + " synced=" + std::to_string(synced)
        + " m_NodeRegistry.total=" + std::to_string(m_NodeRegistry.getAllNodeDefinitions().size()));
    m_CachedDefCount = 0;  // 触发节点菜单缓存重建
}
