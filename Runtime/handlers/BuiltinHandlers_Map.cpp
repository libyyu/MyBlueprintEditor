// Runtime/BuiltinHandlers_Map.cpp -- Map 节点处理器
#include "BuiltinHandlers_Map.h"

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Map(std::unordered_map<std::string, NodeHandler>& handlers)
{
    // NodeDef id 是 "MapMake"，同时保留 "MakeMap" 兼容旧蓝图
    handlers["MapMake"] = handlers["MakeMap"] = [](ExecutionContext& ctx) {
        const auto* node = ctx.GetCurrentNode();
        std::unordered_map<std::string, Variant> result;
        if (node)
        {
            // 从输入引脚中按 Key/Value 配对收集
            std::vector<const PinInfo*> dataPins;
            for (const auto& pin : node->pins)
            {
                if (pin.kind == PinKind::Input && !pin.isExec)
                    dataPins.push_back(&pin);
            }
            // 每两个引脚为一组：Key, Value
            // Key 支持任意类型，通过 asString() 统一转为字符串键
            for (size_t i = 0; i + 1 < dataPins.size(); i += 2)
            {
                std::string key = ctx.GetInputValue(dataPins[i]->id).asString();
                Variant val = ctx.GetInputValue(dataPins[i + 1]->id);
                if (!key.empty())
                    result[key] = val;
            }
        }
        ctx.SetOutputValue("Map", Variant(std::move(result)));
        return true;
    };

    handlers["MapGet"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        auto key = ctx.GetInputValue("Key").asString();
        bool found = map.mapHasKey(key);
        if (found)
            ctx.SetOutputValue("Value", map.mapGet(key));
        else
            ctx.SetOutputValue("Value", Variant());
        ctx.SetOutputValue("Found", Variant(found));
        return true;
    };

    handlers["MapSet"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        auto key = ctx.GetInputValue("Key").asString();
        auto value = ctx.GetInputValue("Value");
        map.mapSet(key, value);
        ctx.SetOutputValue("Map", map);
        ctx.Log("  [MapSet] Set key \"" + key + "\" -> " + value.asString());
        return true;
    };

    handlers["MapRemove"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        auto key = ctx.GetInputValue("Key").asString();
        bool removed = map.mapRemove(key);
        ctx.SetOutputValue("Map", map);
        ctx.SetOutputValue("Removed", Variant(removed));
        ctx.Log("  [MapRemove] Key \"" + key + "\" " + (removed ? "removed" : "not found"));
        return true;
    };

    handlers["MapHasKey"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        auto key = ctx.GetInputValue("Key").asString();
        ctx.SetOutputValue("Result", Variant(map.mapHasKey(key)));
        return true;
    };

    // NodeDef 里有 "MapLength" 和 "MapSize" 两个，逻辑相同
    handlers["MapSize"] = handlers["MapLength"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        ctx.SetOutputValue("Size", Variant(static_cast<int64_t>(map.mapSize())));
        return true;
    };

    handlers["MapKeys"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        auto keys = map.mapKeys();
        std::vector<Variant> keyVariants;
        keyVariants.reserve(keys.size());
        for (const auto& k : keys)
            keyVariants.push_back(Variant(k));
        ctx.SetOutputValue("Keys", Variant(std::move(keyVariants)));
        return true;
    };

    handlers["MapValues"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        auto values = map.mapValues();
        ctx.SetOutputValue("Values", Variant(std::move(values)));
        return true;
    };

    handlers["MapClear"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        ctx.Log("  [MapClear] Cleared map (was size " + std::to_string(map.mapSize()) + ")");
        map.mapClear();
        ctx.SetOutputValue("Map", map);
        return true;
    };

    handlers["MapMerge"] = [](ExecutionContext& ctx) {
        auto mapA = ctx.GetInputValue("Map A");
        auto mapB = ctx.GetInputValue("Map B");
        // 以 Map A 为基础，Map B 的键值对覆盖/追加
        auto keysB = mapB.mapKeys();
        for (const auto& key : keysB)
            mapA.mapSet(key, mapB.mapGet(key));
        ctx.SetOutputValue("Map", mapA);
        ctx.Log("  [MapMerge] Merged (A size=" + std::to_string(mapA.mapSize()) + ")");
        return true;
    };

    handlers["ForEachMapLoop"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        auto keys = map.mapKeys();
        ctx.Log("  [ForEachMapLoop] Map size = " + std::to_string(keys.size()));
        for (const auto& key : keys)
        {
            ctx.SetOutputValue("Key", Variant(key));
            ctx.SetOutputValue("Value", map.mapGet(key));
            if (!ctx.ActivateOutputFlow("Loop Body"))
                return false;
        }
        ctx.ActivateOutputFlow("Completed");
        return true;
    };
}

// ============================================================================
// Array 处理器
// ============================================================================

} // namespace Runtime
} // namespace NodeEditor
