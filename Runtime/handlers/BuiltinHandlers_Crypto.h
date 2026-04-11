#pragma once
#include <unordered_map>
#include <string>
#include "../BlueprintRunner.h"

namespace NodeEditor { namespace Runtime {
    void RegisterHandlers_Crypto(
        std::unordered_map<std::string, NodeHandler>& handlers);
} }
