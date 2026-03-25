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

BLUEPRINT_CAPI BP_Runner BP_CreateRunner(void)
{
    RunnerWrapper* w = new (std::nothrow) RunnerWrapper();
    if (!w) return nullptr;
    // Register all built-in node handlers
    RegisterBuiltinHandlers(w->runner);
    return static_cast<BP_Runner>(w);
}

BLUEPRINT_CAPI void BP_DestroyRunner(BP_Runner runner)
{
    delete asWrapper(runner);
}

// ---------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------

BLUEPRINT_CAPI int BP_LoadFromJson(BP_Runner runner, const char* json)
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

BLUEPRINT_CAPI int BP_LoadFromFile(BP_Runner runner, const char* filePath)
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

BLUEPRINT_CAPI int BP_IsLoaded(BP_Runner runner)
{
    if (!runner) return 0;
    return asWrapper(runner)->runner.IsLoaded() ? 1 : 0;
}

// ---------------------------------------------------------------------------
// Execution
// ---------------------------------------------------------------------------

BLUEPRINT_CAPI int BP_Execute(BP_Runner runner)
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

BLUEPRINT_CAPI int BP_ExecuteNode(BP_Runner runner, uint64_t nodeId)
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

BLUEPRINT_CAPI void BP_Tick(BP_Runner runner, float deltaTime)
{
    if (!runner) return;
    asWrapper(runner)->runner.Tick(deltaTime);
}

// ---------------------------------------------------------------------------
// Variables
// ---------------------------------------------------------------------------

BLUEPRINT_CAPI void BP_SetVariableInt(BP_Runner runner, const char* name, int64_t value)
{
    if (!runner || !name) return;
    asWrapper(runner)->runner.SetVariable(name, Variant(value));
}

BLUEPRINT_CAPI void BP_SetVariableFloat(BP_Runner runner, const char* name, double value)
{
    if (!runner || !name) return;
    asWrapper(runner)->runner.SetVariable(name, Variant(value));
}

BLUEPRINT_CAPI void BP_SetVariableString(BP_Runner runner, const char* name, const char* value)
{
    if (!runner || !name) return;
    asWrapper(runner)->runner.SetVariable(name, Variant(value ? std::string(value) : std::string{}));
}

BLUEPRINT_CAPI void BP_SetVariableBool(BP_Runner runner, const char* name, int value)
{
    if (!runner || !name) return;
    asWrapper(runner)->runner.SetVariable(name, Variant(value != 0));
}

BLUEPRINT_CAPI int64_t BP_GetVariableInt(BP_Runner runner, const char* name)
{
    if (!runner || !name) return 0;
    return asWrapper(runner)->runner.GetVariable(name).asInt();
}

BLUEPRINT_CAPI double BP_GetVariableFloat(BP_Runner runner, const char* name)
{
    if (!runner || !name) return 0.0;
    return asWrapper(runner)->runner.GetVariable(name).asFloat();
}

BLUEPRINT_CAPI int BP_GetVariableBool(BP_Runner runner, const char* name)
{
    if (!runner || !name) return 0;
    return asWrapper(runner)->runner.GetVariable(name).asBool() ? 1 : 0;
}

BLUEPRINT_CAPI int BP_GetVariableString(BP_Runner runner, const char* name, char* buf, int bufLen)
{
    if (!runner || !name) return -1;
    Variant v = asWrapper(runner)->runner.GetVariable(name);
    if (v.type == PinDataType::Unknown) return -1;
    return copyString(v.asString(), buf, bufLen);
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

BLUEPRINT_CAPI void BP_SetLogCallback(BP_Runner runner, BP_LogCallback callback)
{
    if (!runner) return;
    auto* w = asWrapper(runner);
    if (callback)
    {
        w->runner.SetLogCallback([callback](const std::string& msg)
        {
            callback(msg.c_str());
        });
    }
    else
    {
        w->runner.SetLogCallback(nullptr);
    }
}

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

BLUEPRINT_CAPI int BP_GetLastError(BP_Runner runner, char* buf, int bufLen)
{
    if (!runner) return 0;
    return copyString(asWrapper(runner)->lastError, buf, bufLen);
}

} // extern "C"
