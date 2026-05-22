// BlueprintRuntime.cs - Unity C# bindings for BlueprintRuntime C API
//
// Platform dispatch:
//   iOS / WebGL  → static link, DLL name = "__Internal"
//   Android      → libBlueprintRuntime.so
//   Windows      → BlueprintRuntime.dll
//   macOS/Linux  → libBlueprintRuntime.so / libBlueprintRuntime.dylib
//
// Usage example:
//
//   using BlueprintRuntime;
//
//   var runner = new BPRunner();
//   runner.LoadFromJson(jsonText);
//   runner.SetVariable("Health", 100L);
//   runner.Execute();
//   float dmg = (float)runner.GetVariableFloat("Damage");
//   runner.Tick(Time.deltaTime);
//   runner.Dispose();   // or use `using var runner = new BPRunner();`
//
// Script node definition + handler (equivalent to Lua Blueprint.RegisterNodeDef / RegisterHandler):
//
//   runner.RegisterNodeDef(new BPNodeDef {
//       id       = "MyAdd",
//       name     = "My Add",
//       category = "Custom/Math",
//       pins = new[] {
//           new BPPinDef { name="A",      dataType=BPPinType.Float, isInput=true },
//           new BPPinDef { name="B",      dataType=BPPinType.Float, isInput=true },
//           new BPPinDef { name="Result", dataType=BPPinType.Float, isInput=false },
//       }
//   });
//
//   runner.RegisterHandler("MyAdd", ctx => {
//       float a = (float)ctx.GetInputFloat("A");
//       float b = (float)ctx.GetInputFloat("B");
//       ctx.SetOutputFloat("Result", a + b);
//       return true;
//   });

using CutRope.Framework;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using UnityEngine;
using static PlasticGui.PlasticTableColumn;

namespace BlueprintRuntime
{
    // =========================================================================
    // Log level (mirrors Runtime::LogLevel in C++)
    // =========================================================================

    public enum BPLogLevel : int
    {
        Verbose = 0,   // Detailed trace (internal debug)
        Info    = 1,   // Normal output (PrintString default)
        Warning = 2,   // Non-fatal issues
        Error   = 3,   // Fatal / assertion failures
    }

    // =========================================================================
    // Pin data type (mirrors Runtime::PinDataType)
    // =========================================================================

    public enum BPPinType : int
    {
        Unknown = 0,
        Boolean = 1,
        Integer = 2,
        Float   = 3,
        String  = 4,
        Object  = 5,
        Array   = 6,
        Map     = 7,
        Set     = 8,
        Any     = 9,
    }

    // =========================================================================
    // Pin descriptor (used with RegisterNodeDef)
    // =========================================================================

    public sealed class BPPinDef
    {
        /// <summary>Pin name (required).</summary>
        public string   name;

        /// <summary>Data type. Ignored for Flow (exec) pins.</summary>
        public BPPinType dataType = BPPinType.Any;

        /// <summary>true = Input pin, false = Output pin.</summary>
        public bool isInput = true;

        /// <summary>true = Flow (exec) pin; dataType is ignored.</summary>
        public bool isExec = false;

        /// <summary>Optional tooltip shown in the editor.</summary>
        public string tooltip;
    }

    // =========================================================================
    // Node definition descriptor (used with RegisterNodeDef)
    // =========================================================================

    public sealed class BPNodeDef
    {
        /// <summary>Unique node type ID (required, must match definitionId in bjson).</summary>
        public string id;

        /// <summary>Display name shown in the editor. Defaults to id if null.</summary>
        public string name;

        /// <summary>Category path, e.g. "Custom/Math". Determines editor palette position.</summary>
        public string category;

        /// <summary>Header colour in RRGGBB hex, e.g. "FF6600". Optional.</summary>
        public string color;

        /// <summary>Tooltip shown in the editor. Optional.</summary>
        public string description;

        /// <summary>All input and output pins for this node.</summary>
        public BPPinDef[] pins;
    }

    // =========================================================================
    // Execution context  (passed to RegisterHandler callbacks)
    // =========================================================================

    /// <summary>
    /// Provides access to pin values, variables, control flow and logging
    /// from within a node handler callback. Only valid during the callback.
    /// </summary>
    public sealed class BPContext
    {
        private readonly IntPtr _ctx;

        internal BPContext(IntPtr ctx) { _ctx = ctx; }

        // --- Input ---
        public long   GetInputLong   (string pin) => Native.BP_GetInputInt   (_ctx, pin);
        public double GetInputFloat (string pin) => Native.BP_GetInputFloat (_ctx, pin);
        public bool   GetInputBool  (string pin) => Native.BP_GetInputBool  (_ctx, pin) != 0;
        public string GetInputString(string pin)
        {
            IntPtr buf = Marshal.AllocHGlobal(Native.StringBufSize);
            try
            {
                int n = Native.BP_GetInputString(_ctx, pin, buf, Native.StringBufSize);
                return n >= 0 ? Marshal.PtrToStringUTF8(buf, n) : null;
            }
            finally { Marshal.FreeHGlobal(buf); }
        }
        public int GetInputInt (string pin)
        {
            long val = GetInputLong(pin);
            if (val < int.MinValue || val > int.MaxValue)
                throw new OverflowException($"Input '{pin}' value {val} overflows int");
            return (int)val;
        }

        // --- Output ---
        public void SetOutputLong   (string pin, long   val) => Native.BP_SetOutputInt   (_ctx, pin, val);
        public void SetOutputFloat (string pin, double val) => Native.BP_SetOutputFloat (_ctx, pin, val);
        public void SetOutputBool  (string pin, bool   val) => Native.BP_SetOutputBool  (_ctx, pin, val ? 1 : 0);
        public void SetOutputString(string pin, string val) => Native.BP_SetOutputString(_ctx, pin, val ?? "");

        public void SetOutputInt(string pin, int val)
        {
            long longVal = val;
            SetOutputLong(pin, longVal);
        }

        // Convenience float overload
        public void SetOutputFloat(string pin, float val) => SetOutputFloat(pin, (double)val);

        // --- Control flow ---
        /// <summary>Activate an output Flow pin (triggers connected nodes).</summary>
        public bool ActivateOutputFlow(string pin) => Native.BP_ActivateOutputFlow(_ctx, pin) != 0;

        // --- Variables ---
        public long   GetVariableLong   (string name) => Native.BP_CtxGetVariableInt   (_ctx, name);
        public double GetVariableFloat (string name) => Native.BP_CtxGetVariableFloat (_ctx, name);
        public bool   GetVariableBool  (string name) => Native.BP_CtxGetVariableBool  (_ctx, name) != 0;
        public string GetVariableString(string name)
        {
            IntPtr buf = Marshal.AllocHGlobal(Native.StringBufSize);
            try
            {
                int n = Native.BP_CtxGetVariableString(_ctx, name, buf, Native.StringBufSize);
                return n >= 0 ? Marshal.PtrToStringUTF8(buf, n) : null;
            }
            finally { Marshal.FreeHGlobal(buf); }
        }
        public int GetVariableInt(string name)
        {
            long val = GetVariableLong(name);
            if (val < int.MinValue || val > int.MaxValue)
                throw new OverflowException($"Input '{name}' value {val} overflows int");
            return (int)val;
        }

        public void SetVariableLong   (string name, long   val) => Native.BP_CtxSetVariableInt   (_ctx, name, val);
        public void SetVariableFloat (string name, double val) => Native.BP_CtxSetVariableFloat (_ctx, name, val);
        public void SetVariableBool  (string name, bool   val) => Native.BP_CtxSetVariableBool  (_ctx, name, val ? 1 : 0);
        public void SetVariableString(string name, string val) => Native.BP_CtxSetVariableString(_ctx, name, val ?? "");
        public void SetVariableInt(string name, int val) => SetVariableLong(name, (long)val);
        // --- Logging / Print ---
        public void Log     (string msg) => Native.BP_CtxLog     (_ctx, msg);
        public void LogWarn (string msg) => Native.BP_CtxLogWarn (_ctx, msg);
        public void LogError(string msg) => Native.BP_CtxLogError(_ctx, msg);
        public void Print   (string msg) => Native.BP_CtxPrint   (_ctx, msg);

        // --- Current node info ---
        public ulong CurrentNodeId => Native.BP_CtxGetCurrentNodeId(_ctx);
        public string CurrentNodeDefId
        {
            get
            {
                IntPtr buf = Marshal.AllocHGlobal(Native.StringBufSize);
                try
                {
                    int n = Native.BP_CtxGetCurrentNodeDefId(_ctx, buf, Native.StringBufSize);
                    return n > 0 ? Marshal.PtrToStringUTF8(buf, n) : "";
                }
                finally { Marshal.FreeHGlobal(buf); }
            }
        }
        public string ActivatedInputPin
        {
            get
            {
                IntPtr buf = Marshal.AllocHGlobal(Native.StringBufSize);
                try
                {
                    int n = Native.BP_CtxGetActivatedInputPin(_ctx, buf, Native.StringBufSize);
                    return n > 0 ? Marshal.PtrToStringUTF8(buf, n) : "";
                }
                finally { Marshal.FreeHGlobal(buf); }
            }
        }
    }

    // =========================================================================
    // Platform-specific DLL name selection
    // =========================================================================

    internal static class NativeLib
    {
#if (UNITY_IPHONE || UNITY_TVOS || UNITY_WEBGL || UNITY_SWITCH) && !UNITY_EDITOR
        internal const string DLL = "__Internal";
#else
        internal const string DLL = "BlueprintRuntime";
#endif
    }

    // =========================================================================
    // Raw P/Invoke declarations (internal – use BPRunner instead)
    // =========================================================================

    internal static class Native
    {
        internal const int StringBufSize = 4096;

        // --- Delegate types ---
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void LogCallbackDelegate(BPLogLevel level,
            [MarshalAs(UnmanagedType.LPStr)] string message);

        /// <summary>Node handler callback: ctx=execution context, userdata=GCHandle ptr. Return 1=ok, 0=fail.</summary>
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int HandlerDelegate(IntPtr ctx, IntPtr userdata);

        // --- Lifecycle ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr BP_CreateRunner();

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_DestroyRunner(IntPtr runner);

        // --- Global HTTP client (shared across runners, needed by LLM.* nodes) ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_InitDefaultHttpClient();

        // --- Loading ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_LoadFromJson(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string json);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_LoadFromJsonWithBaseDir(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string json,
            [MarshalAs(UnmanagedType.LPStr)] string baseDir);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_LoadFromFile(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string filePath);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_SetBasePath(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string basePath);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_IsLoaded(IntPtr runner);

        // --- Lua extension (optional; calls are no-ops if built without Lua) ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_LoadLuaScript(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string filePath);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_SetGlobalLuaEntry(
            [MarshalAs(UnmanagedType.LPStr)] string filePath);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_GetGlobalLuaEntry(IntPtr buf, int bufLen);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_LoadGlobalLuaEntry(IntPtr runner);

        // --- Execution ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_Execute(IntPtr runner);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_ExecuteAll(IntPtr runner);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_DispatchEvent(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string eventDefinitionId);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_ExecuteNode(IntPtr runner, ulong nodeId);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_Tick(IntPtr runner, float deltaTime);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_GetActiveTimerCount(IntPtr runner);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_HasPendingWork(IntPtr runner);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_DrainQueue();

        // --- Variables: Set ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_SetVariableInt(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string name, long value);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_SetVariableFloat(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string name, double value);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_SetVariableString(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string name,
            [MarshalAs(UnmanagedType.LPStr)] string value);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_SetVariableBool(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string name, int value);

        // --- Variables: Get ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern long BP_GetVariableInt(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string name);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern double BP_GetVariableFloat(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string name);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_GetVariableBool(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string name);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_GetVariableString(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string name,
            IntPtr buf, int bufLen);

        // --- Logging ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_SetLogCallback(IntPtr runner, LogCallbackDelegate callback);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_EnableLogging(IntPtr runner, int enable);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_IsLoggingEnabled(IntPtr runner);

        // --- Print ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_SetPrintCallback(IntPtr runner, LogCallbackDelegate callback);

        // --- Error ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_GetLastError(IntPtr runner, IntPtr buf, int bufLen);

        // --- Script node def registration ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_RegisterNodeDef(
            IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string id,
            [MarshalAs(UnmanagedType.LPStr)] string name,
            [MarshalAs(UnmanagedType.LPStr)] string category,
            [MarshalAs(UnmanagedType.LPStr)] string color,
            IntPtr pins,   // BP_PinDef* (blittable struct array)
            int    pinCount);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_UnregisterNodeDef(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string id);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_HasNodeDef(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string id);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_RegisterHandler(
            IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string definitionId,
            HandlerDelegate fn,
            IntPtr userdata);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_UnregisterHandler(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string definitionId);

        // --- ExecutionContext accessors ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern long BP_GetInputInt(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string pin);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern double BP_GetInputFloat(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string pin);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_GetInputBool(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string pin);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_GetInputString(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string pin,
            IntPtr buf, int bufLen);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_SetOutputInt(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string pin, long val);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_SetOutputFloat(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string pin, double val);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_SetOutputBool(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string pin, int val);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_SetOutputString(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string pin,
            [MarshalAs(UnmanagedType.LPStr)] string val);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_ActivateOutputFlow(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string pin);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern long BP_CtxGetVariableInt(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string name);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern double BP_CtxGetVariableFloat(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string name);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_CtxGetVariableBool(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string name);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_CtxGetVariableString(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string name,
            IntPtr buf, int bufLen);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_CtxSetVariableInt(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string name, long val);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_CtxSetVariableFloat(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string name, double val);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_CtxSetVariableBool(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string name, int val);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_CtxSetVariableString(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string name,
            [MarshalAs(UnmanagedType.LPStr)] string val);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_CtxLog(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string msg);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_CtxLogWarn(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string msg);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_CtxLogError(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string msg);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_CtxPrint(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string msg);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern ulong BP_CtxGetCurrentNodeId(IntPtr ctx);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_CtxGetCurrentNodeDefId(IntPtr ctx, IntPtr buf, int bufLen);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_CtxGetActivatedInputPin(IntPtr ctx, IntPtr buf, int bufLen);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_SetCrashDumpDir(
            [MarshalAs(UnmanagedType.LPStr)] string dir);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_WriteCrashDump(
            [MarshalAs(UnmanagedType.LPStr)] string reason);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)] 
        public static extern void BP_SetExternalLuaState(IntPtr runner, IntPtr L);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr BP_GetLuaState(IntPtr runner);

        /// <summary>xLua 主 VM 即将 Dispose 前调用，通知 BR 清空 m_L 避免野指针访问</summary>
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_NotifyLuaStateClosing(IntPtr L);

        // ----------------------------------------------------------------
        // Metadata / Dependency query
        // ----------------------------------------------------------------
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr BP_MetaParseFromJson([MarshalAs(UnmanagedType.LPUTF8Str)] string json);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_MetaFree(IntPtr meta);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_MetaGetDependencyCount(IntPtr meta);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_MetaGetDependency(IntPtr meta, int index, IntPtr buf, int bufLen);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_MetaGetName(IntPtr meta, IntPtr buf, int bufLen);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_MetaGetVersion(IntPtr meta, IntPtr buf, int bufLen);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_MetaGetBlueprintClass(IntPtr meta);
        // Blittable mirror of C BP_PinDef — layout must match exactly.
        // strings are passed as null-terminated UTF-8 pointers (IntPtr).
        // We pin the string bytes ourselves in BPRunner.RegisterNodeDef.
        [StructLayout(LayoutKind.Sequential)]
        public struct NativePinDef
        {
            public IntPtr name;      // const char*
            public int    dataType;  // BP_PinDataType
            public int    isInput;   // int
            public int    isExec;    // int
            public IntPtr tooltip;   // const char*
        }
    }

    // =========================================================================
    // High-level wrapper  (recommended)
    // =========================================================================

    /// <summary>
    /// Managed wrapper around a native BlueprintRunner.
    /// Implements IDisposable – use with `using` or call Dispose() explicitly.
    /// </summary>
    public sealed class BPRunner : IDisposable
    {
        private const int StringBufSize = Native.StringBufSize;

        private IntPtr _handle;
        private bool   _disposed;

        // Strong refs to delegates to prevent GC collection while native side holds them.
        private Native.LogCallbackDelegate _logDelegate;
        private Native.LogCallbackDelegate _printDelegate;

        // Handler book-keeping: keep delegate + GCHandle alive until unregistered.
        private readonly Dictionary<string, HandlerEntry> _handlers
            = new Dictionary<string, HandlerEntry>();

        private sealed class HandlerEntry
        {
            public Native.HandlerDelegate NativeDelegate;  // strong ref
            public GCHandle               GCHandle;        // for managed callback closure
        }

        /// <summary>Fired for internal debug/diagnostic messages (only when logging enabled).</summary>
        public event Action<BPLogLevel, string> OnLog;

        /// <summary>Fired by PrintString / Log / FormatLog nodes.</summary>
        public event Action<BPLogLevel, string> OnPrint;

        // ---------------------------------------------------------------------
        // Construction / disposal
        // ---------------------------------------------------------------------

        internal BPRunner()
        {
            _handle = Native.BP_CreateRunner();
            if (_handle == IntPtr.Zero)
                throw new InvalidOperationException("BP_CreateRunner returned null");

            _logDelegate   = (lv, msg) => OnLog?.Invoke(lv, msg);
            _printDelegate = (lv, msg) => OnPrint?.Invoke(lv, msg);
            Native.BP_SetLogCallback(_handle, _logDelegate);
            Native.BP_SetPrintCallback(_handle, _printDelegate);
        }

        public void Dispose()
        {
            if (!_disposed)
            {
                if (_handle != IntPtr.Zero)
                {
                    Native.BP_SetLogCallback(_handle, null);
                    Native.BP_SetPrintCallback(_handle, null);
                    // 不在单个 Runner.Dispose 里调 NotifyLuaStateClosing：
                    // 共享 VM 尚被其他 Runner 使用，这里调会把 m_L 清空导致其他 Runner Tick 失败。
                    // NotifyLuaStateClosing 展层由 BlueprintService.ReleaseAllRunner 在 VM 整体关闭前调用。
                    Native.BP_DestroyRunner(_handle);
                    _handle = IntPtr.Zero;
                }
                // Release all handler GCHandles
                foreach (var entry in _handlers.Values)
                {
                    if (entry.GCHandle.IsAllocated)
                        entry.GCHandle.Free();
                }
                _handlers.Clear();
                _logDelegate   = null;
                _printDelegate = null;
                _disposed = true;
            }
        }

        // ---------------------------------------------------------------------
        // Loading
        // ---------------------------------------------------------------------

        public void LoadFromJson(string json)
        {
            ThrowIfDisposed();
            if (Native.BP_LoadFromJson(_handle, json) != 0)
                throw new BPException("LoadFromJson failed: " + GetLastError());
        }

        /// <summary>Load blueprint JSON with explicit base directory for resolving
        /// relative file paths (e.g. embedded Lua scripts).</summary>
        public void LoadFromJson(string json, string baseDir)
        {
            ThrowIfDisposed();
            if (Native.BP_LoadFromJsonWithBaseDir(_handle, json, baseDir ?? "") != 0)
                throw new BPException("LoadFromJsonWithBaseDir failed: " + GetLastError());
        }

        public void LoadFromFile(string filePath)
        {
            ThrowIfDisposed();
            if (Native.BP_LoadFromFile(_handle, filePath) != 0)
                throw new BPException("LoadFromFile failed: " + GetLastError());
        }

        /// <summary>Set base directory for resolving relative file paths.
        /// Useful when loading a blueprint from memory (e.g. TextAsset) that
        /// references other files by relative path.</summary>
        public void SetBasePath(string basePath)
        {
            ThrowIfDisposed();
            Native.BP_SetBasePath(_handle, basePath ?? "");
        }

        public bool IsLoaded
        {
            get { ThrowIfDisposed(); return Native.BP_IsLoaded(_handle) != 0; }
        }

        // ---------------------------------------------------------------------
        // Lua extension (optional — these are no-ops if Runtime built without Lua)
        // ---------------------------------------------------------------------

        /// <summary>Load a Lua extension script (registers node defs / handlers at runtime).
        /// No-op if Runtime was built without BLUEPRINT_HAS_LUA.</summary>
        /// <returns>true on success</returns>
        public bool LoadLuaScript(string filePath)
        {
            ThrowIfDisposed();
            return Native.BP_LoadLuaScript(_handle, filePath) == 0;
        }

        /// <summary>Load the globally-configured Lua entry script into this runner.</summary>
        public bool LoadGlobalLuaEntry()
        {
            ThrowIfDisposed();
            return Native.BP_LoadGlobalLuaEntry(_handle) == 0;
        }

        public IntPtr GetLuaState()
        {
            ThrowIfDisposed();
            return Native.BP_GetLuaState(_handle);
        }

        /// <summary>
        /// 将外部 lua_State（如 xLua 的 VM）注入给 BlueprintRuntime。
        /// 注入后 BR 共享该 VM，不拥有生命周期（不会 lua_close）。
        /// 必须在任何 LoadLuaScript / Execute 之前调用。
        /// </summary>
        /// <summary>
        /// xLua 主 VM 即将 Dispose 前调用。
        /// 通知 BlueprintRuntime 清空内部 lua_State 指针，避免后续 Tick 访问野指针崩溃。
        /// </summary>
        public void NotifyLuaStateClosing()
        {
            ThrowIfDisposed();
            var L = Native.BP_GetLuaState(_handle);
            if (L != IntPtr.Zero)
                Native.BP_NotifyLuaStateClosing(L);
        }

        public void SetExternalLuaState(IntPtr L)
        {
            ThrowIfDisposed();
            Native.BP_SetExternalLuaState(_handle, L);
        }

        // ---------------------------------------------------------------------
        // Execution
        // ---------------------------------------------------------------------

        /// <summary>Execute the blueprint: data-flow evaluation + dispatch OnBeginPlay event.
        /// This is the standard way to run a blueprint. Equivalent to ExecuteDataFlow() + DispatchEvent("OnBeginPlay").</summary>
        public void Execute()
        {
            ThrowIfDisposed();
            if (Native.BP_ExecuteAll(_handle) != 0)
                throw new BPException("Execute failed: " + GetLastError());
        }

        /// <summary>Execute only the data-flow topological evaluation (no event dispatch).
        /// Use this when you need fine-grained control and will call DispatchEvent() separately.</summary>
        public void ExecuteDataFlow()
        {
            ThrowIfDisposed();
            if (Native.BP_Execute(_handle) != 0)
                throw new BPException("ExecuteDataFlow failed: " + GetLastError());
        }

        /// <summary>Dispatch a named event (triggers all OnEvent nodes matching the id).
        /// Return value: number of handlers fired (≥0) or negative on error.</summary>
        public int DispatchEvent(string eventDefinitionId)
        {
            ThrowIfDisposed();
            return Native.BP_DispatchEvent(_handle, eventDefinitionId ?? "");
        }

        public void ExecuteNode(ulong nodeId)
        {
            ThrowIfDisposed();
            if (Native.BP_ExecuteNode(_handle, nodeId) != 0)
                throw new BPException("ExecuteNode failed: " + GetLastError());
        }

        public void Tick(float deltaTime)
        {
            ThrowIfDisposed();
            Native.BP_Tick(_handle, deltaTime);
        }

        /// <summary>Number of currently-active timers (SetTimer / DelayedCall).</summary>
        public int ActiveTimerCount
        {
            get { ThrowIfDisposed(); return Native.BP_GetActiveTimerCount(_handle); }
        }

        /// <summary>True if there are pending async tasks (HTTP / timers / stream chunks).
        /// Cheap to query — use it to skip Tick() when idle, saving CPU.</summary>
        public bool HasPendingWork
        {
            get { ThrowIfDisposed(); return Native.BP_HasPendingWork(_handle) != 0; }
        }

        // ---------------------------------------------------------------------
        // Variables – set
        // ---------------------------------------------------------------------

        public void SetVariable(string name, long   value) { ThrowIfDisposed(); Native.BP_SetVariableInt   (_handle, name, value); }
        public void SetVariable(string name, double value) { ThrowIfDisposed(); Native.BP_SetVariableFloat (_handle, name, value); }
        public void SetVariable(string name, float  value) => SetVariable(name, (double)value);
        public void SetVariable(string name, int    value) => SetVariable(name, (long)value);
        public void SetVariable(string name, bool   value) { ThrowIfDisposed(); Native.BP_SetVariableBool  (_handle, name, value ? 1 : 0); }
        public void SetVariable(string name, string value) { ThrowIfDisposed(); Native.BP_SetVariableString(_handle, name, value ?? ""); }

        // ---------------------------------------------------------------------
        // Variables – get
        // ---------------------------------------------------------------------

        public long   GetVariableInt   (string name) { ThrowIfDisposed(); return Native.BP_GetVariableInt   (_handle, name); }
        public double GetVariableFloat (string name) { ThrowIfDisposed(); return Native.BP_GetVariableFloat (_handle, name); }
        public bool   GetVariableBool  (string name) { ThrowIfDisposed(); return Native.BP_GetVariableBool  (_handle, name) != 0; }

        public string GetVariableString(string name)
        {
            ThrowIfDisposed();
            IntPtr buf = Marshal.AllocHGlobal(StringBufSize);
            try
            {
                int n = Native.BP_GetVariableString(_handle, name, buf, StringBufSize);
                return n >= 0 ? Marshal.PtrToStringUTF8(buf, n) : null;
            }
            finally { Marshal.FreeHGlobal(buf); }
        }

        // ---------------------------------------------------------------------
        // Script node definition registration
        // (equivalent to Blueprint.RegisterNodeDef in Lua)
        // ---------------------------------------------------------------------

        /// <summary>
        /// Register a custom node definition so the editor can display it
        /// and the runtime can execute it (when a matching handler is also registered).
        /// Equivalent to Blueprint.RegisterNodeDef() in Lua.
        /// </summary>
        public void RegisterNodeDef(BPNodeDef def)
        {
            ThrowIfDisposed();
            if (def == null || string.IsNullOrEmpty(def.id))
                throw new ArgumentException("BPNodeDef.id is required");

            var pins = def.pins ?? Array.Empty<BPPinDef>();

            // Build blittable array + pin native strings in one allocation pass.
            // We use byte arrays pinned for the duration of the P/Invoke call.
            var nativePins  = new Native.NativePinDef[pins.Length == 0 ? 1 : pins.Length];
            var pinnedBytes = new List<GCHandle>();

            try
            {
                for (int i = 0; i < pins.Length; ++i)
                {
                    var p = pins[i];
                    nativePins[i].dataType = (int)p.dataType;
                    nativePins[i].isInput  = p.isInput ? 1 : 0;
                    nativePins[i].isExec   = p.isExec  ? 1 : 0;

                    // Pin UTF-8 byte arrays for name and tooltip
                    nativePins[i].name    = PinString(p.name    ?? "", pinnedBytes);
                    nativePins[i].tooltip = PinString(p.tooltip ?? "", pinnedBytes);
                }

                var hPins = GCHandle.Alloc(nativePins, GCHandleType.Pinned);
                pinnedBytes.Add(hPins);

                int result = Native.BP_RegisterNodeDef(
                    _handle,
                    def.id,
                    def.name,
                    def.category,
                    def.color,
                    pins.Length > 0 ? hPins.AddrOfPinnedObject() : IntPtr.Zero,
                    pins.Length);

                if (result != 0)
                    throw new BPException("RegisterNodeDef failed for id: " + def.id);
            }
            finally
            {
                foreach (var h in pinnedBytes)
                    if (h.IsAllocated) h.Free();
            }
        }

        /// <summary>Unregister a previously registered node definition.</summary>
        public void UnregisterNodeDef(string id)
        {
            ThrowIfDisposed();
            Native.BP_UnregisterNodeDef(_handle, id);
        }

        /// <summary>Returns true if a node definition with this id has been registered.</summary>
        public bool HasNodeDef(string id)
        {
            ThrowIfDisposed();
            return Native.BP_HasNodeDef(_handle, id) != 0;
        }

        // ---------------------------------------------------------------------
        // Handler registration
        // (equivalent to Blueprint.RegisterHandler in Lua)
        // ---------------------------------------------------------------------

        /// <summary>
        /// Register a C# delegate as the execution handler for a node type.
        /// The delegate receives a BPContext and returns true on success.
        /// Equivalent to Blueprint.RegisterHandler() in Lua.
        /// </summary>
        public void RegisterHandler(string definitionId, Func<BPContext, bool> handler)
        {
            ThrowIfDisposed();
            if (string.IsNullOrEmpty(definitionId)) throw new ArgumentNullException(nameof(definitionId));
            if (handler == null) throw new ArgumentNullException(nameof(handler));

            // If a handler already exists under this id, release its GCHandle first.
            if (_handlers.TryGetValue(definitionId, out var old))
            {
                if (old.GCHandle.IsAllocated) old.GCHandle.Free();
                _handlers.Remove(definitionId);
            }

            // Box the managed delegate so the GCHandle keeps it alive.
            var entry = new HandlerEntry();
            entry.GCHandle = GCHandle.Alloc(handler);

            entry.NativeDelegate = (ctx, _userdata) =>
            {
                try
                {
                    var bpCtx = new BPContext(ctx);
                    return handler(bpCtx) ? 1 : 0;
                }
                catch (Exception ex)
                {
                    Debug.LogError($"[Blueprint] Handler '{definitionId}' threw: {ex}");
                    return 0;
                }
            };

            _handlers[definitionId] = entry;

            Native.BP_RegisterHandler(_handle, definitionId, entry.NativeDelegate, IntPtr.Zero);
        }

        /// <summary>Unregister a handler registered with RegisterHandler.</summary>
        public void UnregisterHandler(string definitionId)
        {
            ThrowIfDisposed();
            Native.BP_UnregisterHandler(_handle, definitionId);
            if (_handlers.TryGetValue(definitionId, out var entry))
            {
                if (entry.GCHandle.IsAllocated) entry.GCHandle.Free();
                _handlers.Remove(definitionId);
            }
        }

        // ---------------------------------------------------------------------
        // Logging control
        // ---------------------------------------------------------------------

        public void EnableLogging(bool enable)
        {
            ThrowIfDisposed();
            Native.BP_EnableLogging(_handle, enable ? 1 : 0);
        }

        public bool IsLoggingEnabled
        {
            get { ThrowIfDisposed(); return Native.BP_IsLoggingEnabled(_handle) != 0; }
        }

        // ---------------------------------------------------------------------
        // Error
        // ---------------------------------------------------------------------

        public string GetLastError()
        {
            if (_handle == IntPtr.Zero) return "(no runner)";
            IntPtr buf = Marshal.AllocHGlobal(StringBufSize);
            try
            {
                int n = Native.BP_GetLastError(_handle, buf, StringBufSize);
                return n > 0 ? Marshal.PtrToStringUTF8(buf, n) : "";
            }
            finally { Marshal.FreeHGlobal(buf); }
        }

        // ---------------------------------------------------------------------
        // Metadata / Dependency query  (static — no runner needed)
        // ---------------------------------------------------------------------

        /// <summary>
        /// Parse metadata from a bjson string and return a <see cref="BPBlueprintMeta"/> RAII wrapper.
        /// Returns null if parsing fails. Caller must Dispose() the result (or use a using block).
        /// </summary>
        public static BPBlueprintMeta ParseMetaFromJson(string json)
        {
            if (string.IsNullOrEmpty(json)) return null;
            var handle = Native.BP_MetaParseFromJson(json);
            return handle == IntPtr.Zero ? null : new BPBlueprintMeta(handle);
        }

        private void ThrowIfDisposed()
        {
            if (_disposed) throw new ObjectDisposedException(nameof(BPRunner));
        }

        /// <summary>
        /// Convert a string to null-terminated UTF-8 bytes, pin them, add the handle
        /// to the cleanup list, and return a pointer to the first byte.
        /// </summary>
        private static IntPtr PinString(string s, List<GCHandle> handles)
        {
            byte[] bytes = Encoding.UTF8.GetBytes(s + '\0');
            var h = GCHandle.Alloc(bytes, GCHandleType.Pinned);
            handles.Add(h);
            return h.AddrOfPinnedObject();
        }

        // =====================================================================
        // Global (static) API — shared across all runners
        // =====================================================================

        /// <summary>Set dump output directory (UTF-8 path). Call early in Awake().
        /// Recommended: Application.persistentDataPath + "/CrashDumps"</summary>
        public static void SetCrashDumpDir(string dir) =>
            Native.BP_SetCrashDumpDir(dir ?? "");

        /// <summary>Manually write a memory snapshot dump without crashing.
        /// Useful for diagnosing hangs or abnormal states.</summary>
        public static void WriteCrashDump(string reason = null) =>
            Native.BP_WriteCrashDump(reason ?? "manual");

        /// <summary>Initialize the default HTTP client (cpp-httplib on native,
        /// emscripten_fetch on WebGL). Required before any LLM.* / HTTP.* node
        /// executes. Idempotent: safe to call multiple times.</summary>
        public static void InitDefaultHttpClient() => Native.BP_InitDefaultHttpClient();

        /// <summary>Drain the main-thread dispatch queue (async HTTP callbacks etc.).
        /// Normally called automatically by BP_Tick, but can be invoked manually
        /// when you have no active runners but still need to process pending work.</summary>
        public static void DrainQueue() => Native.BP_DrainQueue();

        /// <summary>Set the global Lua entry-point script path. All runners created
        /// afterwards will load this script on first Lua call. No-op without BLUEPRINT_HAS_LUA.</summary>
        public static void SetGlobalLuaEntry(string filePath) =>
            Native.BP_SetGlobalLuaEntry(filePath ?? "");

        /// <summary>Get the currently-configured global Lua entry path ("" if unset).</summary>
        public static string GetGlobalLuaEntry()
        {
            const int bufLen = 512;
            IntPtr buf = Marshal.AllocHGlobal(bufLen);
            try
            {
                int n = Native.BP_GetGlobalLuaEntry(buf, bufLen);
                return n > 0 ? Marshal.PtrToStringUTF8(buf, n) : "";
            }
            finally { Marshal.FreeHGlobal(buf); }
        }
    }

    // =========================================================================
    // Exception type
    // =========================================================================

    public sealed class BPException : Exception
    {
        public BPException(string message) : base(message) { }
    }

    // =========================================================================
    // BPBlueprintMeta — RAII wrapper around BP_Meta opaque handle
    // =========================================================================

    /// <summary>
    /// Parsed blueprint metadata returned by <see cref="BPRunner.ParseMetaFromJson"/>.
    /// Always dispose via using or Dispose() to free native memory.
    /// </summary>
    public sealed class BPBlueprintMeta : IDisposable
    {
        private IntPtr _handle;
        private const int BufSize = 512;

        internal BPBlueprintMeta(IntPtr handle) { _handle = handle; }

        public void Dispose()
        {
            if (_handle != IntPtr.Zero)
            {
                Native.BP_MetaFree(_handle);
                _handle = IntPtr.Zero;
            }
        }

        /// <summary>Number of dependency paths in metadata.dependencies.</summary>
        public int DependencyCount =>
            _handle != IntPtr.Zero ? Native.BP_MetaGetDependencyCount(_handle) : 0;

        /// <summary>Get dependency path at index. Returns null if out of range.</summary>
        public string GetDependency(int index)
        {
            if (_handle == IntPtr.Zero) return null;
            IntPtr buf = Marshal.AllocHGlobal(BufSize);
            try
            {
                int n = Native.BP_MetaGetDependency(_handle, index, buf, BufSize);
                return n >= 0 ? Marshal.PtrToStringUTF8(buf, n) : null;
            }
            finally { Marshal.FreeHGlobal(buf); }
        }

        /// <summary>All dependency paths as a string array.</summary>
        public string[] Dependencies
        {
            get
            {
                int count = DependencyCount;
                var result = new string[count];
                for (int i = 0; i < count; i++)
                    result[i] = GetDependency(i) ?? "";
                return result;
            }
        }

        public string Name
        {
            get
            {
                if (_handle == IntPtr.Zero) return "";
                IntPtr buf = Marshal.AllocHGlobal(BufSize);
                try { int n = Native.BP_MetaGetName(_handle, buf, BufSize); return n > 0 ? Marshal.PtrToStringUTF8(buf, n) : ""; }
                finally { Marshal.FreeHGlobal(buf); }
            }
        }

        public string Version
        {
            get
            {
                if (_handle == IntPtr.Zero) return "";
                IntPtr buf = Marshal.AllocHGlobal(BufSize);
                try { int n = Native.BP_MetaGetVersion(_handle, buf, BufSize); return n > 0 ? Marshal.PtrToStringUTF8(buf, n) : ""; }
                finally { Marshal.FreeHGlobal(buf); }
            }
        }

        public int BlueprintClass =>
            _handle != IntPtr.Zero ? Native.BP_MetaGetBlueprintClass(_handle) : -1;
    }


    public class BlueprintService
    {
        // ── 单例 ───────────────────────────────────────────────────
        private static readonly BlueprintService _instance = new BlueprintService();
        public static BlueprintService Instance => _instance;

        // ── Runner 管理 ───────────────────────────────────────────
        private readonly List<BPRunner> _runners = new List<BPRunner>();

        // ── 状态 ──────────────────────────────────────────────────
        public event Action<BPRunner> OnRunnerCreated;  // Runner 创建后触发（用于注册自定义节点）

        IntPtr LuaState => LuaManager.IsLuaValid ? LuaManager.Instance.ActiveLuaEnv.L : IntPtr.Zero;

        private BlueprintService()
        {
#if UNITY_WEBGL && !UNITY_EDITOR
            // WebGL：HTTP 客户端由 Emscripten + fetch API 提供
            // 默认 CreateDefaultHttpClient 在 WebGL 下就是 HttpClient_Emscripten
            // 注意：LLM 域名必须在 Unity Player Settings → WebGL → Publishing Settings 的 CORS 允许列表里
            // 或由 LLM 供应商返回正确的 CORS 响应头
#endif
            // 初始化默认 HTTP 客户端（Runtime 内部幂等，重复调用无副作用）
            BPRunner.InitDefaultHttpClient();
        }

        /// <summary>创建一个新 Runner。服务会自动每帧 Tick 它。</summary>
        public BPRunner CreateRunner()
        {
            var r = new BPRunner();
            _runners.Add(r);
            OnRunnerCreated?.Invoke(r);

            // 将 xLua 的 lua_State 注入给 BlueprintRuntime
            // BR 不拥有此 VM，不会在 Dispose 时 lua_close
            r.SetExternalLuaState(LuaState);

            string dumpDir = System.IO.Path.Combine(UnityEngine.Application.dataPath, "../CrashDumps");
            BPRunner.SetCrashDumpDir(dumpDir);
            
            r.OnPrint += (lv, msg) =>
            {
                switch (lv)
                {
                    case BPLogLevel.Warning: Debug.LogWarning($"[Blueprint] {msg}"); break;
                    case BPLogLevel.Error: Debug.LogError($"[Blueprint] {msg}"); break;
                    default: Debug.Log($"[Blueprint] {msg}"); break;
                }
            };

#if UNITY_EDITOR || DEVELOPMENT_BUILD
            r.EnableLogging(true);
            r.OnLog += (lv, msg) =>
            {
                switch (lv)
                {
                    case BPLogLevel.Warning: Debug.LogWarning($"[BP:dbg] {msg}"); break;
                    case BPLogLevel.Error: Debug.LogError($"[BP:dbg] {msg}"); break;
                    default: Debug.Log($"[BP:dbg] {msg}"); break;
                }
            };
#endif

            return r;
        }

        /// <summary>释放 Runner。NPC Destroy 时必须调用。</summary>
        public void ReleaseRunner(BPRunner r)
        {
            if (r == null) return;
            _runners.Remove(r);
            // 单个 Runner 销毁时不通知 lua_State 关闭——共享 VM 仍被其他 Runner 使用。
            // NotifyLuaStateClosing 仅在整个 VM 即将 Dispose 时（ReleaseAllRunner）才调用。
            r.Dispose();
        }

        public void ReleaseAllRunner()
        {
            // 先批量通知所有 Runner 清空 m_L（LuaEnv.Dispose 前必须完成），
            // 再批量 Dispose，避免 LuaEnv 已关闭后 Tick 访问野指针。
            foreach (var r in _runners)
            {
                try { r?.NotifyLuaStateClosing(); } catch { }
            }
            foreach (var r in _runners)
            {
                r?.Dispose();
            }
            _runners.Clear();
        }
    }


}

