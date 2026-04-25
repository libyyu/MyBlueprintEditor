// ─────────────────────────────────────────────────────────────────────
// SpeakingIndicator.cs — NPC 说话视觉反馈
//
// 挂到 NPC 的嘴巴/头部/气泡上，监听 NpcVoice 的 AudioSource 实时音量：
//   - 驱动 localScale 模拟口型 (简单但有效)
//   - 或驱动 Animator 的 "IsSpeaking" / "MouthOpen" 参数
//   - 或驱动 UI 的说话波形图标
//
// WebGL 友好：仅读取 AudioSource.isPlaying + GetOutputData。
// ─────────────────────────────────────────────────────────────────────

using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame
{
    public class SpeakingIndicator : MonoBehaviour
    {
        public enum Mode { Scale, Animator, UIWave }

        [Header("模式")]
        [SerializeField] private Mode mode = Mode.Scale;

        [Header("Scale 模式")]
        [Tooltip("被缩放的 Transform（嘴巴/下巴骨骼 或 气泡图标）")]
        [SerializeField] private Transform scaleTarget;
        [SerializeField] private Vector3 baseScale    = Vector3.one;
        [SerializeField] private Vector3 maxScale     = new Vector3(1.3f, 1.5f, 1.3f);
        [SerializeField] private float   smoothSpeed  = 12f;

        [Header("Animator 模式")]
        [SerializeField] private Animator animator;
        [SerializeField] private string   isSpeakingParam = "IsSpeaking";
        [SerializeField] private string   mouthOpenParam  = "MouthOpen";

        [Header("UI Wave 模式")]
        [Tooltip("3~5 个 RectTransform 条形柱，模拟音频波形")]
        [SerializeField] private RectTransform[] waveBars;
        [SerializeField] private float waveMinHeight = 4f;
        [SerializeField] private float waveMaxHeight = 30f;

        [Header("通用")]
        [Tooltip("留空则自动找同物体的 NpcVoice 上的 AudioSource")]
        [SerializeField] private AudioSource audioSource;
        [SerializeField] private int sampleSize = 128;

        private float[] _samples;
        private float _currentVolume;

        void Start()
        {
            if (audioSource == null)
            {
                var voice = GetComponent<NpcVoice>();
                if (voice != null)
                    audioSource = GetComponent<AudioSource>();
            }
            _samples = new float[sampleSize];

            if (scaleTarget == null && mode == Mode.Scale)
                scaleTarget = transform;
        }

        void Update()
        {
            if (audioSource == null) return;

            bool speaking = audioSource.isPlaying;
            float vol = 0f;

            if (speaking)
            {
                audioSource.GetOutputData(_samples, 0);
                float sum = 0f;
                for (int i = 0; i < _samples.Length; i++)
                    sum += _samples[i] * _samples[i];
                vol = Mathf.Sqrt(sum / _samples.Length);  // RMS
            }

            _currentVolume = Mathf.Lerp(_currentVolume, vol, Time.deltaTime * smoothSpeed);

            switch (mode)
            {
                case Mode.Scale:
                    UpdateScale(speaking);
                    break;
                case Mode.Animator:
                    UpdateAnimator(speaking);
                    break;
                case Mode.UIWave:
                    UpdateWave(speaking);
                    break;
            }
        }

        private void UpdateScale(bool speaking)
        {
            if (scaleTarget == null) return;
            float t = Mathf.Clamp01(_currentVolume * 10f);  // 归一化到 0~1
            scaleTarget.localScale = speaking
                ? Vector3.Lerp(baseScale, maxScale, t)
                : Vector3.Lerp(scaleTarget.localScale, baseScale, Time.deltaTime * smoothSpeed);
        }

        private void UpdateAnimator(bool speaking)
        {
            if (animator == null) return;
            animator.SetBool(isSpeakingParam, speaking);
            animator.SetFloat(mouthOpenParam, _currentVolume * 10f);
        }

        private void UpdateWave(bool speaking)
        {
            if (waveBars == null || waveBars.Length == 0) return;

            for (int i = 0; i < waveBars.Length; i++)
            {
                if (waveBars[i] == null) continue;
                float target;
                if (speaking)
                {
                    // 每个条分配不同频段，增加视觉多样性
                    int idx = Mathf.Clamp(i * (_samples.Length / waveBars.Length), 0, _samples.Length - 1);
                    float v = Mathf.Abs(_samples[idx]);
                    target = Mathf.Lerp(waveMinHeight, waveMaxHeight, v * 8f);
                }
                else
                {
                    target = waveMinHeight;
                }

                var sd = waveBars[i].sizeDelta;
                sd.y = Mathf.Lerp(sd.y, target, Time.deltaTime * smoothSpeed);
                waveBars[i].sizeDelta = sd;
            }
        }
    }
}
