// LevelController.cs
// 关卡引擎层 — 只负责场景对象的引用暴露和每帧 tick 传递给 Lua
//
// 设计原则：
//   C# 只提供引擎接口（物理、渲染、输入），不含任何游戏逻辑。
//   通关条件、星级计算、分数规则、特殊机制全部在 Lua / Blueprint 实现。
//
// Lua 侧通过 CS.CutRope.Game.LevelController.Current 获取此实例，
// 注入 OnTick / OnCut / OnCandyEaten / OnCandyFailed 钩子。

using System.Collections.Generic;
using UnityEngine;
using XLua;
using CutRope.Framework;

namespace CutRope.Game
{
    [LuaCallCSharp]
    public class LevelController : MonoBehaviour
    {
        // ── 单例 ──────────────────────────────────────────────────────
        public static LevelController Current { get; private set; }

        // ── 场景对象引用（Inspector 连线或自动收集）──────────────────
        [Header("场景对象（可留空，自动收集）")]
        public List<RopeSpawner> ropes    = new List<RopeSpawner>();
        public Candy             candy;
        public CutInput          cutInput;

        // ── 星星 ─────────────────────────────────────────────────────
        private readonly List<Star> _stars = new List<Star>();
        public int StarCount     => _stars.Count;
        public int StarsCollected { get; private set; }

        // ── 只读引擎数据（Lua 可读，不可在 C# 里判断）───────────────
        public float ElapsedTime { get; private set; }
        public int   CutCount    { get; private set; }
        public bool  IsRunning   { get; private set; }
        public bool  IsPaused    { get; private set; }

        // ── Lua 钩子（逻辑全在这里，由 Lua 注入）────────────────────
        [CSharpCallLua] public delegate void LuaTickDelegate(float dt);
        [CSharpCallLua] public delegate void LuaCutDelegate(int index, float worldX, float worldY);
        [CSharpCallLua] public delegate void LuaVoidDelegate();
        [CSharpCallLua] public delegate void LuaStarDelegate(int starIndex, int totalCollected);

        public LuaTickDelegate OnTick          { get; set; }  // 每帧
        public LuaCutDelegate  OnCut           { get; set; }  // 切割时
        public LuaVoidDelegate OnCandyEaten    { get; set; }  // 糖果进嘴
        public LuaVoidDelegate OnCandyFailed   { get; set; }  // 糖果失败
        public LuaVoidDelegate OnLevelReady    { get; set; }  // 场景就绪（Start 后）
        public LuaStarDelegate OnStarCollected { get; set; }  // 星星收集

        // ── 生命周期 ─────────────────────────────────────────────────
        private void Awake()
        {
            Current = this;
        }

        private void Start()
        {
            // 自动收集
            if (ropes.Count == 0)
                ropes.AddRange(FindObjectsByType<RopeSpawner>(FindObjectsSortMode.None));
            if (candy == null)
                candy = FindFirstObjectByType<Candy>();
            if (cutInput == null)
                cutInput = FindFirstObjectByType<CutInput>();

            // 注册绳子到输入
            if (cutInput != null)
            {
                cutInput.ClearRopes();
                foreach (var r in ropes) cutInput.RegisterRope(r);
            }

            // 绑定物理事件 → 只递增计数 + 调 Lua 钩子，不做判断
            foreach (var rope in ropes)
            {
                rope.OnRopeCut += (r) =>
                {
                    CutCount++;
                    var pos = r.transform.position;
                    OnCut?.Invoke(CutCount, pos.x, pos.y);
                };
            }

            if (candy != null)
            {
                candy.OnEaten  += () => OnCandyEaten?.Invoke();
                candy.OnFailed += () => OnCandyFailed?.Invoke();
            }

            // 自动收集场景中所有星星并注册回调
            var sceneStars = FindObjectsByType<Star>(FindObjectsSortMode.None);
            foreach (var star in sceneStars)
            {
                _stars.Add(star);
                star.OnCollected += (s) =>
                {
                    StarsCollected++;
                    OnStarCollected?.Invoke(s.starIndex, StarsCollected);
                    Debug.Log($"[LevelController] Star {s.starIndex} collected! Total: {StarsCollected}/{StarCount}");
                };
            }
            Debug.Log($"[LevelController] Stars in scene: {_stars.Count}");

            IsRunning = true;

            // 延一帧再触发 Ready：确保 BlueprintBehaviour 也已 Start 完毕，
            // Execute() 的 OnBeginPlay 在 Lua 侧调用 Rope.SpawnAll 时 candy 已初始化
            Invoke(nameof(FireReady), 0.1f);
        }

        private void FireReady()
        {
            OnLevelReady?.Invoke();

            // 通知场景里的 BlueprintBehaviour 执行蓝图（如果它 autoExecute=false）
            // 如果 autoExecute=true，BlueprintBehaviour.Start() 已经执行过了，
            // 这里再 Execute() 会重新触发 OnBeginPlay，确保 candy/ropes 已就绪
            var bp = FindFirstObjectByType<CutRope.Framework.BlueprintBehaviour>();
            if (bp != null && bp.Runner != null && bp.Runner.IsLoaded)
            {
                try { bp.Runner.Execute(); }
                catch (System.Exception e) { Debug.LogError($"[LevelController] Blueprint Execute error: {e.Message}"); }
            }
        }

        private void Update()
        {
            if (!IsRunning) return;
            ElapsedTime += Time.deltaTime;
            OnTick?.Invoke(Time.deltaTime);
        }

        private void OnDestroy()
        {
            if (Current == this) Current = null;
            IsRunning      = false;
            OnTick         = null;
            OnCut          = null;
            OnCandyEaten   = null;
            OnCandyFailed  = null;
            OnLevelReady   = null;
            OnStarCollected = null;
        }

        // ── 引擎接口（供 Lua 调用）───────────────────────────────────

        /// <summary>停止 tick（通关或失败后调用）</summary>
        public void StopLevel()   => IsRunning = false;
        public void PauseLevel()  { IsPaused = true;  Time.timeScale = 0f; }
        public void ResumeLevel() { IsPaused = false; Time.timeScale = 1f; }

        /// <summary>获取第 i 条绳子（Lua 直接操控）</summary>
        public RopeSpawner GetRope(int index)
        {
            if (index < 0 || index >= ropes.Count) return null;
            return ropes[index];
        }

        /// <summary>获取绳子数量</summary>
        public int GetRopeCount() => ropes.Count;
    }
}
