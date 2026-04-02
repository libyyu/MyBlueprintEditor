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

using namespace NodeEditor::Runtime;

// ---------------------------------------------------------------------------
// Internal wrapper: wraps BlueprintRunner + last-error string
// ---------------------------------------------------------------------------

namespace {

struct RunnerWrapper
{
    BlueprintRunner runner;
    std::string     lastError;
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

BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_LoadFromFile(BP_Runner runner, const char* filePath)
{
    if (!runner || !filePath) return 1;
    auto* w = asWrapper(runner);
    if (!w->runner.LoadFromFile(std::string(filePath)))
    {
        w->lastError = std::string("LoadFromFile failed: ") + filePath;
        return 1;
    }
    w->lastError.clear();
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

} // extern "C"
