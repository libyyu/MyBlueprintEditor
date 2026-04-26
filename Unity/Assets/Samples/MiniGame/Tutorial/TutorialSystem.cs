// ─────────────────────────────────────────────────────────────────────
// TutorialSystem.cs — 新手引导系统
//
// 功能：
//   - 步骤序列（TutorialStep[]）：每步指定目标 UI 元素 + 提示文字 + 完成条件
//   - 高亮遮罩：半透明黑遮全屏，仅被指引的 UI 可交互
//   - 箭头指向目标
//   - 进度持久化（完成后不再显示）
//   - 蓝图触发：蓝图节点 Tutorial.Start / Tutorial.Complete 可驱动引导
//   - 条件跳过：已完成的步骤自动跳过
// ─────────────────────────────────────────────────────────────────────

using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

namespace BlueprintRuntime.Samples.MiniGame
{
    public class TutorialSystem : MonoBehaviour
    {
        public static TutorialSystem Instance { get; private set; }

        [Header("引导步骤")]
        [SerializeField] private TutorialStep[] steps;

        [Header("UI 引用")]
        [SerializeField] private GameObject    overlayPanel;     // 全屏半透明遮罩
        [SerializeField] private Image         maskImage;        // 遮罩（可选：镂空效果用 Sprite Mask）
        [SerializeField] private RectTransform arrowTransform;   // 箭头指向
        [SerializeField] private TMP_Text      hintText;         // 提示文字
        [SerializeField] private Button        nextButton;       // "下一步" / "知道了"
        [SerializeField] private Button        skipButton;       // "跳过引导"
        [SerializeField] private TMP_Text      progressText;     // "1/5"

        [Header("持久化")]
        [SerializeField] private string storageKey = "bp_tutorial_done";

        // ── 事件 ────────────────────────────────────────────────────
        public event Action<int>    OnStepShown;       // stepIndex
        public event Action         OnTutorialDone;

        private int  _currentStep = -1;
        private bool _isRunning;
        private HashSet<string> _completed = new();

        [Serializable]
        public class TutorialStep
        {
            public string id;                        // 唯一标识（持久化用）
            [TextArea(1, 3)]
            public string hintMessage;               // 提示文字
            public RectTransform targetUI;           // 高亮的目标 UI 元素
            public float  arrowOffset = 60f;         // 箭头距离目标的偏移
            public bool   waitForClick = true;       // true=点目标或"下一步"完成，false=自动 N 秒后
            public float  autoAdvanceDelay = 3f;     // waitForClick=false 时的延迟
            public string requiredCondition = "";    // 非空时：蓝图变量名须为 true 才显示此步
        }

        void Awake()
        {
            if (Instance != null) { Destroy(gameObject); return; }
            Instance = this;
            LoadProgress();
        }

        void Start()
        {
            if (overlayPanel != null) overlayPanel.SetActive(false);
            if (nextButton != null) nextButton.onClick.AddListener(NextStep);
            if (skipButton != null) skipButton.onClick.AddListener(SkipAll);
        }

        // ── 公共 API ────────────────────────────────────────────────

        /// <summary>从第一个未完成的步骤开始引导</summary>
        public void StartTutorial()
        {
            if (_isRunning || steps == null || steps.Length == 0) return;

            // 如果全部已完成，不再显示
            if (IsAllDone()) { OnTutorialDone?.Invoke(); return; }

            _isRunning = true;
            _currentStep = -1;
            NextStep();
        }

        /// <summary>蓝图可调：直接开始指定步骤</summary>
        public void ShowStep(string stepId)
        {
            if (steps == null) return;
            for (int i = 0; i < steps.Length; i++)
            {
                if (steps[i].id == stepId)
                {
                    _isRunning = true;
                    _currentStep = i - 1;
                    NextStep();
                    return;
                }
            }
        }

        /// <summary>标记某步完成</summary>
        public void CompleteStep(string stepId)
        {
            _completed.Add(stepId);
            SaveProgress();
        }

        public bool IsStepDone(string stepId) => _completed.Contains(stepId);
        public bool IsAllDone()
        {
            if (steps == null) return true;
            foreach (var s in steps)
                if (!_completed.Contains(s.id)) return false;
            return true;
        }

        // ── 步骤驱动 ────────────────────────────────────────────────

        private void NextStep()
        {
            _currentStep++;

            // 跳过已完成/条件不满足的步骤
            while (_currentStep < steps.Length)
            {
                var step = steps[_currentStep];
                if (_completed.Contains(step.id))
                {
                    _currentStep++;
                    continue;
                }
                // 条件检查（简化：检查 PlayerPrefs 或 BlueprintStorage 中的 key）
                if (!string.IsNullOrEmpty(step.requiredCondition))
                {
                    string val = BlueprintStorage.GetString(step.requiredCondition, "");
                    if (val != "true" && val != "1")
                    {
                        _currentStep++;
                        continue;
                    }
                }
                break;
            }

            if (_currentStep >= steps.Length)
            {
                FinishTutorial();
                return;
            }

            ShowCurrentStep();
        }

        private void ShowCurrentStep()
        {
            var step = steps[_currentStep];

            if (overlayPanel != null) overlayPanel.SetActive(true);
            if (hintText     != null) hintText.text = step.hintMessage;
            if (progressText != null) progressText.text = $"{_currentStep + 1}/{steps.Length}";

            // 箭头指向目标
            if (arrowTransform != null && step.targetUI != null)
            {
                arrowTransform.gameObject.SetActive(true);
                PositionArrow(step.targetUI, step.arrowOffset);
            }
            else if (arrowTransform != null)
            {
                arrowTransform.gameObject.SetActive(false);
            }

            // 按钮
            if (nextButton != null)
            {
                var txt = nextButton.GetComponentInChildren<TMP_Text>();
                if (txt != null) txt.text = (_currentStep == steps.Length - 1) ? "完成" : "下一步";
            }

            OnStepShown?.Invoke(_currentStep);

            // 自动推进
            if (!step.waitForClick)
                StartCoroutine(AutoAdvance(step.autoAdvanceDelay, step.id));
        }

        private IEnumerator AutoAdvance(float delay, string stepId)
        {
            yield return new WaitForSeconds(delay);
            _completed.Add(stepId);
            SaveProgress();
            NextStep();
        }

        private void SkipAll()
        {
            if (steps != null)
                foreach (var s in steps) _completed.Add(s.id);
            SaveProgress();
            FinishTutorial();
        }

        private void FinishTutorial()
        {
            _isRunning = false;
            if (overlayPanel != null) overlayPanel.SetActive(false);
            OnTutorialDone?.Invoke();
        }

        private void PositionArrow(RectTransform target, float offset)
        {
            if (arrowTransform == null || target == null) return;

            // 把目标的屏幕位置转到 overlay 的本地坐标
            Vector3 worldPos;
            RectTransformUtility.ScreenPointToWorldPointInRectangle(
                arrowTransform.parent as RectTransform,
                RectTransformUtility.WorldToScreenPoint(null, target.position),
                null, out worldPos);

            arrowTransform.position = worldPos + Vector3.up * offset;

            // 箭头朝下指
            arrowTransform.localRotation = Quaternion.Euler(0, 0, 180);
        }

        // ── 持久化 ──────────────────────────────────────────────────

        private void SaveProgress()
        {
            var wrapper = new ProgressWrapper();
            foreach (var id in _completed)
                wrapper.ids.Add(id);
            BlueprintStorage.SetString(storageKey, JsonUtility.ToJson(wrapper));
        }

        private void LoadProgress()
        {
            string json = BlueprintStorage.GetString(storageKey, "");
            if (string.IsNullOrEmpty(json)) return;
            try
            {
                var wrapper = JsonUtility.FromJson<ProgressWrapper>(json);
                if (wrapper?.ids != null)
                    foreach (var id in wrapper.ids) _completed.Add(id);
            }
            catch { }
        }

        [Serializable]
        private class ProgressWrapper { public List<string> ids = new(); }
    }
}
