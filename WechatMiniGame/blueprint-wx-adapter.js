// blueprint-wx-adapter.js
// ─────────────────────────────────────────────────────────────────────────────
// 微信/抖音小游戏 Blueprint Runtime 接入适配器
//
// 职责：
//   1. 把 WASM 侧发起的 HTTP 请求桥接到 wx.request / tt.request
//   2. 提供统一的 BlueprintBridge JS API 给游戏代码使用
//   3. 处理字符串编解码、内存分配、回调注册
//
// 使用方式（在 Cocos / LayaAir 脚本里）：
//
//   import { BlueprintBridge } from './blueprint-wx-adapter.js';
//
//   await BlueprintBridge.init({
//       wasmUrl:   'https://your-cdn.com/BlueprintRuntime.wasm',
//       jsLoader:  'https://your-cdn.com/BlueprintRuntime.js',
//       host:      'wx',   // 'wx' | 'tt' | 'fetch'
//   });
//
//   const runner = BlueprintBridge.createRunner();
//   runner.onLog(msg => console.log('[BP]', msg));
//   runner.loadFromJson(jsonText);
//   runner.setVariable('PlayerName', 'Alice');
//   runner.execute();
//
// 注意：Runtime 编译参数必须包含：
//   -sEXPORTED_RUNTIME_METHODS=[ccall,cwrap,UTF8ToString,stringToUTF8,lengthBytesUTF8,addFunction,getValue,setValue]
//   -sEXPORTED_FUNCTIONS=[_malloc,_free,_BP_CreateRunner,_BP_DestroyRunner,_BP_LoadFromJson,_BP_Execute,_BP_Tick,_BP_SetVariableString,_BP_SetVariableInt,_BP_SetVariableFloat,_BP_SetVariableBool,_BP_GetVariableString,_BP_GetVariableInt,_BP_GetVariableFloat,_BP_GetVariableBool,_BP_SetLogCallback,_BP_SetPrintCallback,_BP_InstallJSBridgeHttpClient,_BP_Http_SetRequestDispatcher,_BP_Http_GetReqUrl,_BP_Http_GetReqMethod,_BP_Http_GetReqBody,_BP_Http_GetReqHeaders,_BP_Http_OnResponse,_BP_InitDefaultHttpClient]
//   -sALLOW_TABLE_GROWTH=1            // addFunction 需要
//   -sFETCH=1                          // 备用浏览器通道
//   -sASYNCIFY                         // 按需
// ─────────────────────────────────────────────────────────────────────────────

let Module = null;        // Emscripten Module
let _hostRequest = null;  // 被选中的 HTTP 后端：wx.request / tt.request / fetch 适配器

// ═════════════════════════════════════════════════════════════════════════════
// 内部：HTTP 请求派发（WASM → JS → wx.request → WASM）
// ═════════════════════════════════════════════════════════════════════════════

function dispatchRequest(reqId) {
    // 1. 从 WASM 拿请求字段
    const url     = Module.UTF8ToString(Module._BP_Http_GetReqUrl(reqId));
    const method  = (Module.UTF8ToString(Module._BP_Http_GetReqMethod(reqId)) || 'POST').toUpperCase();
    const body    = Module.UTF8ToString(Module._BP_Http_GetReqBody(reqId));
    const hdrRaw  = Module.UTF8ToString(Module._BP_Http_GetReqHeaders(reqId));

    // 2. 解析 headers
    const headers = {};
    if (hdrRaw) {
        const parts = hdrRaw.split('\n');
        for (let i = 0; i + 1 < parts.length; i += 2) {
            if (parts[i]) headers[parts[i]] = parts[i + 1] || '';
        }
    }
    if (!headers['Content-Type'] && !headers['content-type'] && body) {
        headers['Content-Type'] = 'application/json';
    }

    // 3. 交给宿主后端
    _hostRequest({ url, method, headers, body })
        .then(({ statusCode, body: respBody, error }) => {
            onResponse(reqId, statusCode | 0, respBody || '', error || '');
        })
        .catch(err => {
            onResponse(reqId, 0, '', String(err && err.message || err || 'request failed'));
        });
}

function onResponse(reqId, statusCode, bodyStr, errStr) {
    // 把 body/err 拷到 WASM heap
    const bodyPtr = bodyStr ? mallocUTF8(bodyStr) : 0;
    const errPtr  = errStr  ? mallocUTF8(errStr)  : 0;
    try {
        Module._BP_Http_OnResponse(reqId, statusCode, bodyPtr, errPtr);
    } finally {
        if (bodyPtr) Module._free(bodyPtr);
        if (errPtr)  Module._free(errPtr);
    }
}

function mallocUTF8(s) {
    const n = Module.lengthBytesUTF8(s) + 1;
    const p = Module._malloc(n);
    Module.stringToUTF8(s, p, n);
    return p;
}

// ═════════════════════════════════════════════════════════════════════════════
// HTTP 后端实现（按宿主环境切换）
// ═════════════════════════════════════════════════════════════════════════════

// 微信小游戏：wx.request
function wxBackend({ url, method, headers, body }) {
    return new Promise((resolve) => {
        wx.request({
            url, method, header: headers,
            data: body,
            dataType: 'text',      // 不要自动解析 JSON，保留原始字符串
            responseType: 'text',
            success(res) {
                const respBody = typeof res.data === 'string'
                    ? res.data
                    : JSON.stringify(res.data);
                resolve({ statusCode: res.statusCode, body: respBody });
            },
            fail(err) {
                resolve({ statusCode: 0, body: '', error: err && err.errMsg || 'wx.request fail' });
            },
        });
    });
}

// 抖音小游戏：tt.request
function ttBackend({ url, method, headers, body }) {
    return new Promise((resolve) => {
        tt.request({
            url, method, header: headers,
            data: body,
            dataType: 'text',
            success(res) {
                const respBody = typeof res.data === 'string'
                    ? res.data
                    : JSON.stringify(res.data);
                resolve({ statusCode: res.statusCode, body: respBody });
            },
            fail(err) {
                resolve({ statusCode: 0, body: '', error: err && err.errMsg || 'tt.request fail' });
            },
        });
    });
}

// 浏览器：fetch
function fetchBackend({ url, method, headers, body }) {
    return fetch(url, {
        method,
        headers,
        body: (method === 'GET' || method === 'HEAD') ? undefined : body,
    }).then(async (res) => {
        const text = await res.text();
        return { statusCode: res.status, body: text };
    }).catch(err => ({ statusCode: 0, body: '', error: String(err) }));
}

// ═════════════════════════════════════════════════════════════════════════════
// 对外 API
// ═════════════════════════════════════════════════════════════════════════════

export const BlueprintBridge = {
    /**
     * 初始化 Runtime。
     * @param {object} opts
     * @param {string} opts.wasmUrl   BlueprintRuntime.wasm URL
     * @param {string} opts.jsLoader  Emscripten 生成的 BlueprintRuntime.js
     * @param {'wx'|'tt'|'fetch'|Function} [opts.host='wx']
     *        HTTP 后端；传函数则作为自定义 backend
     */
    async init(opts) {
        // 选择 HTTP 后端
        if (typeof opts.host === 'function') {
            _hostRequest = opts.host;
        } else {
            switch (opts.host || 'wx') {
                case 'wx':    _hostRequest = wxBackend;    break;
                case 'tt':    _hostRequest = ttBackend;    break;
                case 'fetch': _hostRequest = fetchBackend; break;
                default: throw new Error('Unknown host: ' + opts.host);
            }
        }

        // 加载 Emscripten 模块（开发者自行引入 <script> 或 import）
        if (!opts.moduleFactory) {
            throw new Error('opts.moduleFactory is required (Emscripten Module factory)');
        }
        Module = await opts.moduleFactory({
            locateFile: (path) => opts.wasmUrl || path,
        });

        // 注册 dispatcher（C 函数指针）
        // 'vi' = void func(int32)
        const fnPtr = Module.addFunction(dispatchRequest, 'vi');
        Module._BP_Http_SetRequestDispatcher(fnPtr);

        // 安装 JSBridge HttpClient
        Module._BP_InstallJSBridgeHttpClient();
    },

    /** 创建 Runner */
    createRunner() {
        if (!Module) throw new Error('BlueprintBridge not initialized');
        const handle = Module._BP_CreateRunner();
        if (!handle) throw new Error('BP_CreateRunner returned null');
        return new Runner(handle);
    },
};

// ═════════════════════════════════════════════════════════════════════════════
// Runner 包装器
// ═════════════════════════════════════════════════════════════════════════════

class Runner {
    constructor(handle) {
        this.handle = handle;
        this._logFnPtr = 0;
        this._printFnPtr = 0;
    }

    loadFromJson(jsonStr) {
        const p = mallocUTF8(jsonStr);
        try { return !Module._BP_LoadFromJson(this.handle, p); }  // C: 0=success
        finally { Module._free(p); }
    }

    execute() {
        return !Module._BP_Execute(this.handle);
    }

    tick(deltaTime) {
        Module._BP_Tick(this.handle, deltaTime);
    }

    // ── Variables ────────────────────────────────────────────────
    setVariable(name, value) {
        const np = mallocUTF8(name);
        try {
            if (typeof value === 'number') {
                if (Number.isInteger(value)) {
                    // i32 截断；大整数请用 setVariableBigInt
                    Module._BP_SetVariableInt(this.handle, np, value, 0);
                } else {
                    Module._BP_SetVariableFloat(this.handle, np, value);
                }
            } else if (typeof value === 'boolean') {
                Module._BP_SetVariableBool(this.handle, np, value ? 1 : 0);
            } else {
                const vp = mallocUTF8(String(value));
                try { Module._BP_SetVariableString(this.handle, np, vp); }
                finally { Module._free(vp); }
            }
        } finally { Module._free(np); }
    }

    getVariableString(name) {
        const np = mallocUTF8(name);
        const bufLen = 4096;
        const buf = Module._malloc(bufLen);
        try {
            Module._BP_GetVariableString(this.handle, np, buf, bufLen);
            return Module.UTF8ToString(buf);
        } finally {
            Module._free(np);
            Module._free(buf);
        }
    }

    getVariableInt(name) {
        const np = mallocUTF8(name);
        try { return Module._BP_GetVariableInt(this.handle, np); }
        finally { Module._free(np); }
    }

    getVariableFloat(name) {
        const np = mallocUTF8(name);
        try { return Module._BP_GetVariableFloat(this.handle, np); }
        finally { Module._free(np); }
    }

    getVariableBool(name) {
        const np = mallocUTF8(name);
        try { return !!Module._BP_GetVariableBool(this.handle, np); }
        finally { Module._free(np); }
    }

    // ── Logging ──────────────────────────────────────────────────
    onLog(cb) {
        const fn = (level, msgPtr) => {
            try { cb(Module.UTF8ToString(msgPtr), level); }
            catch (e) { console.error(e); }
        };
        this._logFnPtr = Module.addFunction(fn, 'vii');
        Module._BP_SetLogCallback(this.handle, this._logFnPtr);
    }

    /** PrintString 节点输出回调（游戏 UI 直接拿这个显示文本） */
    onPrint(cb) {
        const fn = (level, msgPtr) => {
            try { cb(Module.UTF8ToString(msgPtr), level); }
            catch (e) { console.error(e); }
        };
        this._printFnPtr = Module.addFunction(fn, 'vii');
        Module._BP_SetPrintCallback(this.handle, this._printFnPtr);
    }

    destroy() {
        if (this.handle) {
            Module._BP_DestroyRunner(this.handle);
            this.handle = 0;
        }
        if (this._logFnPtr)   { Module.removeFunction(this._logFnPtr);   this._logFnPtr = 0; }
        if (this._printFnPtr) { Module.removeFunction(this._printFnPtr); this._printFnPtr = 0; }
    }
}
