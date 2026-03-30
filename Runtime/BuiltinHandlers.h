// Runtime/BuiltinHandlers.h -- 内置节点处理器注册（独立于编辑器）
//
// 提供 RegisterBuiltinHandlers() 自由函数，
// 可在任意环境中向 BlueprintRunner 注册所有内置节点处理器，
// 无需依赖 BlueprintEditor 或 ImGui。
//
// 用法：
//   BlueprintRunner runner;
//   runner.LoadFromFile("blueprint.json");
//   NodeEditor::Runtime::RegisterBuiltinHandlers(runner, "base/path");
//   runner.Execute();
//

#pragma once
#include "BlueprintExport.h"

#include "BlueprintRunner.h"
#include <string>

namespace NodeEditor {
namespace Runtime {

// 向 runner 注册所有内置节点处理器
// 参数：
//   runner    - 蓝图运行器（处理器将注册到其中）
//   basePath  - 基准路径（用于解析 ExecuteBlueprint 等节点的相对路径）
//   handlers  - 可选输出，返回已注册的处理器映射表（供子蓝图继承）
BLUEPRINT_API void RegisterBuiltinHandlers(
    BlueprintRunner& runner,
    const std::string& basePath = "",
    std::unordered_map<std::string, NodeHandler>* handlers = nullptr);

} // namespace Runtime
} // namespace NodeEditor
