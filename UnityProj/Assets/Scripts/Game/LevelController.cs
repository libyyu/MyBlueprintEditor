// LevelController.cs
// 关卡生命周期控制器 — 挂到每个关卡场景的根 GameObject
//
// 职责：
//   - 初始化关卡（绑定绳子、糖果、输入）
//   - 桥接 Lua 关卡逻辑（通过 LuaManager）
//   - 计时、计分、星级判定
//   - 通知 LevelManager 通关/失败
//
// Lua 侧通过 require 'game/level_controller' 获取当前关卡实例并注入逻辑。

using System.Collections.Generic;
using UnityEngine;
using XLua;
using CutRope.Framework;

namespace CutRope.Game
{
    [LuaCallCSharp]
    public class LevelController : MonoBehaviour
    {
        // ── 单例（关卡场景内唯一）────────────────────────────────────
        public static LevelController Current { get; private set; }

        [Header("关卡配置")]
        public string levelId = "1_1";

        [Header("星级判定（剩余绳子数 ≥ 阈值）")]
        public int star3MinCuts = 1;  // 1 刀以内 → 3星
        public int star2MinCuts = 2;  // 2 刀以内 → 2星
        // 否则 1 星

        // ── 运行时组件（场景中拖入或自动收集）──────────────────────
        [Header("场景组件")]
        public List<RopeSpawner> ropes  = new List<RopeSpawner>();
        public Candy             candy;
        public CutInput          cutInput;

        // ── 运行时状态 ────────────────────────────────────────────────
        public int   CutCount    { get; private set; }
        public float ElapsedTime { get; private set; }
        public bool  IsComplete  { get; private set; }
        public bool  IsFailed    { get; private set; }

        // ── Lua 钩子（由 Lua 侧注入）─────────────────────────────────
        public System.Action           OnLevelStart    { get; set; }
        public System.Action<int, int> OnLevelComplete { get; set; }  // (stars, score)
        public System.Action           OnLevelFailed   { get; set; }
        public System.Action<int>      OnCut           { get; set; }  // (cutCount)

        // ── 生命周期 ─────────────────────────────────────────────────
        private void Awake()
        {
            Current = this;
        }

        private void Start()
        {
            // 自动收集场景内的组件（Inspector 没手动连线时）
            if (ropes.Count == 0)
                ropes.AddRange(FindObjectsByType<RopeSpawner>(FindObjectsSortMode.None));
            if (candy == null)
                candy = FindFirstObjectByType<Candy>();
            if (cutInput == null)
                cutInput = FindFirstObjectByType<CutInput>();

            // 注册绳子到输入处理器
            if (cutInput != null)
            {
                cutInput.ClearRopes();
                foreach (var r in ropes) cutInput.RegisterRope(r);
            }

            // 绑定绳子切割事件
            foreach (var rope in ropes)
            {
                rope.OnRopeCut += OnRopeCut;
            }

            // 绑定糖果事件
            if (candy != null)
            {
                candy.OnEaten  += HandleCandyEaten;
                candy.OnFailed += HandleCandyFailed;
            }

            // 通知 Lua：关卡开始
            // LuaManager 会在场景加载后执行 game/level_controller.lua，注入钩子后再触发 Start
            Invoke(nameof(NotifyLuaStart), 0.1f);
        }

        private void Update()
        {
            if (!IsComplete && !IsFailed)
                ElapsedTime += Time.deltaTime;
        }

        private void OnDestroy()
        {
            if (Current == this) Current = null;
            OnLevelStart    = null;
            OnLevelComplete = null;
            OnLevelFailed   = null;
            OnCut           = null;
        }

        // ── 事件处理 ─────────────────────────────────────────────────

        private void OnRopeCut(RopeSpawner rope)
        {
            CutCount++;
            OnCut?.Invoke(CutCount);
            Debug.Log($"[LevelController] Cut #{CutCount}");
        }

        private void HandleCandyEaten()
        {
            if (IsComplete || IsFailed) return;
            IsComplete = true;

            int stars = CalcStars();
            int score = CalcScore();

            Debug.Log($"[LevelController] Complete! stars={stars} score={score}");
            OnLevelComplete?.Invoke(stars, score);
        }

        private void HandleCandyFailed()
        {
            if (IsComplete || IsFailed) return;
            IsFailed = true;

            Debug.Log("[LevelController] Failed!");
            OnLevelFailed?.Invoke();
        }

        // ── 星级 / 分数计算 ───────────────────────────────────────────

        private int CalcStars()
        {
            if (CutCount <= star3MinCuts) return 3;
            if (CutCount <= star2MinCuts) return 2;
            return 1;
        }

        private int CalcScore()
        {
            // 基础分 1000，每多一刀扣 200，时间越短加分
            int base_  = 1000;
            int cutPen = Mathf.Max(0, CutCount - 1) * 200;
            int timeBonus = Mathf.Max(0, 500 - (int)(ElapsedTime * 50));
            return Mathf.Max(0, base_ - cutPen + timeBonus);
        }

        // ── Lua 通知 ──────────────────────────────────────────────────
        private void NotifyLuaStart()
        {
            OnLevelStart?.Invoke();
        }
    }
}
