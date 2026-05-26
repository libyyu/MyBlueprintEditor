// Runtime/handlers/BuiltinHandlers_Agent.cpp
// Agent 扩展节点：
//
//   Memory.Store       — 向全局键值记忆库写入条目（可选 TTL）
//   Memory.Recall      — 从记忆库读取条目（支持模糊键前缀搜索）
//   Memory.Clear       — 清空记忆库（全部或指定前缀）
//   Tool.Register      — 在蓝图内动态注册工具（供 Tool.CallByName 调用）
//   Trigger.Cron       — 定时触发（基于 SetTimer 循环，支持 cron-like 间隔）
//   Trigger.FileWatch  — 文件变化触发（轮询 mtime）

#include "BuiltinHandlers_Agent.h"
#include "../SharedRegistry.h"
#include "../BlueprintRunner.h"
#include "../BlueprintExporter.h"
#include "../../Utils/Json/crude_json.h"
#include "../FrameTimerManager.h"

#ifndef __EMSCRIPTEN__
#include <filesystem>
#endif

#include <mutex>
#include <unordered_map>
#include <string>
#include <vector>
#include <atomic>
#include <ctime>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 全局记忆库（进程级，跨 runner 共享）
// ============================================================================
struct MemoryEntry {
    std::string value;
    double      expiresAt = 0.0; // 0 = 永不过期
};

static std::mutex                              s_memMutex;
static std::unordered_map<std::string, MemoryEntry> s_memStore;

static void memPurgeExpired()
{
    double now = static_cast<double>(FrameTimerManager::GetCurrentUnixTime());
    for (auto it = s_memStore.begin(); it != s_memStore.end(); ) {
        if (it->second.expiresAt > 0.0 && it->second.expiresAt <= now)
            it = s_memStore.erase(it);
        else
            ++it;
    }
}

// ============================================================================
void RegisterHandlers_Agent(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner)
{
    // ========================================================================
    // Memory.Store
    // 向全局记忆库写入一个键值对
    // in:  exec, Key(String), Value(String), TTL(Float, 0=永久)
    // out: exec
    // ========================================================================
    handlers["Memory.Store"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string key   = ctx.GetInputValue("Key").asString();
        std::string value = ctx.GetInputValue("Value").asString();
        double ttl        = ctx.GetInputValue("TTL").asFloat();

        if (key.empty()) {
            ctx.LogError("[Memory.Store] Key is empty");
            return true;
        }

        double expiresAt = 0.0;
        if (ttl > 0.0)
            expiresAt = static_cast<double>(FrameTimerManager::GetCurrentUnixTime()) + ttl;

        {
            std::lock_guard<std::mutex> lk(s_memMutex);
            s_memStore[key] = MemoryEntry{value, expiresAt};
        }
        ctx.Log("[Memory.Store] key='" + key + "' ttl=" + std::to_string(ttl));
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // ========================================================================
    // Memory.Recall
    // 从记忆库读取键值
    // in:  Key(String), Prefix(String, 若非空则返回所有前缀匹配项的JSON对象)
    // out: Value(String), Found(Bool), AllMatches(String JSON object)
    // ========================================================================
    handlers["Memory.Recall"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string key    = ctx.GetInputValue("Key").asString();
        std::string prefix = ctx.GetInputValue("Prefix").asString();

        std::lock_guard<std::mutex> lk(s_memMutex);
        memPurgeExpired();

        if (!prefix.empty()) {
            // 前缀搜索 → AllMatches JSON object
            crude_json::object obj;
            for (const auto& kv : s_memStore) {
                if (kv.first.substr(0, prefix.size()) == prefix)
                    obj[kv.first] = crude_json::value(kv.second.value);
            }
            std::string json = crude_json::value(std::move(obj)).dump();
            ctx.SetOutputValue("Value",      Variant(std::string("")));
            ctx.SetOutputValue("Found",      Variant(!obj.empty()));
            ctx.SetOutputValue("AllMatches", Variant(json));
            return true;
        }

        // 精确键查找
        auto it = s_memStore.find(key);
        bool found = (it != s_memStore.end());
        ctx.SetOutputValue("Value",      Variant(found ? it->second.value : std::string("")));
        ctx.SetOutputValue("Found",      Variant(found));
        ctx.SetOutputValue("AllMatches", Variant(std::string("{}")));
        return true;
    };

    // ========================================================================
    // Memory.Clear
    // 清空记忆库（全部，或指定 Prefix 前缀）
    // in:  exec, Prefix(String, 空=清全部)
    // out: exec, RemovedCount(Integer)
    // ========================================================================
    handlers["Memory.Clear"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string prefix = ctx.GetInputValue("Prefix").asString();
        std::lock_guard<std::mutex> lk(s_memMutex);
        int64_t removed = 0;
        if (prefix.empty()) {
            removed = static_cast<int64_t>(s_memStore.size());
            s_memStore.clear();
        } else {
            for (auto it = s_memStore.begin(); it != s_memStore.end(); ) {
                if (it->first.substr(0, prefix.size()) == prefix) {
                    it = s_memStore.erase(it);
                    ++removed;
                } else {
                    ++it;
                }
            }
        }
        ctx.SetOutputValue("RemovedCount", Variant(removed));
        ctx.Log("[Memory.Clear] removed=" + std::to_string(removed));
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // ========================================================================
    // Tool.Register
    // 在蓝图内动态注册一个工具（供 Tool.CallByName 调用）
    // 工具实现通过子蓝图 FilePath 指定（注册后由 Tool.CallByName 路由过去）
    // in:  exec, Name(String), FilePath(String), Description(String)
    // out: exec, ErrorMessage(String)
    // ========================================================================
    handlers["Tool.Register"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string name        = ctx.GetInputValue("Name").asString();
        std::string filePath    = ctx.GetInputValue("FilePath").asString();
        std::string description = ctx.GetInputValue("Description").asString();

        if (name.empty()) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Name is empty")));
            ctx.ActivateOutputFlow("exec");
            return true;
        }

        // 将工具映射信息写入全局记忆库，Tool.CallByName 会在这里查找
        // key: "__tool_reg_<name>" value: JSON {filePath, description}
        std::string regKey = "__tool_reg_" + name;
        crude_json::object info;
        info["filePath"]    = crude_json::value(filePath);
        info["description"] = crude_json::value(description);
        {
            std::lock_guard<std::mutex> lk(s_memMutex);
            s_memStore[regKey] = MemoryEntry{crude_json::value(std::move(info)).dump(), 0.0};
        }

        // 同时注册一个 handler：当 Tool.CallByName 调用 name 时，
        // 路由到对应子蓝图执行（同步简化版）
        // Handler 全局共享（HandlerRegistry），无需快照
        HandlerRegistry::Instance().Register("__dyntool_" + name,
            [name, filePath](ExecutionContext& c) -> bool {
                auto* r = c.GetRunner();
                std::string args = c.GetInputValue("Arguments").asString();
                // 创建子 runner；handler 全局共享
                auto subRunner = std::make_unique<BlueprintRunner>(r->GetFileSystem());
                subRunner->SetParentTimerManager(r->GetTimerManagerPtr());
                subRunner->SetLogCallback([&c](LogLevel, const std::string& m){ c.Log(m); });
                if (!subRunner->LoadFromFileWithDeps(filePath)) {
                    c.SetOutputValue("Result", Variant(std::string("[Error] cannot load: " + filePath)));
                    c.ActivateOutputFlow("onNotFound");
                    return true;
                }
                if (!args.empty()) {
                    crude_json::value params = crude_json::value::parse(args);
                    if (params.is_object()) {
                        for (const auto& kv : params.get<crude_json::object>()) {
                            if      (kv.second.is_string())  subRunner->SetVariable(kv.first, Variant(kv.second.get<std::string>()));
                            else if (kv.second.is_number())  subRunner->SetVariable(kv.first, Variant(kv.second.get<double>()));
                            else if (kv.second.is_boolean()) subRunner->SetVariable(kv.first, Variant(kv.second.get<bool>()));
                            else subRunner->SetVariable(kv.first, Variant(kv.second.dump()));
                        }
                    }
                }
                subRunner->Execute();
                subRunner->DispatchEvent("OnBeginPlay");
                // 读取 Result 变量作为输出
                Variant result = subRunner->GetVariable("Result");
                c.SetOutputValue("Result", result.type != PinDataType::Unknown
                    ? result : Variant(std::string("[done]")));
                c.ActivateOutputFlow("onSuccess");
                return true;
            });

        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.Log("[Tool.Register] registered tool '" + name + "' → " + filePath);
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // ========================================================================
    // Trigger.Cron
    // 定时周期触发蓝图事件（以秒为单位的间隔，非标准 cron 表达式）
    // in:  exec, IntervalSec(Float), MaxCount(Integer, 0=无限), EventName(String)
    // out: onTick(exec), onDone(exec)
    //      TickCount(Integer)
    // ========================================================================
    handlers["Trigger.Cron"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        double  interval  = ctx.GetInputValue("IntervalSec").asFloat();
        int64_t maxCount  = ctx.GetInputValue("MaxCount").asInt();
        std::string event = ctx.GetInputValue("EventName").asString();

        if (interval <= 0.0) interval = 1.0;
        if (event.empty())   event    = "OnTick";

        auto* node = ctx.GetCurrentNode();
        std::string countKey = "__cron_count_" + (node ? std::to_string(node->id) : "0");

        ctx.SetVariable(countKey, Variant(int64_t(0)));

        PinId tickPinId = ctx.GetPinId("onTick");
        PinId donePinId = ctx.GetPinId("onDone");
        ctx.MarkDownstreamAsHandled("onTick");
        ctx.MarkDownstreamAsHandled("onDone");

        ExecutionContext* pCtx = &ctx;
        auto alive = runner->GetAliveFlag();

        ctx.SetTimer(static_cast<float>(interval), -1,
            [pCtx, maxCount, countKey, tickPinId, donePinId, alive]() -> bool {
                if (!alive->load(std::memory_order_acquire)) return false;

                int64_t cnt = pCtx->GetVariable(countKey).asInt() + 1;
                pCtx->SetVariable(countKey, Variant(cnt));
                pCtx->SetOutputValue("TickCount", Variant(cnt));
                pCtx->Log("[Trigger.Cron] tick #" + std::to_string(cnt));
                pCtx->ActivateOutputFlow(tickPinId);

                if (maxCount > 0 && cnt >= maxCount) {
                    pCtx->ActivateOutputFlow(donePinId);
                    return false; // 停止定时器
                }
                return true; // 继续
            });

        return true;
    };

    // ========================================================================
    // Trigger.FileWatch
    // 监视文件变化（轮询 mtime），变化时触发 onChanged
    // in:  exec, FilePath(String), PollIntervalSec(Float, 默认1.0)
    // out: onChanged(exec), onError(exec)
    //      FilePath(String) — 变化的文件路径
    // ========================================================================
    handlers["Trigger.FileWatch"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
#ifdef __EMSCRIPTEN__
        ctx.SetOutputValue("FilePath", Variant(std::string("")));
        ctx.LogError("[Trigger.FileWatch] not supported on WebGL");
        ctx.ActivateOutputFlow("onError");
        return true;
#else
        std::string filePath   = ctx.GetInputValue("FilePath").asString();
        double pollInterval    = ctx.GetInputValue("PollIntervalSec").asFloat();
        if (pollInterval <= 0) pollInterval = 1.0;

        if (filePath.empty()) {
            ctx.LogError("[Trigger.FileWatch] FilePath is empty");
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        auto* node = ctx.GetCurrentNode();
        std::string mtimeKey = "__filewatch_mtime_" + (node ? std::to_string(node->id) : "0");

        // 初始化 mtime
        double initialMtime = 0.0;
        {
            namespace fs = std::filesystem;
            std::error_code ec;
            if (fs::exists(filePath, ec)) {
                auto lwt = fs::last_write_time(filePath, ec);
                if (!ec) {
                    auto dur = lwt.time_since_epoch();
                    initialMtime = static_cast<double>(
                        std::chrono::duration_cast<std::chrono::seconds>(dur).count());
                }
            }
        }
        ctx.SetVariable(mtimeKey, Variant(initialMtime));

        PinId changedPinId = ctx.GetPinId("onChanged");
        PinId errorPinId   = ctx.GetPinId("onError");
        ctx.MarkDownstreamAsHandled("onChanged");
        ctx.MarkDownstreamAsHandled("onError");

        ExecutionContext* pCtx = &ctx;
        auto alive = runner->GetAliveFlag();

        ctx.SetTimer(static_cast<float>(pollInterval), -1,
            [pCtx, filePath, mtimeKey, changedPinId, errorPinId, alive]() -> bool {
                if (!alive->load(std::memory_order_acquire)) return false;

                namespace fs = std::filesystem;
                std::error_code ec;
                if (!fs::exists(filePath, ec)) return true; // 文件不存在，继续轮询

                auto lwt = fs::last_write_time(filePath, ec);
                if (ec) return true;

                double mtime = static_cast<double>(
                    std::chrono::duration_cast<std::chrono::seconds>(
                        lwt.time_since_epoch()).count());
                double lastMtime = pCtx->GetVariable(mtimeKey).asFloat();

                if (mtime != lastMtime) {
                    pCtx->SetVariable(mtimeKey, Variant(mtime));
                    pCtx->SetOutputValue("FilePath", Variant(filePath));
                    pCtx->Log("[Trigger.FileWatch] changed: " + filePath);
                    pCtx->ActivateOutputFlow(changedPinId);
                }
                return true; // 持续轮询
            });

        return true;
#endif
    };
}

} // namespace Runtime
} // namespace NodeEditor
