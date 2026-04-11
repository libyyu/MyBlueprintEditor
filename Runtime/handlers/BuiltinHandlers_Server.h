#pragma once
#include <unordered_map>
#include <string>
#include "../BlueprintRunner.h"

namespace NodeEditor { namespace Runtime {
    void RegisterHandlers_Server(
        std::unordered_map<std::string, NodeHandler>& handlers,
        BlueprintRunner& runner);
} }
