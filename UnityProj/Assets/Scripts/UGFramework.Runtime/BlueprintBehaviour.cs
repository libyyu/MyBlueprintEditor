using System;
using UnityEngine;

namespace UGFramework.Runtime
{
    // =========================================================================
    // MonoBehaviour helper �C optional convenience component
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
    ///   4. Override RegisterNodes() to add custom node defs + handlers before load.
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

        /// <summary>Access the underlying runner for variable read/write and handler registration.</summary>
        public BlueprintRuntime.BPRunner Runner { get; private set; }

        protected virtual void Awake()
        {
            Runner = BlueprintRuntime.BlueprintService.Instance.CreateRunner();

            // Register custom node defs + handlers before loading the blueprint.
            RegisterNodes(Runner);

            try
            {
                if (blueprintJson != null)
                    Runner.LoadFromJson(blueprintJson.text);
                else if (!string.IsNullOrEmpty(blueprintFilePath))
                    Runner.LoadFromFile(blueprintFilePath);
            }
            catch (BlueprintRuntime.BPException ex)
            {
                Debug.LogError($"[Blueprint] Load error: {ex.Message}");
            }
        }

        protected virtual void Start()
        {
            if (autoExecute && Runner.IsLoaded)
            {
                try { Runner.Execute(); }
                catch (BlueprintRuntime.BPException ex) { Debug.LogError($"[Blueprint] Execute error: {ex.Message}"); }
            }
        }

        protected virtual void Update()
        {
            // xLua 主 VM 模式：LuaEnv 已 Dispose 时跳过 Tick，
            // 避免 Update 在 LuaEnv.Dispose 后访问已释放的 lua_State。
            if (tickEveryFrame && Runner != null && LuaManager.IsLuaValid)
                Runner.Tick(Time.deltaTime);
        }

        protected virtual void OnDestroy()
        {
            BlueprintRuntime.BlueprintService.Instance.ReleaseRunner(Runner);
        }

        /// <summary>
        /// Override this to register custom node definitions and handlers before
        /// the blueprint is loaded. Called at the end of Awake().
        ///
        /// NOTE: BPRunner.RegisterNodeDef / RegisterHandler are static and register
        /// process-globally; the `runner` parameter is unused by these calls
        /// (kept only to avoid breaking the override signature). You only need
        /// to register each id once per process.
        ///
        /// Example:
        ///   protected override void RegisterNodes(BPRunner runner)
        ///   {
        ///       BPRunner.RegisterNodeDef(new BPNodeDef {
        ///           id = "MyAdd", name = "My Add", category = "Custom",
        ///           pins = new[] {
        ///               new BPPinDef { name="A", dataType=BPPinType.Float, isInput=true  },
        ///               new BPPinDef { name="B", dataType=BPPinType.Float, isInput=true  },
        ///               new BPPinDef { name="R", dataType=BPPinType.Float, isInput=false },
        ///           }
        ///       });
        ///       BPRunner.RegisterHandler("MyAdd", ctx => {
        ///           ctx.SetOutputFloat("R", (float)ctx.GetInputFloat("A") + (float)ctx.GetInputFloat("B"));
        ///           return true;
        ///       });
        ///   }
        /// </summary>
        protected virtual void RegisterNodes(BlueprintRuntime.BPRunner runner) { }
    }
}