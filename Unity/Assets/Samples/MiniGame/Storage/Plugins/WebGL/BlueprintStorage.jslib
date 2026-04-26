// BlueprintStorage.jslib
// ─────────────────────────────────────────────────────────────────────────────
// Unity WebGL → localStorage / 微信小游戏 wx.storage 桥
//
// 自动检测运行环境：
//   - 有 wx.setStorageSync → 微信小游戏（适配器替换后的全局 API）
//   - 否则 → 浏览器 localStorage
//
// 放置路径：Assets/Plugins/WebGL/BlueprintStorage.jslib
// Unity 构建 WebGL 时自动合并到 build 的 JS 中。
// ─────────────────────────────────────────────────────────────────────────────

var BlueprintStorageLib = {
    // 使用带前缀的 key 避免与其他系统冲突
    $_bpStorage_prefix: "bp_",

    $_bpStorage_isWx: function() {
        return typeof wx !== 'undefined' &&
               wx && typeof wx.setStorageSync === 'function';
    },

    $_bpStorage_set__deps: ['$_bpStorage_prefix', '$_bpStorage_isWx'],
    $_bpStorage_set: function(key, value) {
        var k = _bpStorage_prefix + key;
        try {
            if (_bpStorage_isWx()) {
                wx.setStorageSync(k, value);
            } else if (typeof localStorage !== 'undefined') {
                localStorage.setItem(k, value);
            }
        } catch (e) {
            console.warn('[BPStorage] set failed:', e);
        }
    },

    $_bpStorage_get__deps: ['$_bpStorage_prefix', '$_bpStorage_isWx'],
    $_bpStorage_get: function(key, fallback) {
        var k = _bpStorage_prefix + key;
        try {
            if (_bpStorage_isWx()) {
                var v = wx.getStorageSync(k);
                return (v === '' || v === undefined || v === null) ? fallback : v;
            } else if (typeof localStorage !== 'undefined') {
                var v2 = localStorage.getItem(k);
                return v2 === null ? fallback : v2;
            }
        } catch (e) {
            console.warn('[BPStorage] get failed:', e);
        }
        return fallback;
    },

    BPStorage_SetString__deps: ['$_bpStorage_set'],
    BPStorage_SetString: function(keyPtr, valuePtr) {
        var key   = UTF8ToString(keyPtr);
        var value = UTF8ToString(valuePtr);
        _bpStorage_set(key, value);
    },

    BPStorage_GetString__deps: ['$_bpStorage_get'],
    BPStorage_GetString: function(keyPtr, fallbackPtr) {
        var key      = UTF8ToString(keyPtr);
        var fallback = UTF8ToString(fallbackPtr);
        var value    = _bpStorage_get(key, fallback);
        // 返回字符串：Emscripten 要求我们 malloc 并返回指针
        var bufSize = lengthBytesUTF8(value) + 1;
        var buf = _malloc(bufSize);
        stringToUTF8(value, buf, bufSize);
        return buf;
    },

    BPStorage_HasKey__deps: ['$_bpStorage_prefix', '$_bpStorage_isWx'],
    BPStorage_HasKey: function(keyPtr) {
        var key = UTF8ToString(keyPtr);
        var k = _bpStorage_prefix + key;
        try {
            if (_bpStorage_isWx()) {
                var info = wx.getStorageInfoSync();
                return (info.keys && info.keys.indexOf(k) >= 0) ? 1 : 0;
            } else if (typeof localStorage !== 'undefined') {
                return (localStorage.getItem(k) !== null) ? 1 : 0;
            }
        } catch (e) {}
        return 0;
    },

    BPStorage_Remove__deps: ['$_bpStorage_prefix', '$_bpStorage_isWx'],
    BPStorage_Remove: function(keyPtr) {
        var key = UTF8ToString(keyPtr);
        var k = _bpStorage_prefix + key;
        try {
            if (_bpStorage_isWx()) {
                wx.removeStorageSync(k);
            } else if (typeof localStorage !== 'undefined') {
                localStorage.removeItem(k);
            }
        } catch (e) {}
    },

    BPStorage_Clear__deps: ['$_bpStorage_prefix', '$_bpStorage_isWx'],
    BPStorage_Clear: function() {
        try {
            if (_bpStorage_isWx()) {
                // 仅清除我们 prefix 的 key，不影响其他系统
                var info = wx.getStorageInfoSync();
                if (info && info.keys) {
                    info.keys.forEach(function(k) {
                        if (k.indexOf(_bpStorage_prefix) === 0) wx.removeStorageSync(k);
                    });
                }
            } else if (typeof localStorage !== 'undefined') {
                var keys = [];
                for (var i = 0; i < localStorage.length; i++) {
                    var key = localStorage.key(i);
                    if (key && key.indexOf(_bpStorage_prefix) === 0) keys.push(key);
                }
                keys.forEach(function(k) { localStorage.removeItem(k); });
            }
        } catch (e) {
            console.warn('[BPStorage] clear failed:', e);
        }
    },
};

autoAddDeps(BlueprintStorageLib, '$_bpStorage_prefix');
autoAddDeps(BlueprintStorageLib, '$_bpStorage_isWx');
autoAddDeps(BlueprintStorageLib, '$_bpStorage_set');
autoAddDeps(BlueprintStorageLib, '$_bpStorage_get');
mergeInto(LibraryManager.library, BlueprintStorageLib);
