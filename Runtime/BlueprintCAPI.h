// Runtime/BlueprintCAPI.h - Plain C interface for Unity DllImport
//
// All functions use the BLUEPRINT_CAPI macro which resolves to:
//   Windows  → __declspec(dllexport) with cdecl calling convention
//   Linux    → visibility("default")
//   Emscripten → EMSCRIPTEN_KEEPALIVE
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
#   define BLUEPRINT_CAPI EMSCRIPTEN_KEEPALIVE
#elif defined(_WIN32) || defined(_WIN64)
#   define BLUEPRINT_CAPI __declspec(dllexport) __cdecl
#elif defined(__GNUC__) || defined(__clang__)
#   define BLUEPRINT_CAPI __attribute__((visibility("default")))
#else
#   define BLUEPRINT_CAPI
#endif

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
// Lifecycle
// ---------------------------------------------------------------------------

/// Create a new BlueprintRunner instance. Returns NULL on OOM.
BLUEPRINT_CAPI BP_Runner BP_CreateRunner(void);

/// Destroy a runner created with BP_CreateRunner. Safe to call with NULL.
BLUEPRINT_CAPI void BP_DestroyRunner(BP_Runner runner);

// ---------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------

/// Load blueprint from a JSON string.
/// Returns 0 on success, non-zero on failure (call BP_GetLastError for details).
BLUEPRINT_CAPI int BP_LoadFromJson(BP_Runner runner, const char* json);

/// Load blueprint from a file path (uses the runner's IFileSystem).
/// Returns 0 on success, non-zero on failure.
BLUEPRINT_CAPI int BP_LoadFromFile(BP_Runner runner, const char* filePath);

/// Returns 1 if a blueprint has been loaded successfully, 0 otherwise.
BLUEPRINT_CAPI int BP_IsLoaded(BP_Runner runner);

// ---------------------------------------------------------------------------
// Execution
// ---------------------------------------------------------------------------

/// Execute the blueprint from its entry point(s).
/// Returns 0 on success, non-zero on failure.
BLUEPRINT_CAPI int BP_Execute(BP_Runner runner);

/// Execute a single node by ID.
/// Returns 0 on success, non-zero on failure.
BLUEPRINT_CAPI int BP_ExecuteNode(BP_Runner runner, uint64_t nodeId);

/// Advance async timers (call once per frame with your deltaTime in seconds).
BLUEPRINT_CAPI void BP_Tick(BP_Runner runner, float deltaTime);

// ---------------------------------------------------------------------------
// Variables
// ---------------------------------------------------------------------------

BLUEPRINT_CAPI void   BP_SetVariableInt   (BP_Runner runner, const char* name, int64_t value);
BLUEPRINT_CAPI void   BP_SetVariableFloat (BP_Runner runner, const char* name, double value);
BLUEPRINT_CAPI void   BP_SetVariableString(BP_Runner runner, const char* name, const char* value);
BLUEPRINT_CAPI void   BP_SetVariableBool  (BP_Runner runner, const char* name, int value); // 0=false 1=true

BLUEPRINT_CAPI int64_t BP_GetVariableInt   (BP_Runner runner, const char* name);
BLUEPRINT_CAPI double  BP_GetVariableFloat (BP_Runner runner, const char* name);
BLUEPRINT_CAPI int     BP_GetVariableBool  (BP_Runner runner, const char* name); // returns 0 or 1

/// Copy the string variable into buf (at most bufLen-1 bytes + NUL).
/// Returns the number of bytes written (excluding NUL), or -1 if not found.
BLUEPRINT_CAPI int BP_GetVariableString(BP_Runner runner, const char* name, char* buf, int bufLen);

// ---------------------------------------------------------------------------
// Logging  (internal debug diagnostics – gated by BP_EnableLogging)
// ---------------------------------------------------------------------------

/// Register a callback that receives internal debug/diagnostic messages.
/// Only fires when logging is enabled (see BP_EnableLogging).
/// Pass NULL to clear.
BLUEPRINT_CAPI void BP_SetLogCallback(BP_Runner runner, BP_LogCallback callback);

/// Enable or disable internal debug logging.
/// Default: 0 (disabled) when built with NDEBUG (Release), 1 otherwise.
BLUEPRINT_CAPI void BP_EnableLogging(BP_Runner runner, int enable);

/// Returns 1 if internal logging is currently enabled, 0 otherwise.
BLUEPRINT_CAPI int BP_IsLoggingEnabled(BP_Runner runner);

// ---------------------------------------------------------------------------
// Print  (application-level output – PrintString / Log / FormatLog nodes)
// ---------------------------------------------------------------------------

/// Register a callback that receives output from PrintString / Log / FormatLog.
/// This is the channel that game engines (Unity, etc.) should override.
/// Independent of BP_EnableLogging – always fires when set.
/// If not set, output falls back to the log callback (if logging enabled)
/// and then to stderr.
/// Pass NULL to clear.
BLUEPRINT_CAPI void BP_SetPrintCallback(BP_Runner runner, BP_LogCallback callback);

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

/// Copy the last error message into buf (at most bufLen-1 bytes + NUL).
/// Returns the number of bytes written (excluding NUL).
BLUEPRINT_CAPI int BP_GetLastError(BP_Runner runner, char* buf, int bufLen);

#ifdef __cplusplus
} // extern "C"
#endif
