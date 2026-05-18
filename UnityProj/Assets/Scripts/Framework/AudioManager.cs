// AudioManager.cs
// 音频管理器 — 全局单例，提供 SFX 和 BGM 播放能力
//
// 设计：
//   - SFX：直接 AudioSource.PlayOneShot，不限数量
//   - BGM：淡入淡出切换，同时只播一首
//   - YooAsset 按需加载 AudioClip（PlaySFX/PlayBGM 触发异步加载）
//   - 蓝图通过 Audio.Play / Audio.PlayBGM / Audio.Stop 节点调用
//
// 初始化：
//   GameLauncher 场景里作为 DontDestroyOnLoad 对象挂载，
//   或由 GameLauncher.cs 在 Awake 里通过 AddComponent 创建。

using System;
using System.Collections;
using UnityEngine;
using YooAsset;
using XLua;

namespace CutRope.Framework
{
    [LuaCallCSharp]
    public class AudioManager : MonoBehaviour
    {
        // ── 单例 ─────────────────────────────────────────────────────
        public static AudioManager Instance { get; private set; }

        // ── Inspector 配置 ────────────────────────────────────────────
        [Header("SFX")]
        [Tooltip("同时最多播放几个音效（超出时丢弃最旧）")]
        public int sfxPoolSize = 8;

        [Header("BGM")]
        public float defaultFadeTime = 0.5f;

        // ── 内部状态 ──────────────────────────────────────────────────
        private AudioSource[] _sfxPool;
        private int           _sfxIdx;
        private AudioSource   _bgmSource;
        private Coroutine     _bgmFadeRoutine;

        // ── 生命周期 ──────────────────────────────────────────────────

        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
            DontDestroyOnLoad(gameObject);

            // SFX 音源池
            _sfxPool = new AudioSource[sfxPoolSize];
            for (int i = 0; i < sfxPoolSize; i++)
            {
                var go = new GameObject($"SFX_{i}");
                go.transform.SetParent(transform);
                _sfxPool[i] = go.AddComponent<AudioSource>();
                _sfxPool[i].playOnAwake = false;
            }

            // BGM 音源
            var bgmGo = new GameObject("BGM");
            bgmGo.transform.SetParent(transform);
            _bgmSource = bgmGo.AddComponent<AudioSource>();
            _bgmSource.loop       = true;
            _bgmSource.playOnAwake = false;
        }

        private void OnDestroy()
        {
            if (Instance == this) Instance = null;
        }

        // ── 公共 API ──────────────────────────────────────────────────

        /// <summary>
        /// 播放音效（YooAsset address，如 "Audio/cut"）
        /// </summary>
        public void PlaySFX(string address, float volume = 1f)
        {
            if (string.IsNullOrEmpty(address)) return;
            StartCoroutine(LoadAndPlaySFX(address, volume));
        }

        /// <summary>
        /// 播放背景音乐（淡入替换当前 BGM）
        /// </summary>
        public void PlayBGM(string address, float fadeTime = -1f)
        {
            if (fadeTime < 0) fadeTime = defaultFadeTime;
            if (string.IsNullOrEmpty(address)) return;
            StartCoroutine(LoadAndPlayBGM(address, fadeTime));
        }

        /// <summary>
        /// 停止背景音乐（淡出）
        /// </summary>
        public void StopBGM(float fadeTime = -1f)
        {
            if (fadeTime < 0) fadeTime = defaultFadeTime;
            if (_bgmFadeRoutine != null) StopCoroutine(_bgmFadeRoutine);
            _bgmFadeRoutine = StartCoroutine(FadeOut(_bgmSource, fadeTime));
        }

        // ── 内部协程 ──────────────────────────────────────────────────

        private IEnumerator LoadAndPlaySFX(string address, float volume)
        {
            var package = YooAssets.GetPackage("DefaultPackage");
            if (package == null) yield break;

            var handle = package.LoadAssetAsync<AudioClip>(address);
            yield return handle;

            if (handle.Status != EOperationStatus.Succeed)
            {
                Debug.LogWarning($"[AudioManager] SFX not found: {address}");
                yield break;
            }

            var clip = handle.AssetObject as AudioClip;
            if (clip == null) yield break;

            // 从池里找空闲 AudioSource
            var src = _sfxPool[_sfxIdx % sfxPoolSize];
            _sfxIdx++;
            src.volume = volume;
            src.PlayOneShot(clip);
            // 延迟释放（等播放结束）
            StartCoroutine(ReleaseAfterPlay(handle, clip.length + 0.1f));
        }

        private IEnumerator LoadAndPlayBGM(string address, float fadeTime)
        {
            var package = YooAssets.GetPackage("DefaultPackage");
            if (package == null) yield break;

            var handle = package.LoadAssetAsync<AudioClip>(address);
            yield return handle;

            if (handle.Status != EOperationStatus.Succeed)
            {
                Debug.LogWarning($"[AudioManager] BGM not found: {address}");
                yield break;
            }

            var clip = handle.AssetObject as AudioClip;
            if (clip == null) yield break;

            if (_bgmFadeRoutine != null) StopCoroutine(_bgmFadeRoutine);

            // 淡出当前 → 换曲 → 淡入
            if (_bgmSource.isPlaying)
                yield return FadeOut(_bgmSource, fadeTime * 0.5f);

            _bgmSource.clip   = clip;
            _bgmSource.volume = 0f;
            _bgmSource.Play();
            _bgmFadeRoutine = StartCoroutine(FadeIn(_bgmSource, fadeTime * 0.5f));
        }

        private IEnumerator FadeOut(AudioSource src, float duration)
        {
            float start = src.volume;
            float t = 0;
            while (t < duration)
            {
                t += Time.unscaledDeltaTime;
                src.volume = Mathf.Lerp(start, 0f, t / duration);
                yield return null;
            }
            src.Stop();
            src.volume = start;
        }

        private IEnumerator FadeIn(AudioSource src, float duration)
        {
            float t = 0;
            while (t < duration)
            {
                t += Time.unscaledDeltaTime;
                src.volume = Mathf.Lerp(0f, 1f, t / duration);
                yield return null;
            }
            src.volume = 1f;
        }

        private IEnumerator ReleaseAfterPlay(AssetHandle handle, float delay)
        {
            yield return new WaitForSeconds(delay);
            handle.Release();
        }
    }
}
