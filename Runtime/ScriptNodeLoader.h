// Runtime/ScriptNodeLoader.h -- 从 JSON 文件加载自定义节点定义
#pragma once

#include "BlueprintExport.h"
#include "NodeDefinition.h"
#include <string>

namespace NodeEditor {
namespace Runtime {

// 从指定 JSON 文件加载节点定义并注册到 registry。
// 若文件不存在，静默跳过。返回成功加载的节点数量。
BLUEPRINT_API int LoadCustomNodesFromFile(INodeRegistry& registry, const std::string& filePath);

} // namespace Runtime
} // namespace NodeEditor
