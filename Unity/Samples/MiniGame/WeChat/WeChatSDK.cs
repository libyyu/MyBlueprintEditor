// WeChatSDK.cs
// ─────────────────────────────────────────────────────────────────────────────
// 微信小游戏 wx.* API C# 适配层
//
// 覆盖最常用的 4 个 API（覆盖 80% 需求）：
//   1. wx.shareAppMessage   — 转发给好友（必备！带病毒传播）
//   2. wx.login             — 拿 openid（用户识别、存档）
//   3. wx.showToast         — 原生 Toast 提示（比 UGUI 好看）
//   4. wx.createRewardedVideoAd — 激励视频广告（看广告领奖励）
//
// 平台自动分派：
//   UNITY_WEBGL 且 wx 可用     → 真实 wx.* 调用
//   UNITY_EDITOR / Standalone  → 模拟实现（Debug.Log / 模拟成功）
//
// 配合蓝图节点使用：
//   可注册一个 "WeChat.Share" Handler，蓝图里调 Share 节点触发
//   见文件末尾的 BlueprintWeChatNodes
// ─────────────────────────────────────────────────────────────────────────────

using System;
using UnityEngine;

#if UNITY_WEBGL && !UNITY_EDITOR
using System.Runtime.InteropServices;
#endif

namespace BlueprintRuntime.Samples.MiniGame.WeChat
{
    public static class WeChatSDK
    {
#if UNITY_WEBGL && !UNITY_EDITOR
        [DllImport("__Internal")] private static extern int  WX_IsAvailable();
        [DllImport("__Internal")] private static extern void WX_ShareAppMessage(string title, string imageUrl, string query);
        [DllImport("__Internal")] private static extern void WX_Login(string callbackGameObject, string callbackMethod);
        [DllImport("__Internal")] private static extern void WX_ShowToast(string text, string icon, int duration);
        [DllImport("__Internal")] private static extern void WX_CreateRewardedVideoAd(string adUnitId, string callbackGameObject, string callbackMethod);
        [DllImport("__Internal")] private static extern void WX_ShowRewardedVideoAd();
#endif

        /// <summary>是否处于微信小游戏环境（不是浏览器 WebGL，也不是 Editor）</summary>
        public static bool IsInWeChat
        {
            get
            {
#if UNITY_WEBGL && !UNITY_EDITOR
                try { return WX_IsAvailable() != 0; }
                catch { return false; }
#else
                return false;
#endif
            }
        }

        // ── 1. 分享给好友 ─────────────────────────────────────────────
        /// <summary>
        /// 分享当前游戏给好友。
        /// </summary>
        /// <param name="title">分享标题，建议 &lt; 20 字</param>
        /// <param name="imageUrl">分享封面图 URL（5:4 比例最佳），可空</param>
        /// <param name="query">启动参数（好友点击后用 wx.getLaunchOptionsSync 拿到）</param>
        public static void Share(string title, string imageUrl = "", string query = "")
        {
#if UNITY_WEBGL && !UNITY_EDITOR
            if (IsInWeChat)
            {
                WX_ShareAppMessage(title ?? "", imageUrl ?? "", query ?? "");
                return;
            }
#endif
            Debug.Log($"[WeChatSDK.Share/mock] title={title} img={imageUrl} query={query}");
        }

        // ── 2. 登录获取 openid ───────────────────────────────────────
        /// <summary>
        /// 发起微信登录。完成后 UnityEngine 会通过 SendMessage 回调指定对象。
        /// 回调方法签名：void OnWxLogin(string json) — json 内含 code 字段。
        /// </summary>
        public static void Login(GameObject callbackTarget, string callbackMethod)
        {
#if UNITY_WEBGL && !UNITY_EDITOR
            if (IsInWeChat && callbackTarget != null)
            {
                WX_Login(callbackTarget.name, callbackMethod);
                return;
            }
#endif
            // Mock：立即回调一个假 code
            if (callbackTarget != null)
                callbackTarget.SendMessage(callbackMethod,
                    "{\"code\":\"MOCK_LOGIN_CODE_" + DateTime.UtcNow.Ticks + "\"}",
                    SendMessageOptions.DontRequireReceiver);
        }

        // ── 3. 原生 Toast ─────────────────────────────────────────────
        public enum ToastIcon { Success, Error, Loading, None }

        public static void ShowToast(string text, ToastIcon icon = ToastIcon.None, int durationMs = 1500)
        {
#if UNITY_WEBGL && !UNITY_EDITOR
            if (IsInWeChat)
            {
                WX_ShowToast(text ?? "", IconToStr(icon), durationMs);
                return;
            }
#endif
            Debug.Log($"[WeChatSDK.Toast/mock] {icon} {text} ({durationMs}ms)");
        }

        private static string IconToStr(ToastIcon i) => i switch
        {
            ToastIcon.Success => "success",
            ToastIcon.Error   => "error",
            ToastIcon.Loading => "loading",
            _                 => "none",
        };

        // ── 4. 激励视频广告 ───────────────────────────────────────────
        private static string s_rewardAdUnitId;
        private static GameObject s_rewardCallbackTarget;
        private static string s_rewardCallbackMethod;

        /// <summary>
        /// 预创建激励视频广告（一次游戏生命周期调一次即可）。
        /// 回调方法签名：void OnRewardedAd(string result) — result="ok"/"close"/"error:..."
        /// </summary>
        public static void PreloadRewardedVideoAd(string adUnitId, GameObject callbackTarget, string callbackMethod)
        {
            s_rewardAdUnitId         = adUnitId;
            s_rewardCallbackTarget   = callbackTarget;
            s_rewardCallbackMethod   = callbackMethod;
#if UNITY_WEBGL && !UNITY_EDITOR
            if (IsInWeChat && callbackTarget != null)
            {
                WX_CreateRewardedVideoAd(adUnitId, callbackTarget.name, callbackMethod);
                return;
            }
#endif
        }

        /// <summary>
        /// 播放激励视频广告。玩家看完后 callbackMethod 会被回调 "ok"，取消则回调 "close"。
        /// 前提：已 PreloadRewardedVideoAd。
        /// </summary>
        public static void ShowRewardedVideoAd()
        {
#if UNITY_WEBGL && !UNITY_EDITOR
            if (IsInWeChat)
            {
                WX_ShowRewardedVideoAd();
                return;
            }
#endif
            // Mock：1 秒后假装玩家看完广告
            if (s_rewardCallbackTarget != null)
            {
                var target = s_rewardCallbackTarget;
                var method = s_rewardCallbackMethod;
                // 用 MonoBehaviour 不方便，直接同步发
                target.SendMessage(method, "ok", SendMessageOptions.DontRequireReceiver);
            }
        }
    }
}
