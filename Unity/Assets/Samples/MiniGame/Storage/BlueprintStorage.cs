// BlueprintStorage.cs
// ─────────────────────────────────────────────────────────────────────────────
// 蓝图数据持久化 — 跨平台统一接口
//
// 平台自动分派：
//   UNITY_EDITOR / UNITY_STANDALONE → PlayerPrefs（注册表 / plist / ini）
//   UNITY_WEBGL 纯浏览器         → localStorage（通过 jslib）
//   UNITY_WEBGL 转微信小游戏       → wx.setStorageSync / getStorageSync（通过 jslib）
//
// 使用：
//   BlueprintStorage.SetString("NPC_MeowMeow.affinity", "50");
//   string v = BlueprintStorage.GetString("NPC_MeowMeow.affinity", "0");
//   BlueprintStorage.SetJson("NPC_MeowMeow.history", historyObj);  // 自动 JsonUtility 序列化
//   var h = BlueprintStorage.GetJson<ChatHistory>("NPC_MeowMeow.history");
//
// 命名规范（推荐）：
//   "NPC_{name}.{field}"     per-NPC 数据（好感度、最后对话时间等）
//   "NPC_{name}.history"     对话历史 JSON
//   "Global.{field}"         全局游戏状态（玩家名、金币、当前任务等）
//
// 容量限制：
//   PlayerPrefs     无限（磁盘文件大小而已）
//   localStorage    ~5MB 共享
//   wx.setStorage   单 key 1MB，单账号总 10MB
// ─────────────────────────────────────────────────────────────────────────────

using System;
using UnityEngine;

#if UNITY_WEBGL && !UNITY_EDITOR
using System.Runtime.InteropServices;
#endif

namespace BlueprintRuntime.Samples.MiniGame
{
    public static class BlueprintStorage
    {
#if UNITY_WEBGL && !UNITY_EDITOR
        // jslib 绑定（对应 Plugins/WebGL/BlueprintStorage.jslib）
        [DllImport("__Internal")] private static extern void BPStorage_SetString(string key, string value);
        [DllImport("__Internal")] private static extern string BPStorage_GetString(string key, string fallback);
        [DllImport("__Internal")] private static extern int BPStorage_HasKey(string key);
        [DllImport("__Internal")] private static extern void BPStorage_Remove(string key);
        [DllImport("__Internal")] private static extern void BPStorage_Clear();
#endif

        // ── 基础字符串 API ────────────────────────────────────────────
        public static void SetString(string key, string value)
        {
            if (string.IsNullOrEmpty(key)) return;
            value = value ?? "";
#if UNITY_WEBGL && !UNITY_EDITOR
            BPStorage_SetString(key, value);
#else
            PlayerPrefs.SetString(key, value);
            PlayerPrefs.Save();
#endif
        }

        public static string GetString(string key, string fallback = "")
        {
            if (string.IsNullOrEmpty(key)) return fallback;
#if UNITY_WEBGL && !UNITY_EDITOR
            return BPStorage_GetString(key, fallback ?? "");
#else
            return PlayerPrefs.GetString(key, fallback ?? "");
#endif
        }

        public static bool HasKey(string key)
        {
            if (string.IsNullOrEmpty(key)) return false;
#if UNITY_WEBGL && !UNITY_EDITOR
            return BPStorage_HasKey(key) != 0;
#else
            return PlayerPrefs.HasKey(key);
#endif
        }

        public static void Remove(string key)
        {
            if (string.IsNullOrEmpty(key)) return;
#if UNITY_WEBGL && !UNITY_EDITOR
            BPStorage_Remove(key);
#else
            PlayerPrefs.DeleteKey(key);
            PlayerPrefs.Save();
#endif
        }

        public static void Clear()
        {
#if UNITY_WEBGL && !UNITY_EDITOR
            BPStorage_Clear();
#else
            PlayerPrefs.DeleteAll();
            PlayerPrefs.Save();
#endif
        }

        // ── 数值便捷方法 ──────────────────────────────────────────────
        public static int  GetInt  (string key, int    fallback = 0)     => int.TryParse(GetString(key, fallback.ToString()), out var v) ? v : fallback;
        public static void SetInt  (string key, int    value)            => SetString(key, value.ToString(System.Globalization.CultureInfo.InvariantCulture));

        public static float GetFloat(string key, float fallback = 0f)    => float.TryParse(GetString(key, fallback.ToString(System.Globalization.CultureInfo.InvariantCulture)),
                                                                              System.Globalization.NumberStyles.Float,
                                                                              System.Globalization.CultureInfo.InvariantCulture, out var v) ? v : fallback;
        public static void SetFloat(string key, float value)             => SetString(key, value.ToString("R", System.Globalization.CultureInfo.InvariantCulture));

        public static bool GetBool (string key, bool   fallback = false) => GetString(key, fallback ? "1" : "0") == "1";
        public static void SetBool (string key, bool   value)            => SetString(key, value ? "1" : "0");

        // ── JSON 对象 ────────────────────────────────────────────────
        public static void SetJson<T>(string key, T obj) where T : class
        {
            if (obj == null) { Remove(key); return; }
            SetString(key, JsonUtility.ToJson(obj));
        }

        public static T GetJson<T>(string key) where T : class, new()
        {
            var s = GetString(key, "");
            if (string.IsNullOrEmpty(s)) return null;
            try { return JsonUtility.FromJson<T>(s); }
            catch (Exception e) { Debug.LogWarning($"[BlueprintStorage] GetJson<{typeof(T).Name}> failed for '{key}': {e.Message}"); return null; }
        }
    }
}
