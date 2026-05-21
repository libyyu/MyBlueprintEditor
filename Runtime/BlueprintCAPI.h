// Runtime/BlueprintCAPI.h - Plain C interface for Unity DllImport
//
// All functions are declared as:
//   BLUEPRINT_CAPI_EXPORT return_type BLUEPRINT_CAPI_CALL funcname(args);
//
// This correctly places __declspec(dllexport) before the return type and
// __cdecl (calling convention) between the return type and function name,
// as required by MSVC on Windows.
//
// Typical Unity usage (C#):
//
//   using System.Runtime.InteropServices;
//
//   public static class BlueprintNative {
//       const string DLL = "BlueprintRuntime";
//
//       [DllImport(DLL)] public static extern IntPtr  BP_CreateRunner();
//       [DllImport(DLL)] public static extern void    BP_DestroyRunner(IntPtr runner);
//       [DllImport(DLL)] public static extern int     BP_LoadFromJson(IntPtr runner, string json);
//       [DllImport(DLL)] public static extern int     BP_Execute(IntPtr runner);
//       [DllImport(DLL)] public static extern void    BP_Tick(IntPtr runner, float deltaTime);
//       [DllImport(DLL)] public static extern void    BP_SetVariableInt(IntPtr runner, string name, long value);
//       [DllImport(DLL)] public static extern void    BP_SetVariableFloat(IntPtr runner, string name, double value);
//       [DllImport(DLL)] public static extern void    BP_SetVariableString(IntPtr runner, string name, string value);
//       [DllImport(DLL)] public static extern void    BP_SetVariableBool(IntPtr runner, string name, int value);
//       [DllImport(DLL)] public static extern long    BP_GetVariableInt(IntPtr runner, string name);
//       [DllImport(DLL)] public static extern double  BP_GetVariableFloat(IntPtr runner, string name);
//       [DllImport(DLL)] public static extern int     BP_GetVariableBool(IntPtr runner, string name);
//       [DllImport(DLL)] public static extern int     BP_GetVariableString(IntPtr runner, string name, IntPtr buf, int bufLen);
//       [DllImport(DLL)] public static extern void    BP_SetLogCallback(IntPtr runner, LogCallback cb);
//       [DllImport(DLL)] public static extern int     BP_GetLastError(IntPtr runner, IntPtr buf, int bufLen);
//
//       [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
//       public delegate void LogCallback(string message);
//   }
//
// Notes:
//   - All opaque handles (BP_Runner*) map to IntPtr in C#.
//   - String parameters are passed as const char* (marshalled as UTF-8 by Unity).
//   - Return value 0 = success / false, 1 = failure / true (matching C convention).
//   - On Emscripten the same functions are exported via EMSCRIPTEN_KEEPALIVE and
//     can be called from JavaScript via Module.ccall / Module.cwrap.

#pragma once

#include "BlueprintExport.h"
#include <stdint.h>

// ---------------------------------------------------------------------------
// BLUEPRINT_CAPI - calling convention + visibility for C exports
// ---------------------------------------------------------------------------

#if defined(__EMSCRIPTEN__)
#   include <emscripten.h>
#   define BLUEPRINT_CAPI_EXPORT EMSCRIPTEN_KEEPALIVE
#   define BLUEPRINT_CAPI_CALL
#elif defined(_WIN32) || defined(_WIN64)
#   define BLUEPRINT_CAPI_EXPORT __declspec(dllexport)
#   define BLUEPRINT_CAPI_CALL   __cdecl
#elif defined(__GNUC__) || defined(__clang__)
#   define BLUEPRINT_CAPI_EXPORT __attribute__((visibility("default")))
#   define BLUEPRINT_CAPI_CALL
#else
#   define BLUEPRINT_CAPI_EXPORT
#   define BLUEPRINT_CAPI_CALL
#endif

// Full declaration syntax (correct on all platforms):
//   BLUEPRINT_CAPI_EXPORT return_type BLUEPRINT_CAPI_CALL funcname(args);
//
// On Windows:    __declspec(dllexport) ret __cdecl funcname(args)
// On Linux/macOS: __attribute__((visibility("default"))) ret funcname(args)
// On Emscripten: EMSCRIPTEN_KEEPALIVE ret funcname(args)

// ---------------------------------------------------------------------------
// Opaque handle
// ---------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

typedef void* BP_Runner;

// Log level – matches NodeEditor::Runtime::LogLevel
typedef enum BP_LogLevel {
    BP_LOG_VERBOSE = 0,
    BP_LOG_INFO    = 1,
    BP_LOG_WARNING = 2,
    BP_LOG_ERROR   = 3
} BP_LogLevel;

// Log/print callback type: (level, message)
typedef void (*BP_LogCallback)(BP_LogLevel level, const char* message);

// ---------------------------------------------------------------------------
// Global HTTP client (shared across all runners)
// ---------------------------------------------------------------------------

/// Register the built-in HTTP client (cpp-httplib on native, emscripten_fetch on WebGL).
/// Must be called once before any LLM.Chat / http.* Lua nodes execute.
/// Safe to call multiple times (idempotent – only registers if not already set).
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_InitDefaultHttpClient(void);

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

/// Create a new BlueprintRunner instance. Returns NULL on OOM.
BLUEPRINT_CAPI_EXPORT BP_Runner BLUEPRINT_CAPI_CALL BP_CreateRunner(void);

/// Destroy a runner created with BP_CreateRunner. Safe to call with NULL.
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_DestroyRunner(BP_Runner runner);

// ---------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------

/// Load blueprint from a JSON string.
/// Returns 0 on success, non-zero on failure (call BP_GetLastError for details).
/// Note: dependencies declared in metadata.dependencies are NOT loaded.
/// Use BP_LoadFromJsonWithBaseDir to load dependencies automatically.
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_LoadFromJson(BP_Runner runner, const char* json);

/// Load blueprint from a JSON string and automatically load dependencies.
/// baseDir: directory used to resolve relative paths in metadata.dependencies.
///   - If the JSON was read from a file, pass the file's parent directory.
///   - Pass NULL or "" to use the current working directory.
/// Returns 0 on success, non-zero on failure.
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_LoadFromJsonWithBaseDir(
    BP_Runner runner, const char* json, const char* baseDir);

/// Load blueprint from a file path (uses the runner's IFileSystem).
/// Returns 0 on success, non-zero on failure.
/// Note: also calls BP_SetBasePath internally with the file's parent directory.
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_LoadFromFile(BP_Runner runner, const char* filePath);

/// Set the base directory used to resolve relative paths in ExecuteBlueprint nodes.
/// Automatically called by BP_LoadFromFile with the loaded file's parent directory.
/// Call this manually when using BP_LoadFromJson/BP_LoadFromJsonWithBaseDir.
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetBasePath(BP_Runner runner, const char* basePath);

/// Load a Lua script file into the runner's Lua VM.
/// The VM is created lazily on first call. File not found or syntax error → returns 1.
/// No-op (returns 0) when the library was compiled without BLUEPRINT_HAS_LUA.
/// BP_LoadFromFile and BP_LoadFromJsonWithBaseDir automatically attempt to load
/// BlueprintEntry.lua from the blueprint directory (then the DLL directory).
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_LoadLuaScript(BP_Runner runner, const char* filePath);

/// Set the global Lua entry file path used by all runners when loading blueprints.
/// Search order in tryLoadBlueprintEntry:
///   1. This path (highest priority)
///   2. Blueprint file directory/BlueprintEntry.lua
///   3. Current working directory/BlueprintEntry.lua
///   4. DLL/exe directory/BlueprintEntry.lua
/// Pass NULL or "" to clear (disable).
/// Thread-safe. No-op when BLUEPRINT_HAS_LUA is not defined.
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetGlobalLuaEntry(const char* filePath);

/// Get the current global Lua entry path. Returns number of bytes written.
BLUEPRINT_CAPI_EXPORT int  BLUEPRINT_CAPI_CALL BP_GetGlobalLuaEntry(char* buf, int bufLen);

/// Immediately load the global Lua entry (or auto-search) into the runner.
/// Useful after calling BP_SetGlobalLuaEntry to force an immediate reload.
BLUEPRINT_CAPI_EXPORT int  BLUEPRINT_CAPI_CALL BP_LoadGlobalLuaEntry(BP_Runner runner);

/// Returns 1 if a blueprint has been loaded successfully, 0 otherwise.
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_IsLoaded(BP_Runner runner);

// ---------------------------------------------------------------------------
// Execution
// ---------------------------------------------------------------------------

/// Execute the blueprint from its entry point(s).
/// Returns 0 on success, non-zero on failure.
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_Execute(BP_Runner runner);

/// Execute data-flow + dispatch "OnBeginPlay" in one call.
/// This is the recommended way to run a blueprint — equivalent to:
///   BP_Execute(runner);
///   BP_DispatchEvent(runner, "OnBeginPlay");
/// Returns 0 on success, non-zero on failure.
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_ExecuteAll(BP_Runner runner);

/// Dispatch a named event (e.g. "OnBeginPlay", "OnTick").
/// Finds the event source node with the matching definitionId and executes its
/// exec-downstream chain. Returns 0 on success, non-zero on failure.
/// If the blueprint has no matching event node, returns 0 (silent no-op).
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_DispatchEvent(BP_Runner runner, const char* eventDefinitionId);

/// Execute a single node by ID.
/// Returns 0 on success, non-zero on failure.
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_ExecuteNode(BP_Runner runner, uint64_t nodeId);

/// Advance async timers (call once per frame with your deltaTime in seconds).
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_Tick(BP_Runner runner, float deltaTime);

/// Returns the number of active (pending) async timers.
/// Prefer BP_HasPendingWork() over checking this directly.
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_GetActiveTimerCount(BP_Runner runner);

/// Returns 1 if there is any pending work: async operations (HTTP/LLM/FireEvent)
/// OR active timers (Delay/SetTimer). Use as the single Tick loop exit condition:
///   while (BP_HasPendingWork(runner)) { BP_DrainQueue(); BP_Tick(runner, dt); }
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_HasPendingWork(BP_Runner runner);

/// Drain the global MainThreadDispatcher queue.
/// Must be called each frame BEFORE BP_Tick to process FireEvent callbacks and
/// async HTTP/LLM results posted by background threads.
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_DrainQueue(void);

// ---------------------------------------------------------------------------
// Variables
// ---------------------------------------------------------------------------

BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetVariableInt(BP_Runner runner, const char* name, int64_t value);
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetVariableFloat(BP_Runner runner, const char* name, double value);
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetVariableString(BP_Runner runner, const char* name, const char* value);
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetVariableBool(BP_Runner runner, const char* name, int value); // 0=false 1=true

BLUEPRINT_CAPI_EXPORT int64_t BLUEPRINT_CAPI_CALL BP_GetVariableInt(BP_Runner runner, const char* name);
BLUEPRINT_CAPI_EXPORT double BLUEPRINT_CAPI_CALL BP_GetVariableFloat(BP_Runner runner, const char* name);
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_GetVariableBool(BP_Runner runner, const char* name); // returns 0 or 1

/// Copy the string variable into buf (at most bufLen-1 bytes + NUL).
/// Returns the number of bytes written (excluding NUL), or -1 if not found.
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_GetVariableString(BP_Runner runner, const char* name, char* buf, int bufLen);

// ---------------------------------------------------------------------------
// Logging  (internal debug diagnostics – gated by BP_EnableLogging)
// ---------------------------------------------------------------------------

/// Register a callback that receives internal debug/diagnostic messages.
/// Only fires when logging is enabled (see BP_EnableLogging).
/// Pass NULL to clear.
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetLogCallback(BP_Runner runner, BP_LogCallback callback);

/// Enable or disable internal debug logging.
/// Default: 0 (disabled) when built with NDEBUG (Release), 1 otherwise.
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_EnableLogging(BP_Runner runner, int enable);

/// Returns 1 if internal logging is currently enabled, 0 otherwise.
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_IsLoggingEnabled(BP_Runner runner);

// ---------------------------------------------------------------------------
// Print  (application-level output – PrintString / Log / FormatLog nodes)
// ---------------------------------------------------------------------------

/// Register a callback that receives output from PrintString / Log / FormatLog.
/// This is the channel that game engines (Unity, etc.) should override.
/// Independent of BP_EnableLogging – always fires when set.
/// If not set, output falls back to the log callback (if logging enabled)
/// and then to stderr.
/// Pass NULL to clear.
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetPrintCallback(BP_Runner runner, BP_LogCallback callback);

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

/// Copy the last error message into buf (at most bufLen-1 bytes + NUL).
/// Returns the number of bytes written (excluding NUL).
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_GetLastError(BP_Runner runner, char* buf, int bufLen);

// ---------------------------------------------------------------------------
// Lua external state (iOS / Emscripten / static-link platforms)
// ---------------------------------------------------------------------------
// On platforms where Blueprint is linked statically alongside another Lua host
// (e.g. xLua on iOS), duplicate Lua symbols cause linker errors.
// Use BP_SetExternalLuaState() to share the host's lua_State with Blueprint
// instead of letting Blueprint create its own VM.
//
// Must be called BEFORE BP_Initialize() (or the first script load).
// The lua_State lifetime is managed by the caller; Blueprint will NOT call
// lua_close() on it.
//
// On Android / macOS / Windows (dynamic Lua), this is a no-op — Blueprint
// uses its own VM as usual.
//
// Only available when BLUEPRINT_HAS_LUA is defined.
#ifdef BLUEPRINT_HAS_LUA
struct lua_State;
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetExternalLuaState(BP_Runner runner, lua_State* L);
/// Get the internal lua_State used by this runner.
/// Returns NULL if the runner is not yet initialized or has no Lua engine.
/// Use this to attach an xLua LuaEnv to the same VM as the Blueprint Runtime.
BLUEPRINT_CAPI_EXPORT lua_State* BLUEPRINT_CAPI_CALL BP_GetLuaState(BP_Runner runner);

/// Notify Blueprint that a lua_State is going to be closed by the host.
///
/// Must be called BEFORE host's lua_close(L) when:
///   - Any-typed Variant pins were used (ctx:SetOutputAny / GetInputAny)
///   - The lua_State is shared via BP_SetExternalLuaState
///
/// Without this call, Variants that outlive the VM (e.g. cached in a graph
/// state, kept across VM swaps) will, on destruction, call luaL_unref on a
/// dangling lua_State* and crash.
///
/// After this call, all Any Variants tied to L become safe-noop on destruction
/// (skip luaL_unref because the registry has been GC'd by lua_close).
///
/// Safe to call multiple times; safe to call on a state never seen by Blueprint.
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_NotifyLuaStateClosing(lua_State* L);
#endif

// ---------------------------------------------------------------------------
// Script node definition registration  (Lua / C# shared)
// ---------------------------------------------------------------------------
//
// Allows Lua scripts and C# code to dynamically register node definitions
// and execution handlers at runtime, equivalent to what Lua scripts can do
// via Blueprint.RegisterNodeDef() / Blueprint.RegisterHandler().
//
// C# usage example:
//
//   // 1. Define pin descriptors
//   BP_PinDef[] pins = {
//       new BP_PinDef { name="A",      dataType=2 /*Float*/, isInput=1 },
//       new BP_PinDef { name="B",      dataType=2 /*Float*/, isInput=1 },
//       new BP_PinDef { name="Result", dataType=2 /*Float*/, isInput=0 },
//   };
//   BP_RegisterNodeDef(runner, "MyAdd", "My Add", "Custom/Math", null, pins, 3);
//
//   // 2. Register handler
//   BP_RegisterHandler(runner, "MyAdd", (ctx, ud) => {
//       float a = BP_GetInputFloat(ctx, "A");
//       float b = BP_GetInputFloat(ctx, "B");
//       BP_SetOutputFloat(ctx, "Result", a + b);
//       return 1;
//   }, IntPtr.Zero);
//
// PinDataType integer values (BP_PinDef.dataType):
//   0=Unknown/Any  1=Boolean  2=Integer  3=Float  4=String
//   5=Object       6=Array    7=Map      8=Set
//
// For Flow (exec) pins: set isExec=1 (dataType is ignored).

/// Opaque execution context handle — valid only inside a BP_HandlerFn call.
typedef void* BP_Context;

/// Pin data type constants (match NodeEditor::Runtime::PinDataType)
typedef enum BP_PinDataType {
    BP_PIN_UNKNOWN  = 0,
    BP_PIN_BOOLEAN  = 1,
    BP_PIN_INTEGER  = 2,
    BP_PIN_FLOAT    = 3,
    BP_PIN_STRING   = 4,
    BP_PIN_OBJECT   = 5,
    BP_PIN_ARRAY    = 6,
    BP_PIN_MAP      = 7,
    BP_PIN_SET      = 8,
    BP_PIN_ANY      = 9
} BP_PinDataType;

/// Descriptor for a single pin (input or output).
typedef struct BP_PinDef {
    const char*      name;       ///< Pin name (required)
    int              dataType;   ///< BP_PinDataType value; ignored when isExec=1
    int              isInput;    ///< 1 = Input pin, 0 = Output pin
    int              isExec;     ///< 1 = Flow (exec) pin, 0 = data pin
    const char*      tooltip;    ///< Optional tooltip (may be NULL)
} BP_PinDef;

/// Node handler callback type.
/// ctx:      execution context — use BP_GetInput*/BP_SetOutput* etc.
/// userdata: value passed to BP_RegisterHandler (e.g. GCHandle in C#).
/// Return 1 on success, 0 on failure.
typedef int (BLUEPRINT_CAPI_CALL *BP_HandlerFn)(BP_Context ctx, void* userdata);

/// Register a node definition dynamically (equivalent to Blueprint.RegisterNodeDef in Lua).
/// id/name/category/color may be NULL (name defaults to id, others default to empty).
/// pins is an array of pinCount BP_PinDef entries; may be NULL if pinCount==0.
/// Returns 0 on success, non-zero on failure.
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_RegisterNodeDef(
    BP_Runner       runner,
    const char*     id,
    const char*     name,
    const char*     category,
    const char*     color,
    BP_PinDef*      pins,
    int             pinCount
);

/// Unregister a previously registered node definition.
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_UnregisterNodeDef(
    BP_Runner runner, const char* id);

/// Returns 1 if a node def with the given id has been registered, 0 otherwise.
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_HasNodeDef(
    BP_Runner runner, const char* id);

/// Register a C handler for a node definition (equivalent to Blueprint.RegisterHandler in Lua).
/// fn is called each time a node of this type executes.
/// userdata is an arbitrary pointer forwarded to fn (e.g. a GCHandle.ToIntPtr() in C#).
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_RegisterHandler(
    BP_Runner    runner,
    const char*  definitionId,
    BP_HandlerFn fn,
    void*        userdata
);

/// Unregister a handler registered with BP_RegisterHandler.
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_UnregisterHandler(
    BP_Runner runner, const char* definitionId);

// ---------------------------------------------------------------------------
// ExecutionContext accessors  (valid only inside BP_HandlerFn)
// ---------------------------------------------------------------------------

// --- Input ---
BLUEPRINT_CAPI_EXPORT int64_t BLUEPRINT_CAPI_CALL BP_GetInputInt   (BP_Context ctx, const char* pin);
BLUEPRINT_CAPI_EXPORT double  BLUEPRINT_CAPI_CALL BP_GetInputFloat (BP_Context ctx, const char* pin);
BLUEPRINT_CAPI_EXPORT int     BLUEPRINT_CAPI_CALL BP_GetInputBool  (BP_Context ctx, const char* pin);
/// Copies the string value into buf. Returns bytes written (excl. NUL), or -1 if not found.
BLUEPRINT_CAPI_EXPORT int     BLUEPRINT_CAPI_CALL BP_GetInputString(BP_Context ctx, const char* pin,
                                                                     char* buf, int bufLen);

// --- Output ---
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetOutputInt   (BP_Context ctx, const char* pin, int64_t val);
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetOutputFloat (BP_Context ctx, const char* pin, double  val);
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetOutputBool  (BP_Context ctx, const char* pin, int     val);
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_SetOutputString(BP_Context ctx, const char* pin, const char* val);

// --- Control flow ---
/// Activate an output Flow pin (triggers connected nodes). Returns 1 on success.
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_ActivateOutputFlow(BP_Context ctx, const char* pin);

// --- Variables ---
BLUEPRINT_CAPI_EXPORT int64_t BLUEPRINT_CAPI_CALL BP_CtxGetVariableInt   (BP_Context ctx, const char* name);
BLUEPRINT_CAPI_EXPORT double  BLUEPRINT_CAPI_CALL BP_CtxGetVariableFloat (BP_Context ctx, const char* name);
BLUEPRINT_CAPI_EXPORT int     BLUEPRINT_CAPI_CALL BP_CtxGetVariableBool  (BP_Context ctx, const char* name);
BLUEPRINT_CAPI_EXPORT int     BLUEPRINT_CAPI_CALL BP_CtxGetVariableString(BP_Context ctx, const char* name,
                                                                           char* buf, int bufLen);
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_CtxSetVariableInt   (BP_Context ctx, const char* name, int64_t val);
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_CtxSetVariableFloat (BP_Context ctx, const char* name, double  val);
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_CtxSetVariableBool  (BP_Context ctx, const char* name, int     val);
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_CtxSetVariableString(BP_Context ctx, const char* name, const char* val);

// --- Logging / Print (from within a handler) ---
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_CtxLog     (BP_Context ctx, const char* msg);
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_CtxLogWarn (BP_Context ctx, const char* msg);
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_CtxLogError(BP_Context ctx, const char* msg);
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_CtxPrint   (BP_Context ctx, const char* msg);

// --- Current node info ---
BLUEPRINT_CAPI_EXPORT uint64_t BLUEPRINT_CAPI_CALL BP_CtxGetCurrentNodeId(BP_Context ctx);
/// Copies current node's definitionId into buf. Returns bytes written (excl. NUL).
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_CtxGetCurrentNodeDefId(BP_Context ctx,
                                                                          char* buf, int bufLen);
/// Copies the name of the activated input pin into buf.
BLUEPRINT_CAPI_EXPORT int BLUEPRINT_CAPI_CALL BP_CtxGetActivatedInputPin(BP_Context ctx,
                                                                           char* buf, int bufLen);

#ifdef __cplusplus
} // extern "C"
#endif
