// ─────────────────────────────────────────────────────────────────────
// GameAudioManager.cs — 游戏音效管理器
//
// 统一管理 UI 音效、NPC 对话音效、环境音。
// 支持音量控制和静音。
// ─────────────────────────────────────────────────────────────────────

using UnityEngine;

namespace BlueprintMiniGame
{
    public class GameAudioManager : MonoBehaviour
    {
        public static GameAudioManager Instance { get; private set; }

        [Header("音源")]
        [SerializeField] private AudioSource uiSource;
        [SerializeField] private AudioSource bgmSource;
        [SerializeField] private AudioSource sfxSource;

        [Header("UI 音效")]
        [SerializeField] private AudioClip clickSound;
        [SerializeField] private AudioClip sendSound;
        [SerializeField] private AudioClip receiveSound;
        [SerializeField] private AudioClip questAcceptSound;
        [SerializeField] private AudioClip questCompleteSound;
        [SerializeField] private AudioClip levelUpSound;
        [SerializeField] private AudioClip buySound;
        [SerializeField] private AudioClip errorSound;
        [SerializeField] private AudioClip notificationSound;

        [Header("环境")]
        [SerializeField] private AudioClip tavernBgm;
        [SerializeField] private AudioClip nightBgm;

        [Header("音量")]
        [Range(0f, 1f)] [SerializeField] private float masterVolume = 1f;
        [Range(0f, 1f)] [SerializeField] private float bgmVolume    = 0.5f;
        [Range(0f, 1f)] [SerializeField] private float sfxVolume    = 0.8f;

        void Awake()
        {
            if (Instance != null) { Destroy(gameObject); return; }
            Instance = this;

            // 加载保存的音量设置
            masterVolume = BlueprintStorage.GetFloat("audio_master", 1f);
            bgmVolume    = BlueprintStorage.GetFloat("audio_bgm", 0.5f);
            sfxVolume    = BlueprintStorage.GetFloat("audio_sfx", 0.8f);

            ApplyVolumes();
        }

        void Start()
        {
            // 自动订阅游戏事件
            if (Quest.QuestSystem.Instance != null)
            {
                Quest.QuestSystem.Instance.OnQuestAccepted  += _ => PlayUI(questAcceptSound);
                Quest.QuestSystem.Instance.OnQuestCompleted += _ => PlayUI(questCompleteSound);
            }
            if (PlayerStats.Instance != null)
            {
                PlayerStats.Instance.OnLevelUp += _ => PlayUI(levelUpSound);
            }

            // 播放背景音乐
            PlayBgm(tavernBgm);
        }

        // ── 公共 API ────────────────────────────────────────────────

        public void PlayClick()        => PlayUI(clickSound);
        public void PlaySend()         => PlayUI(sendSound);
        public void PlayReceive()      => PlayUI(receiveSound);
        public void PlayBuy()          => PlayUI(buySound);
        public void PlayError()        => PlayUI(errorSound);
        public void PlayNotification() => PlayUI(notificationSound);

        public void PlayUI(AudioClip clip)
        {
            if (clip == null || uiSource == null) return;
            uiSource.PlayOneShot(clip, sfxVolume * masterVolume);
        }

        public void PlaySfx(AudioClip clip, float volumeScale = 1f)
        {
            if (clip == null || sfxSource == null) return;
            sfxSource.PlayOneShot(clip, sfxVolume * masterVolume * volumeScale);
        }

        public void PlayBgm(AudioClip clip)
        {
            if (bgmSource == null) return;
            if (bgmSource.clip == clip && bgmSource.isPlaying) return;
            bgmSource.clip = clip;
            bgmSource.loop = true;
            bgmSource.volume = bgmVolume * masterVolume;
            if (clip != null) bgmSource.Play();
        }

        /// <summary>根据时段切换 BGM</summary>
        public void UpdateBgmForTime(string timePhase)
        {
            bool isNight = timePhase == "夜晚" || timePhase == "深夜";
            PlayBgm(isNight ? nightBgm : tavernBgm);
        }

        // ── 音量控制 ────────────────────────────────────────────────

        public float MasterVolume
        {
            get => masterVolume;
            set { masterVolume = Mathf.Clamp01(value); ApplyVolumes(); SaveVolumes(); }
        }

        public float BgmVolume
        {
            get => bgmVolume;
            set { bgmVolume = Mathf.Clamp01(value); ApplyVolumes(); SaveVolumes(); }
        }

        public float SfxVolume
        {
            get => sfxVolume;
            set { sfxVolume = Mathf.Clamp01(value); ApplyVolumes(); SaveVolumes(); }
        }

        private void ApplyVolumes()
        {
            if (bgmSource != null) bgmSource.volume = bgmVolume * masterVolume;
        }

        private void SaveVolumes()
        {
            BlueprintStorage.SetFloat("audio_master", masterVolume);
            BlueprintStorage.SetFloat("audio_bgm", bgmVolume);
            BlueprintStorage.SetFloat("audio_sfx", sfxVolume);
        }
    }
}
