// BlueprintRuntime.jslib
// Unity WebGL P/Invoke bridge for BlueprintRuntime.
//
// Place this file alongside libBlueprintRuntime.a in:
//   Assets/Plugins/WebGL/
//
// Unity merges this .jslib into the generated JS module at build time,
// exposing the BlueprintCAPI C functions to managed C# code via
// [DllImport("__Internal")] declarations.
//
// All functions here are thin wrappers that delegate to the compiled WASM
// symbols.  The real implementations live in libBlueprintRuntime.a (compiled
// by Emscripten from the C++ source).
//
// Note: BP_SetLogCallback / BP_SetPrintCallback take function pointers that
// cross the C#→WASM boundary.  Unity WebGL passes C# delegate pointers as
// integers; Emscripten's dynCall mechanism handles the actual invocation.
// On the C++ side the callback is stored and called via lua-style indirect
// call.  No extra JS glue is needed here because Emscripten already generates
// the dynCall tables.

var BlueprintRuntimeLib = {
    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------
    BP_CreateRunner: function() {
        return _BP_CreateRunner();
    },
    BP_DestroyRunner: function(runner) {
        _BP_DestroyRunner(runner);
    },

    // -----------------------------------------------------------------------
    // Load
    // -----------------------------------------------------------------------
    BP_LoadFromJson: function(runner, jsonPtr) {
        return _BP_LoadFromJson(runner, jsonPtr);
    },
    BP_LoadFromFile: function(runner, pathPtr) {
        return _BP_LoadFromFile(runner, pathPtr);
    },
    BP_IsLoaded: function(runner) {
        return _BP_IsLoaded(runner);
    },

    // -----------------------------------------------------------------------
    // Execution
    // -----------------------------------------------------------------------
    BP_Execute: function(runner) {
        return _BP_Execute(runner);
    },
    BP_ExecuteNode: function(runner, nodeId_lo, nodeId_hi) {
        // uint64 split into two i32 on the JS side
        var nodeId = nodeId_lo + nodeId_hi * 4294967296;
        return _BP_ExecuteNode(runner, nodeId);
    },
    BP_Tick: function(runner, deltaTime) {
        _BP_Tick(runner, deltaTime);
    },

    // -----------------------------------------------------------------------
    // Variables – set
    // -----------------------------------------------------------------------
    BP_SetVariableInt: function(runner, namePtr, value_lo, value_hi) {
        var value = value_lo + value_hi * 4294967296;
        _BP_SetVariableInt(runner, namePtr, value);
    },
    BP_SetVariableFloat: function(runner, namePtr, value) {
        _BP_SetVariableFloat(runner, namePtr, value);
    },
    BP_SetVariableString: function(runner, namePtr, valuePtr) {
        _BP_SetVariableString(runner, namePtr, valuePtr);
    },
    BP_SetVariableBool: function(runner, namePtr, value) {
        _BP_SetVariableBool(runner, namePtr, value);
    },

    // -----------------------------------------------------------------------
    // Variables – get
    // -----------------------------------------------------------------------
    BP_GetVariableInt: function(runner, namePtr) {
        return _BP_GetVariableInt(runner, namePtr);
    },
    BP_GetVariableFloat: function(runner, namePtr) {
        return _BP_GetVariableFloat(runner, namePtr);
    },
    BP_GetVariableBool: function(runner, namePtr) {
        return _BP_GetVariableBool(runner, namePtr);
    },
    BP_GetVariableString: function(runner, namePtr, bufPtr, bufLen) {
        return _BP_GetVariableString(runner, namePtr, bufPtr, bufLen);
    },

    // -----------------------------------------------------------------------
    // Logging
    // -----------------------------------------------------------------------
    BP_SetLogCallback: function(runner, callback) {
        _BP_SetLogCallback(runner, callback);
    },
    BP_EnableLogging: function(runner, enable) {
        _BP_EnableLogging(runner, enable);
    },
    BP_IsLoggingEnabled: function(runner) {
        return _BP_IsLoggingEnabled(runner);
    },
    BP_SetPrintCallback: function(runner, callback) {
        _BP_SetPrintCallback(runner, callback);
    },

    // -----------------------------------------------------------------------
    // Error handling
    // -----------------------------------------------------------------------
    BP_GetLastError: function(runner, bufPtr, bufLen) {
        return _BP_GetLastError(runner, bufPtr, bufLen);
    },
};

mergeInto(LibraryManager.library, BlueprintRuntimeLib);
