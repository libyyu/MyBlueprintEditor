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

using System;
using System.Runtime.InteropServices;
using System.Text;
using UnityEngine;

namespace BlueprintRuntime
{
    // =========================================================================
    // Platform-specific DLL name selection
    // =========================================================================

    internal static class NativeLib
    {
#if UNITY_IOS || UNITY_WEBGL
        internal const string DLL = "__Internal";
#elif UNITY_ANDROID
        internal const string DLL = "BlueprintRuntime";
#elif UNITY_STANDALONE_WIN || UNITY_EDITOR_WIN
        internal const string DLL = "BlueprintRuntime";
#elif UNITY_STANDALONE_OSX || UNITY_EDITOR_OSX
        internal const string DLL = "BlueprintRuntime";
#elif UNITY_STANDALONE_LINUX || UNITY_EDITOR_LINUX
        internal const string DLL = "BlueprintRuntime";
#else
        internal const string DLL = "BlueprintRuntime";
#endif
    }

    // =========================================================================
    // Raw P/Invoke declarations (internal – use BPRunner instead)
    // =========================================================================

    internal static class Native
    {
        // Log callback delegate – must be cdecl to match C side
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void LogCallbackDelegate([MarshalAs(UnmanagedType.LPStr)] string message);

        // --- Lifecycle ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr BP_CreateRunner();

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_DestroyRunner(IntPtr runner);

        // --- Loading ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_LoadFromJson(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string json);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_LoadFromFile(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string filePath);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_IsLoaded(IntPtr runner);

        // --- Execution ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_Execute(IntPtr runner);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_ExecuteNode(IntPtr runner, ulong nodeId);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_Tick(IntPtr runner, float deltaTime);

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

        // BP_GetVariableString copies into a caller-owned byte buffer.
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_GetVariableString(IntPtr runner,
            [MarshalAs(UnmanagedType.LPStr)] string name,
            IntPtr buf, int bufLen);

        // --- Logging (internal debug diagnostics) ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_SetLogCallback(IntPtr runner, LogCallbackDelegate callback);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_EnableLogging(IntPtr runner, int enable);

        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_IsLoggingEnabled(IntPtr runner);

        // --- Print (application-level output: PrintString / Log / FormatLog nodes) ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void BP_SetPrintCallback(IntPtr runner, LogCallbackDelegate callback);

        // --- Error ---
        [DllImport(NativeLib.DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int BP_GetLastError(IntPtr runner, IntPtr buf, int bufLen);
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
        // Internal buffer size for string retrieval
        private const int StringBufSize = 4096;

        private IntPtr _handle;
        private bool   _disposed;

        // Keep a strong reference to delegates to prevent GC collection
        // while the native side holds function pointers.
        private Native.LogCallbackDelegate _logDelegate;
        private Native.LogCallbackDelegate _printDelegate;

        /// <summary>
        /// Fired for internal debug/diagnostic messages.
        /// Only active when logging is enabled (see EnableLogging).
        /// </summary>
        public event Action<string> OnLog;

        /// <summary>
        /// Fired by PrintString / Log / FormatLog nodes.
        /// This is the application-level print channel – wire it to your
        /// game console, UI log, or Debug.Log.  Independent of EnableLogging.
        /// </summary>
        public event Action<string> OnPrint;

        // ---------------------------------------------------------------------
        // Construction / disposal
        // ---------------------------------------------------------------------

        public BPRunner()
        {
            _handle = Native.BP_CreateRunner();
            if (_handle == IntPtr.Zero)
                throw new InvalidOperationException("BP_CreateRunner returned null");

            // Wire up callbacks – keep delegate refs alive to prevent GC
            _logDelegate   = msg => OnLog?.Invoke(msg);
            _printDelegate = msg => OnPrint?.Invoke(msg);
            Native.BP_SetLogCallback(_handle, _logDelegate);
            Native.BP_SetPrintCallback(_handle, _printDelegate);
        }

        public void Dispose()
        {
            if (!_disposed)
            {
                if (_handle != IntPtr.Zero)
                {
                    // Clear native callbacks before destroying to avoid dangling pointers
                    Native.BP_SetLogCallback(_handle, null);
                    Native.BP_SetPrintCallback(_handle, null);
                    Native.BP_DestroyRunner(_handle);
                    _handle = IntPtr.Zero;
                }
                _logDelegate   = null;
                _printDelegate = null;
                _disposed = true;
            }
        }

        // ---------------------------------------------------------------------
        // Loading
        // ---------------------------------------------------------------------

        /// <summary>
        /// Load a blueprint from a JSON string.
        /// Throws BPException on failure.
        /// </summary>
        public void LoadFromJson(string json)
        {
            ThrowIfDisposed();
            if (Native.BP_LoadFromJson(_handle, json) != 0)
                throw new BPException("LoadFromJson failed: " + GetLastError());
        }

        /// <summary>
        /// Load a blueprint from a file path (uses the runtime's IFileSystem).
        /// On WebGL, paths are relative to StreamingAssets.
        /// Throws BPException on failure.
        /// </summary>
        public void LoadFromFile(string filePath)
        {
            ThrowIfDisposed();
            if (Native.BP_LoadFromFile(_handle, filePath) != 0)
                throw new BPException("LoadFromFile failed: " + GetLastError());
        }

        /// <summary>Returns true if a blueprint has been loaded successfully.</summary>
        public bool IsLoaded
        {
            get
            {
                ThrowIfDisposed();
                return Native.BP_IsLoaded(_handle) != 0;
            }
        }

        // ---------------------------------------------------------------------
        // Execution
        // ---------------------------------------------------------------------

        /// <summary>
        /// Execute the blueprint from its entry point(s).
        /// Throws BPException on failure.
        /// </summary>
        public void Execute()
        {
            ThrowIfDisposed();
            if (Native.BP_Execute(_handle) != 0)
                throw new BPException("Execute failed: " + GetLastError());
        }

        /// <summary>
        /// Execute a single node by ID.
        /// Throws BPException on failure.
        /// </summary>
        public void ExecuteNode(ulong nodeId)
        {
            ThrowIfDisposed();
            if (Native.BP_ExecuteNode(_handle, nodeId) != 0)
                throw new BPException("ExecuteNode failed: " + GetLastError());
        }

        /// <summary>
        /// Advance async timers. Call once per frame from Update() or FixedUpdate().
        /// </summary>
        public void Tick(float deltaTime)
        {
            ThrowIfDisposed();
            Native.BP_Tick(_handle, deltaTime);
        }

        // ---------------------------------------------------------------------
        // Variables – set
        // ---------------------------------------------------------------------

        public void SetVariable(string name, long value)
        {
            ThrowIfDisposed();
            Native.BP_SetVariableInt(_handle, name, value);
        }

        public void SetVariable(string name, double value)
        {
            ThrowIfDisposed();
            Native.BP_SetVariableFloat(_handle, name, value);
        }

        public void SetVariable(string name, float value)
            => SetVariable(name, (double)value);

        public void SetVariable(string name, int value)
            => SetVariable(name, (long)value);

        public void SetVariable(string name, bool value)
        {
            ThrowIfDisposed();
            Native.BP_SetVariableBool(_handle, name, value ? 1 : 0);
        }

        public void SetVariable(string name, string value)
        {
            ThrowIfDisposed();
            Native.BP_SetVariableString(_handle, name, value ?? "");
        }

        // ---------------------------------------------------------------------
        // Variables – get
        // ---------------------------------------------------------------------

        public long   GetVariableInt   (string name) { ThrowIfDisposed(); return Native.BP_GetVariableInt(_handle, name); }
        public double GetVariableFloat (string name) { ThrowIfDisposed(); return Native.BP_GetVariableFloat(_handle, name); }
        public bool   GetVariableBool  (string name) { ThrowIfDisposed(); return Native.BP_GetVariableBool(_handle, name) != 0; }

        public string GetVariableString(string name)
        {
            ThrowIfDisposed();
            // Allocate unmanaged buffer, copy, convert to managed string
            IntPtr buf = Marshal.AllocHGlobal(StringBufSize);
            try
            {
                int written = Native.BP_GetVariableString(_handle, name, buf, StringBufSize);
                if (written < 0) return null;   // variable not found
                return Marshal.PtrToStringUTF8(buf, written);
            }
            finally
            {
                Marshal.FreeHGlobal(buf);
            }
        }

        // ---------------------------------------------------------------------
        // Logging control
        // ---------------------------------------------------------------------

        /// <summary>
        /// Enable or disable internal debug logging (OnLog callbacks).
        /// Disabled by default in Release (NDEBUG) builds of the native library.
        /// </summary>
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
                int written = Native.BP_GetLastError(_handle, buf, StringBufSize);
                return written > 0 ? Marshal.PtrToStringUTF8(buf, written) : "";
            }
            finally
            {
                Marshal.FreeHGlobal(buf);
            }
        }

        // ---------------------------------------------------------------------
        // Internals
        // ---------------------------------------------------------------------

        private void ThrowIfDisposed()
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(BPRunner));
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
    // MonoBehaviour helper – optional convenience component
    // =========================================================================

    /// <summary>
    /// Drop this on a GameObject to drive a BlueprintRunner from Unity.
    ///
    ///   1. Assign blueprintJson (TextAsset) in the Inspector, OR
    ///      set blueprintFilePath to load from StreamingAssets at runtime.
    ///   2. The runner is created in Awake, executed in Start (if autoExecute),
    ///      and ticked every frame in Update.
    ///   3. Call Execute() / SetVariable() / GetVariable*() from other scripts
    ///      via GetComponent&lt;BlueprintBehaviour&gt;().Runner.
    /// </summary>
    public class BlueprintBehaviour : MonoBehaviour
    {
        [Header("Blueprint Source (pick one)")]
        [Tooltip("JSON TextAsset dragged from Project window")]
        public TextAsset blueprintJson;

        [Tooltip("Path relative to StreamingAssets (used on WebGL/iOS if blueprintJson is null)")]
        public string blueprintFilePath;

        [Header("Options")]
        [Tooltip("Call Execute() automatically in Start()")]
        public bool autoExecute = true;

        [Tooltip("Call Tick(deltaTime) every frame in Update()")]
        public bool tickEveryFrame = true;

        /// <summary>Access the underlying runner for variable read/write.</summary>
        public BPRunner Runner { get; private set; }

        protected virtual void Awake()
        {
            Runner = new BPRunner();
            // PrintString / Log / FormatLog → Unity console
            Runner.OnPrint += msg => Debug.Log($"[Blueprint] {msg}");
            // Internal debug log only in Editor / development builds
#if UNITY_EDITOR || DEVELOPMENT_BUILD
            Runner.EnableLogging(true);
            Runner.OnLog += msg => Debug.Log($"[Blueprint:dbg] {msg}");
#endif

            try
            {
                if (blueprintJson != null)
                    Runner.LoadFromJson(blueprintJson.text);
                else if (!string.IsNullOrEmpty(blueprintFilePath))
                    Runner.LoadFromFile(blueprintFilePath);
            }
            catch (BPException ex)
            {
                Debug.LogError($"[Blueprint] Load error: {ex.Message}");
            }
        }

        protected virtual void Start()
        {
            if (autoExecute && Runner.IsLoaded)
            {
                try   { Runner.Execute(); }
                catch (BPException ex) { Debug.LogError($"[Blueprint] Execute error: {ex.Message}"); }
            }
        }

        protected virtual void Update()
        {
            if (tickEveryFrame && Runner != null)
                Runner.Tick(Time.deltaTime);
        }

        protected virtual void OnDestroy()
        {
            Runner?.Dispose();
            Runner = null;
        }
    }
}
