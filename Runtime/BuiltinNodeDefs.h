// Runtime/BuiltinNodeDefs.h -- 内置节点定义注册（独立于编辑器）
//
// 提供 RegisterBuiltinNodeDefinitions() 自由函数，
// 可在任意环境中向 INodeRegistry 注册所有内置节点定义，
// 无需依赖 BlueprintEditor 或 ImGui。
//
// 用法：
//   DefaultNodeRegistry registry;
//   NodeEditor::Runtime::RegisterBuiltinNodeDefinitions(registry);
//

#pragma once
#include "BlueprintExport.h"

#include "NodeDefinition.h"

namespace NodeEditor {
namespace Runtime {

// 向注册表注册所有内置节点定义（分类 + 节点模板）
void RegisterBuiltinNodeDefinitions(INodeRegistry& registry);

} // namespace Runtime
} // namespace NodeEditor
