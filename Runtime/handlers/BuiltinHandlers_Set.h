#pragma once
#include "../BlueprintRunner.h"
#include <unordered_map>
#include <string>

namespace NodeEditor { namespace Runtime {
void RegisterHandlers_Set(std::unordered_map<std::string, NodeHandler>& handlers);
}}
