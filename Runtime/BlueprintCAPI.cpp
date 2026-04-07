// Runtime/BlueprintCAPI.cpp - Plain C interface implementation
//
// Each function simply delegates to the corresponding BlueprintRunner C++ method.
// The opaque BP_Runner handle is a heap-allocated BlueprintRunner*.

#include "BlueprintCAPI.h"
#include "BlueprintRunner.h"
#include "BuiltinHandlers.h"
#include "BuiltinNodeDefs.h"

#include <cstring>
#include <string>
#include <vector>
#if !defined(__EMSCRIPTEN__)
#  include <filesystem>
#endif
#if defined(_WIN32) || defined(_WIN64)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  undef IsLoggingEnabled   // 防止 Windows 宏污染 BlueprintRunner 方法名
#endif

using namespace NodeEditor::Runtime;

// ---------------------------------------------------------------------------
// Internal wrapper: wraps BlueprintRunner + last-error string
// ---------------------------------------------------------------------------

namespace {

struct RunnerWrapper
{
    BlueprintRunner runner;
    std::string     lastError;
    std::string     basePath;   // 蓝图文件所在目录，供 ExecuteBlueprint 相对路径解析

    // 重新注册内置 handler，使用当前 basePath
    void refreshHandlers()
    {
        RegisterBuiltinHandlers(runner, basePath);
    }

    // 静默加载 BlueprintEntry.lua：
    //   先找 basePath/BlueprintEntry.lua，再找 DLL 所在目录/BlueprintEntry.lua
    //   任一存在则加载（不报错，不存在直接跳过）
    void tryLoadBlueprintEntry()
    {
#if defined(BLUEPRINT_HAS_LUA) && !defined(__EMSCRIPTEN__)
        namespace fs = std::filesystem;
        std::vector<std::string> candidates;

        // 1. 蓝图文件所在目录
        if (!basePath.empty())
            candidates.push_back(basePath + "/BlueprintEntry.lua");

        // 2. DLL 所在目录（Windows: GetModuleFileNameA，其他平台跳过）
#if defined(_WIN32) || defined(_WIN64)
        {
            char dllPath[MAX_PATH] = {};
            HMODULE hm = nullptr;
            // 用模块内静态局部地址定位所在 DLL
            static const int kAnchor = 0;
            if (::GetModuleHandleExA(
                    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    reinterpret_cast<LPCSTR>(&kAnchor),
                    &hm))
            {
                ::GetModuleFileNameA(hm, dllPath, MAX_PATH);
                fs::path dllDir = fs::path(dllPath).parent_path();
                candidates.push_back((dllDir / "BlueprintEntry.lua").string());
            }
        }
#endif

        for (const auto& path : candidates)
        {
            if (fs::exists(path))
            {
                runner.LoadLuaScript(path);
                break;  // 找到第一个存在的就加载，不重复加载
            }
        }
#endif
    }
};

inline RunnerWrapper* asWrapper(BP_Runner h)
{
    return static_cast<RunnerWrapper*>(h);
}

// Copy at most bufLen-1 bytes of src into dst and NUL-terminate.
// Returns the number of bytes written (excluding NUL).
int copyString(const std::string& src, char* dst, int bufLen)
{
    if (!dst || bufLen <= 0) return 0;
    int len = static_cast<int>(src.size());
    if (len >= bufLen) len = bufLen - 1;
    std::memcpy(dst, src.c_str(), static_cast<size_t>(len));
    dst[len] = '\0';
    return len;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

extern "C" {

BLUEPRINT_CAPI_EXPORT BP_Runner BLUEPRINT_CAPI_CALL BP_CreateRunner(void)
{
    RunnerWrapper* w = new (std::nothrow) RunnerWrapper();
    if (!w) return nullptr;
    // Register all built-in node handlers
    RegisterBuiltinHandlers(w->runner);
    return static_cast<BP_Runner>(w);
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_DestroyRunner(BP_Runner runner)
{
    delete asWrapper(runner);
}

// ---------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_LoadFromJson(BP_Runner runner, const char* json)
{
    if (!runner || !json) return 1;
    auto* w = asWrapper(runner);
    if (!w->runner.LoadFromJson(std::string(json)))
    {
        w->lastError = "LoadFromJson failed";
        return 1;
    }
    w->lastError.clear();
    return 0;
}

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_LoadFromJsonWithBaseDir(
    BP_Runner runner, const char* json, const char* baseDir)
{
    if (!runner || !json) return 1;
    auto* w = asWrapper(runner);
    std::string baseDirStr = (baseDir && *baseDir) ? std::string(baseDir) : std::string("");
    if (!w->runner.LoadFromJsonWithDeps(std::string(json), baseDirStr))
    {
        w->lastError = w->runner.GetLastError();
        return 1;
    }
    // 设置 basePath 供 ExecuteBlueprint 节点解析相对路径
    w->basePath = baseDirStr;
    w->refreshHandlers();
    // 静默加载 BlueprintEntry.lua
    w->tryLoadBlueprintEntry();
    w->lastError.clear();
    return 0;
}

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_LoadFromFile(BP_Runner runner, const char* filePath)
{
    if (!runner || !filePath) return 1;
    auto* w = asWrapper(runner);
    // 使用 LoadFromFileWithDeps 自动递归加载 metadata.dependencies 中声明的 Library，
    // 确保 FuncLib.* 节点等依赖函数库的功能可以正常执行
    if (!w->runner.LoadFromFileWithDeps(std::string(filePath)))
    {
        w->lastError = std::string("LoadFromFile failed: ") + filePath;
        return 1;
    }
    // 自动提取文件目录作为 basePath，供 ExecuteBlueprint 节点解析相对路径
    std::string fp(filePath);
    size_t sl = fp.find_last_of("/\\");
    w->basePath = (sl != std::string::npos) ? fp.substr(0, sl) : "";
    w->refreshHandlers();
    // 静默加载 BlueprintEntry.lua（蓝图目录优先，次选 DLL 目录）
    w->tryLoadBlueprintEntry();
    w->lastError.clear();
    return 0;
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetBasePath(BP_Runner runner, const char* basePath)
{
    if (!runner) return;
    auto* w = asWrapper(runner);
    w->basePath = (basePath && *basePath) ? std::string(basePath) : std::string("");
    w->refreshHandlers();
}

/// Manually load a Lua script file into the runner's Lua engine.
/// The engine is created lazily on first call.
/// Silently succeeds (returns 0) if BLUEPRINT_HAS_LUA is not defined.
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_LoadLuaScript(BP_Runner runner, const char* filePath)
{
    if (!runner || !filePath) return 1;
#ifdef BLUEPRINT_HAS_LUA
    auto* w = asWrapper(runner);
    if (!w->runner.LoadLuaScript(std::string(filePath)))
    {
        w->lastError = w->runner.GetLastError();
        return 1;
    }
    w->lastError.clear();
#endif
    return 0;
}

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_IsLoaded(BP_Runner runner)
{
    if (!runner) return 0;
    return asWrapper(runner)->runner.IsLoaded() ? 1 : 0;
}

// ---------------------------------------------------------------------------
// Execution
// ---------------------------------------------------------------------------

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_Execute(BP_Runner runner)
{
    if (!runner) return 1;
    auto* w = asWrapper(runner);
    ExecutionResult result = w->runner.Execute();
    if (!result.success)
    {
        w->lastError = result.errorMessage;
        return 1;
    }
    w->lastError.clear();
    return 0;
}

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_DispatchEvent(BP_Runner runner, const char* eventDefinitionId)
{
    if (!runner || !eventDefinitionId) return 1;
    auto* w = asWrapper(runner);
    ExecutionResult result = w->runner.DispatchEvent(std::string(eventDefinitionId));
    if (!result.success)
    {
        w->lastError = result.errorMessage;
        return 1;
    }
    w->lastError.clear();
    return 0;
}

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_ExecuteNode(BP_Runner runner, uint64_t nodeId)
{
    if (!runner) return 1;
    auto* w = asWrapper(runner);
    ExecutionResult result = w->runner.ExecuteNode(static_cast<NodeId>(nodeId));
    if (!result.success)
    {
        w->lastError = result.errorMessage;
        return 1;
    }
    w->lastError.clear();
    return 0;
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_Tick(BP_Runner runner, float deltaTime)
{
    if (!runner) return;
    asWrapper(runner)->runner.Tick(deltaTime);
}

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_GetActiveTimerCount(BP_Runner runner)
{
    if (!runner) return 0;
    return static_cast<int>(asWrapper(runner)->runner.GetTimerManager().GetActiveTimerCount());
}

// ---------------------------------------------------------------------------
// Variables
// ---------------------------------------------------------------------------

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetVariableInt(BP_Runner runner, const char* name, int64_t value)
{
    if (!runner || !name) return;
    asWrapper(runner)->runner.SetVariable(name, Variant(value));
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetVariableFloat(BP_Runner runner, const char* name, double value)
{
    if (!runner || !name) return;
    asWrapper(runner)->runner.SetVariable(name, Variant(value));
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetVariableString(BP_Runner runner, const char* name, const char* value)
{
    if (!runner || !name) return;
    asWrapper(runner)->runner.SetVariable(name, Variant(value ? std::string(value) : std::string{}));
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetVariableBool(BP_Runner runner, const char* name, int value)
{
    if (!runner || !name) return;
    asWrapper(runner)->runner.SetVariable(name, Variant(value != 0));
}

BLUEPRINT_CAPI_EXPORT int64_t BLUEPRINT_CAPI_CALL BP_GetVariableInt(BP_Runner runner, const char* name)
{
    if (!runner || !name) return 0;
    return asWrapper(runner)->runner.GetVariable(name).asInt();
}

BLUEPRINT_CAPI_EXPORT double BLUEPRINT_CAPI_CALL BP_GetVariableFloat(BP_Runner runner, const char* name)
{
    if (!runner || !name) return 0.0;
    return asWrapper(runner)->runner.GetVariable(name).asFloat();
}

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_GetVariableBool(BP_Runner runner, const char* name)
{
    if (!runner || !name) return 0;
    return asWrapper(runner)->runner.GetVariable(name).asBool() ? 1 : 0;
}

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_GetVariableString(BP_Runner runner, const char* name, char* buf, int bufLen)
{
    if (!runner || !name) return -1;
    Variant v = asWrapper(runner)->runner.GetVariable(name);
    if (v.type == PinDataType::Unknown) return -1;
    return copyString(v.asString(), buf, bufLen);
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetLogCallback(BP_Runner runner, BP_LogCallback callback)
{
    if (!runner) return;
    auto* w = asWrapper(runner);
    if (callback)
        w->runner.SetLogCallback([callback](LogLevel lv, const std::string& msg) {
            callback(static_cast<BP_LogLevel>(lv), msg.c_str());
        });
    else
        w->runner.SetLogCallback(nullptr);
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_EnableLogging(BP_Runner runner, int enable)
{
    if (!runner) return;
    asWrapper(runner)->runner.EnableLogging(enable != 0);
}

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_IsLoggingEnabled(BP_Runner runner)
{
    if (!runner) return 0;
    return asWrapper(runner)->runner.IsLoggingEnabled() ? 1 : 0;
}

// ---------------------------------------------------------------------------
// Print (application-level output)
// ---------------------------------------------------------------------------

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetPrintCallback(BP_Runner runner, BP_LogCallback callback)
{
    if (!runner) return;
    auto* w = asWrapper(runner);
    if (callback)
        w->runner.SetPrintCallback([callback](LogLevel lv, const std::string& msg) {
            callback(static_cast<BP_LogLevel>(lv), msg.c_str());
        });
    else
        w->runner.SetPrintCallback(nullptr);
}

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_GetLastError(BP_Runner runner, char* buf, int bufLen)
{
    if (!runner) return 0;
    return copyString(asWrapper(runner)->lastError, buf, bufLen);
}

// ---------------------------------------------------------------------------
// Lua external state
// ---------------------------------------------------------------------------

#ifdef BLUEPRINT_HAS_LUA
#include "LuaScriptEngine.h"

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetExternalLuaState(BP_Runner runner, lua_State* L)
{
    if (!runner || !L) return;
    auto* w = asWrapper(runner);
    LuaScriptEngine* engine = w->runner.GetLuaEngine();
    if (!engine) return;
    if (engine->IsInitialized()) return;  // 已初始化则忽略，避免重复设置

    engine->InitializeWithExternalState(L, &w->runner);
}
#endif

// ---------------------------------------------------------------------------
// Script node definition registration
// ---------------------------------------------------------------------------

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_RegisterNodeDef(
    BP_Runner   runner,
    const char* id,
    const char* name,
    const char* category,
    const char* color,
    BP_PinDef*  pins,
    int         pinCount)
{
    if (!runner || !id || id[0] == '\0') return 1;
    auto* w = asWrapper(runner);

    NodeDefinition def;
    def.id       = id;
    def.name     = (name && name[0]) ? name : id;
    def.category = category ? category : "";
    def.color    = color    ? color    : "";

    for (int i = 0; i < pinCount; ++i)
    {
        const BP_PinDef& p = pins[i];
        PinDefinition pin;
        pin.name    = p.name ? p.name : "";
        pin.kind    = p.isInput ? PinKind::Input : PinKind::Output;
        pin.tooltip = p.tooltip ? p.tooltip : "";

        if (p.isExec)
        {
            pin.isExec   = true;
            pin.dataType = PinDataType::Unknown;
        }
        else
        {
            pin.isExec   = false;
            // BP_PinDataType 与 PinDataType 枚举值一一对应
            pin.dataType = static_cast<PinDataType>(p.dataType);
        }

        if (p.isInput)
            def.inputPins.push_back(std::move(pin));
        else
            def.outputPins.push_back(std::move(pin));
    }

    w->runner.RegisterNodeDef(def);
    return 0;
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_UnregisterNodeDef(BP_Runner runner, const char* id)
{
    if (!runner || !id) return;
    asWrapper(runner)->runner.UnregisterNodeDef(id);
}

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_HasNodeDef(BP_Runner runner, const char* id)
{
    if (!runner || !id) return 0;
    return asWrapper(runner)->runner.HasNodeDef(id) ? 1 : 0;
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_RegisterHandler(
    BP_Runner    runner,
    const char*  definitionId,
    BP_HandlerFn fn,
    void*        userdata)
{
    if (!runner || !definitionId || !fn) return;
    auto* w = asWrapper(runner);

    // 捕获 fn + userdata，包装成 C++ NodeHandler
    w->runner.RegisterHandler(definitionId,
        [fn, userdata](ExecutionContext& ctx) -> bool {
            return fn(static_cast<BP_Context>(&ctx), userdata) != 0;
        });
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_UnregisterHandler(
    BP_Runner runner, const char* definitionId)
{
    if (!runner || !definitionId) return;
    asWrapper(runner)->runner.UnregisterHandler(definitionId);
}

// ---------------------------------------------------------------------------
// ExecutionContext accessors
// ---------------------------------------------------------------------------

namespace {
inline ExecutionContext* asCtx(BP_Context c)
{
    return static_cast<ExecutionContext*>(c);
}
} // anonymous namespace

// --- Input ---

BLUEPRINT_CAPI_EXPORT int64_t BLUEPRINT_CAPI_CALL BP_GetInputInt(BP_Context ctx, const char* pin)
{
    if (!ctx || !pin) return 0;
    return asCtx(ctx)->GetInputValue(pin).asInt();
}

BLUEPRINT_CAPI_EXPORT double BLUEPRINT_CAPI_CALL BP_GetInputFloat(BP_Context ctx, const char* pin)
{
    if (!ctx || !pin) return 0.0;
    return asCtx(ctx)->GetInputValue(pin).asFloat();
}

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_GetInputBool(BP_Context ctx, const char* pin)
{
    if (!ctx || !pin) return 0;
    return asCtx(ctx)->GetInputValue(pin).asBool() ? 1 : 0;
}

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_GetInputString(BP_Context ctx, const char* pin,
                                                                  char* buf, int bufLen)
{
    if (!ctx || !pin) return -1;
    Variant v = asCtx(ctx)->GetInputValue(pin);
    if (v.type == PinDataType::Unknown) return -1;
    return copyString(v.asString(), buf, bufLen);
}

// --- Output ---

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetOutputInt(BP_Context ctx, const char* pin, int64_t val)
{
    if (!ctx || !pin) return;
    asCtx(ctx)->SetOutputValue(pin, Variant(val));
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetOutputFloat(BP_Context ctx, const char* pin, double val)
{
    if (!ctx || !pin) return;
    asCtx(ctx)->SetOutputValue(pin, Variant(val));
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetOutputBool(BP_Context ctx, const char* pin, int val)
{
    if (!ctx || !pin) return;
    asCtx(ctx)->SetOutputValue(pin, Variant(val != 0));
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetOutputString(BP_Context ctx, const char* pin,
                                                                    const char* val)
{
    if (!ctx || !pin) return;
    asCtx(ctx)->SetOutputValue(pin, Variant(val ? std::string(val) : std::string{}));
}

// --- Control flow ---

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_ActivateOutputFlow(BP_Context ctx, const char* pin)
{
    if (!ctx || !pin) return 0;
    return asCtx(ctx)->ActivateOutputFlow(pin) ? 1 : 0;
}

// --- Variables ---

BLUEPRINT_CAPI_EXPORT int64_t BLUEPRINT_CAPI_CALL BP_CtxGetVariableInt(BP_Context ctx, const char* name)
{
    if (!ctx || !name) return 0;
    return asCtx(ctx)->GetVariable(name).asInt();
}

BLUEPRINT_CAPI_EXPORT double BLUEPRINT_CAPI_CALL BP_CtxGetVariableFloat(BP_Context ctx, const char* name)
{
    if (!ctx || !name) return 0.0;
    return asCtx(ctx)->GetVariable(name).asFloat();
}

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_CtxGetVariableBool(BP_Context ctx, const char* name)
{
    if (!ctx || !name) return 0;
    return asCtx(ctx)->GetVariable(name).asBool() ? 1 : 0;
}

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_CtxGetVariableString(BP_Context ctx, const char* name,
                                                                        char* buf, int bufLen)
{
    if (!ctx || !name) return -1;
    Variant v = asCtx(ctx)->GetVariable(name);
    if (v.type == PinDataType::Unknown) return -1;
    return copyString(v.asString(), buf, bufLen);
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_CtxSetVariableInt(BP_Context ctx, const char* name,
                                                                      int64_t val)
{
    if (!ctx || !name) return;
    asCtx(ctx)->SetVariable(name, Variant(val));
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_CtxSetVariableFloat(BP_Context ctx, const char* name,
                                                                        double val)
{
    if (!ctx || !name) return;
    asCtx(ctx)->SetVariable(name, Variant(val));
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_CtxSetVariableBool(BP_Context ctx, const char* name,
                                                                       int val)
{
    if (!ctx || !name) return;
    asCtx(ctx)->SetVariable(name, Variant(val != 0));
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_CtxSetVariableString(BP_Context ctx, const char* name,
                                                                         const char* val)
{
    if (!ctx || !name) return;
    asCtx(ctx)->SetVariable(name, Variant(val ? std::string(val) : std::string{}));
}

// --- Logging / Print ---

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_CtxLog(BP_Context ctx, const char* msg)
{
    if (!ctx || !msg) return;
    asCtx(ctx)->Log(msg);
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_CtxLogWarn(BP_Context ctx, const char* msg)
{
    if (!ctx || !msg) return;
    asCtx(ctx)->LogWarning(msg);
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_CtxLogError(BP_Context ctx, const char* msg)
{
    if (!ctx || !msg) return;
    asCtx(ctx)->LogError(msg);
}

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_CtxPrint(BP_Context ctx, const char* msg)
{
    if (!ctx || !msg) return;
    asCtx(ctx)->Print(msg);
}

// --- Current node info ---

BLUEPRINT_CAPI_EXPORT uint64_t BLUEPRINT_CAPI_CALL BP_CtxGetCurrentNodeId(BP_Context ctx)
{
    if (!ctx) return 0;
    const NodeInstance* node = asCtx(ctx)->GetCurrentNode();
    return node ? static_cast<uint64_t>(node->id) : 0;
}

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_CtxGetCurrentNodeDefId(BP_Context ctx,
                                                                          char* buf, int bufLen)
{
    if (!ctx) return 0;
    const NodeInstance* node = asCtx(ctx)->GetCurrentNode();
    if (!node) return 0;
    return copyString(node->definitionId, buf, bufLen);
}

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_CtxGetActivatedInputPin(BP_Context ctx,
                                                                           char* buf, int bufLen)
{
    if (!ctx) return 0;
    return copyString(asCtx(ctx)->GetActivatedInputPinName(), buf, bufLen);
}

} // extern "C"
