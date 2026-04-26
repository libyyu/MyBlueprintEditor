// WeChatSDK.jslib
// ─────────────────────────────────────────────────────────────────────────────
// Unity WebGL → 微信小游戏 wx.* API 桥
// 放置路径：Assets/Plugins/WebGL/WeChatSDK.jslib
//
// 注意：
//   - 微信 Unity WebGL 转换工具会在运行时注入全局 wx 对象
//   - 浏览器 WebGL 下 wx 未定义，所有 API 都应降级为 no-op
// ─────────────────────────────────────────────────────────────────────────────

var WeChatSDKLib = {
    $_wx_isAvailable: function() {
        return (typeof wx !== 'undefined') && wx;
    },

    WX_IsAvailable__deps: ['$_wx_isAvailable'],
    WX_IsAvailable: function() {
        return _wx_isAvailable() ? 1 : 0;
    },

    // ── 分享 ───────────────────────────────────────────────────────
    WX_ShareAppMessage__deps: ['$_wx_isAvailable'],
    WX_ShareAppMessage: function(titlePtr, imageUrlPtr, queryPtr) {
        if (!_wx_isAvailable() || !wx.shareAppMessage) return;
        try {
            wx.shareAppMessage({
                title:    UTF8ToString(titlePtr),
                imageUrl: UTF8ToString(imageUrlPtr) || undefined,
                query:    UTF8ToString(queryPtr)    || undefined,
            });
        } catch (e) { console.warn('[WX] share failed:', e); }
    },

    // ── 登录 ───────────────────────────────────────────────────────
    WX_Login__deps: ['$_wx_isAvailable'],
    WX_Login: function(goPtr, methodPtr) {
        var go     = UTF8ToString(goPtr);
        var method = UTF8ToString(methodPtr);
        if (!_wx_isAvailable() || !wx.login) {
            SendMessage(go, method, '{"code":"","error":"wx.login unavailable"}');
            return;
        }
        wx.login({
            success: function(res) {
                var payload = JSON.stringify({ code: res.code || '' });
                SendMessage(go, method, payload);
            },
            fail: function(err) {
                var payload = JSON.stringify({ code: '', error: (err && err.errMsg) || 'login fail' });
                SendMessage(go, method, payload);
            },
        });
    },

    // ── Toast ──────────────────────────────────────────────────────
    WX_ShowToast__deps: ['$_wx_isAvailable'],
    WX_ShowToast: function(textPtr, iconPtr, durationMs) {
        if (!_wx_isAvailable() || !wx.showToast) return;
        try {
            wx.showToast({
                title:    UTF8ToString(textPtr),
                icon:     UTF8ToString(iconPtr) || 'none',
                duration: durationMs,
            });
        } catch (e) { console.warn('[WX] showToast failed:', e); }
    },

    // ── 激励视频广告 ───────────────────────────────────────────────
    $_wx_rewardedAd: null,
    $_wx_rewardedCallback: null,

    WX_CreateRewardedVideoAd__deps: ['$_wx_isAvailable', '$_wx_rewardedAd', '$_wx_rewardedCallback'],
    WX_CreateRewardedVideoAd: function(adUnitIdPtr, goPtr, methodPtr) {
        var adUnitId = UTF8ToString(adUnitIdPtr);
        var go       = UTF8ToString(goPtr);
        var method   = UTF8ToString(methodPtr);

        if (!_wx_isAvailable() || !wx.createRewardedVideoAd) {
            SendMessage(go, method, 'error:wx.createRewardedVideoAd unavailable');
            return;
        }

        try {
            _wx_rewardedCallback = function(res) {
                var result = (res && res.isEnded) ? 'ok' : 'close';
                SendMessage(go, method, result);
            };

            _wx_rewardedAd = wx.createRewardedVideoAd({ adUnitId: adUnitId });
            _wx_rewardedAd.onClose(_wx_rewardedCallback);
            _wx_rewardedAd.onError(function(err) {
                SendMessage(go, method, 'error:' + (err && err.errMsg || 'unknown'));
            });
            // 提前 load 一次
            _wx_rewardedAd.load().catch(function(e) {
                console.warn('[WX] rewardedAd.load failed', e);
            });
        } catch (e) {
            SendMessage(go, method, 'error:' + e.message);
        }
    },

    WX_ShowRewardedVideoAd__deps: ['$_wx_isAvailable', '$_wx_rewardedAd'],
    WX_ShowRewardedVideoAd: function() {
        if (!_wx_rewardedAd) {
            console.warn('[WX] rewardedAd not preloaded');
            return;
        }
        _wx_rewardedAd.show().catch(function() {
            // 播放失败时先 load 再 show
            _wx_rewardedAd.load().then(function() {
                _wx_rewardedAd.show();
            }).catch(function(e) {
                console.warn('[WX] rewardedAd.show failed', e);
            });
        });
    },
};

autoAddDeps(WeChatSDKLib, '$_wx_isAvailable');
autoAddDeps(WeChatSDKLib, '$_wx_rewardedAd');
autoAddDeps(WeChatSDKLib, '$_wx_rewardedCallback');
mergeInto(LibraryManager.library, WeChatSDKLib);
