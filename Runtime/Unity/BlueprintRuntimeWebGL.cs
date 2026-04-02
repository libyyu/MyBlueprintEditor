// BlueprintRuntimeWebGL.cs
// Unity WebGL C# P/Invoke bindings for BlueprintRuntime.
//
// For non-WebGL platforms, use the standard BlueprintRuntime.cs instead
// (which uses the regular DllImport with the library name).
//
// Usage:
//   Place BlueprintRuntime.jslib + libBlueprintRuntime.a in Assets/Plugins/WebGL/
//   Use #if UNITY_WEBGL && !UNITY_EDITOR guards in your game code.

#if UNITY_WEBGL && !UNITY_EDITOR
using System.Runtime.InteropServices;

public static class BlueprintRuntime
{
    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------
    [DllImport("__Internal")] public static extern System.IntPtr BP_CreateRunner();
    [DllImport("__Internal")] public static extern void BP_DestroyRunner(System.IntPtr runner);

    // -----------------------------------------------------------------------
    // Load
    // -----------------------------------------------------------------------
    [DllImport("__Internal")] public static extern int BP_LoadFromJson(System.IntPtr runner, string json);
    [DllImport("__Internal")] public static extern int BP_LoadFromFile(System.IntPtr runner, string path);
    [DllImport("__Internal")] public static extern int BP_IsLoaded(System.IntPtr runner);

    // -----------------------------------------------------------------------
    // Execution
    // -----------------------------------------------------------------------
    [DllImport("__Internal")] public static extern int  BP_Execute(System.IntPtr runner);
    [DllImport("__Internal")] public static extern int  BP_ExecuteNode(System.IntPtr runner, ulong nodeId);
    [DllImport("__Internal")] public static extern void BP_Tick(System.IntPtr runner, float deltaTime);

    // -----------------------------------------------------------------------
    // Variables
    // -----------------------------------------------------------------------
    [DllImport("__Internal")] public static extern void BP_SetVariableInt   (System.IntPtr runner, string name, long value);
    [DllImport("__Internal")] public static extern void BP_SetVariableFloat (System.IntPtr runner, string name, double value);
    [DllImport("__Internal")] public static extern void BP_SetVariableString(System.IntPtr runner, string name, string value);
    [DllImport("__Internal")] public static extern void BP_SetVariableBool  (System.IntPtr runner, string name, int value);

    [DllImport("__Internal")] public static extern long   BP_GetVariableInt   (System.IntPtr runner, string name);
    [DllImport("__Internal")] public static extern double BP_GetVariableFloat (System.IntPtr runner, string name);
    [DllImport("__Internal")] public static extern int    BP_GetVariableBool  (System.IntPtr runner, string name);
    [DllImport("__Internal")] public static extern int    BP_GetVariableString(System.IntPtr runner, string name, byte[] buf, int bufLen);

    // -----------------------------------------------------------------------
    // Logging / Error
    // -----------------------------------------------------------------------
    [DllImport("__Internal")] public static extern void BP_EnableLogging    (System.IntPtr runner, int enable);
    [DllImport("__Internal")] public static extern int  BP_IsLoggingEnabled (System.IntPtr runner);
    [DllImport("__Internal")] public static extern int  BP_GetLastError     (System.IntPtr runner, byte[] buf, int bufLen);
}
#endif
