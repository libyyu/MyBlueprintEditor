// Runtime/handlers/BuiltinHandlers_GameExtra.h
// 游戏扩展节点：Random 随机系统、Easing 缓动函数、实用游戏工具
#pragma once
#include <unordered_map>
#include <string>
#include "../BlueprintRunner.h"

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_GameExtra(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner);

} // namespace Runtime
} // namespace NodeEditor
