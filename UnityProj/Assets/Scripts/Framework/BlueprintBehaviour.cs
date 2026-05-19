using BlueprintRuntime;
using UnityEngine;

namespace CutRope.Framework
{
    // =========================================================================
    // MonoBehaviour helper ¨C optional convenience component
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
        public BPRunner Runner { get; private set; }

        protected virtual void Awake()
        {
            Runner = new BPRunner();

            Runner.OnPrint += (lv, msg) =>
            {
                switch (lv)
                {
                    case BPLogLevel.Warning: Debug.LogWarning($"[Blueprint] {msg}"); break;
                    case BPLogLevel.Error: Debug.LogError($"[Blueprint] {msg}"); break;
                    default: Debug.Log($"[Blueprint] {msg}"); break;
                }
            };

#if UNITY_EDITOR || DEVELOPMENT_BUILD
            Runner.EnableLogging(true);
            Runner.OnLog += (lv, msg) =>
            {
                switch (lv)
                {
                    case BPLogLevel.Warning: Debug.LogWarning($"[BP:dbg] {msg}"); break;
                    case BPLogLevel.Error: Debug.LogError($"[BP:dbg] {msg}"); break;
                    default: Debug.Log($"[BP:dbg] {msg}"); break;
                }
            };
#endif
            // Register custom node defs + handlers before loading the blueprint.
            RegisterNodes(Runner);

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
                try { Runner.Execute(); }
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

        /// <summary>
        /// Override this to register custom node definitions and handlers before
        /// the blueprint is loaded. Called at the end of Awake().
        ///
        /// Example:
        ///   protected override void RegisterNodes(BPRunner runner)
        ///   {
        ///       runner.RegisterNodeDef(new BPNodeDef {
        ///           id = "MyAdd", name = "My Add", category = "Custom",
        ///           pins = new[] {
        ///               new BPPinDef { name="A", dataType=BPPinType.Float, isInput=true  },
        ///               new BPPinDef { name="B", dataType=BPPinType.Float, isInput=true  },
        ///               new BPPinDef { name="R", dataType=BPPinType.Float, isInput=false },
        ///           }
        ///       });
        ///       runner.RegisterHandler("MyAdd", ctx => {
        ///           ctx.SetOutputFloat("R", (float)ctx.GetInputFloat("A") + (float)ctx.GetInputFloat("B"));
        ///           return true;
        ///       });
        ///   }
        /// </summary>
        protected virtual void RegisterNodes(BPRunner runner) { }
    }
}