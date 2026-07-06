// blueprint-wx-fs.js
// ─────────────────────────────────────────────────────────────────────────────
// 小游戏文件读取桥 + 热更 Updater
//
// 两件事：
//   1. installFileReader(Module, opts)
//      把 wasm 侧 BP_SetFileReader 的三个回调(read/free/exists)桥接到
//      小游戏的 wx.getFileSystemManager()（同步 API：readFileSync / accessSync）。
//      装好后 Runtime 内部加载 .bjson / require 资源会透明读到 wx 用户目录里的文件，
//      与 Native 的 Godot FileAccess 文件桥同一套设计，只是后端换成 wx FS。
//
//   2. MiniGameUpdater
//      小游戏版热更：拉 version.json → 复用平台无关决策 → wx.downloadFile 下变动条目
//      → 校验 → 落 wx 用户目录 → runner.loadFromJson 重载（等价 Native 的"挂载后生效"）。
//
// 关键前提（wasm 编译参数需额外导出）：
//   -sEXPORTED_FUNCTIONS 追加：_BP_SetFileReader
//   已有的 _malloc/_free/addFunction/stringToUTF8/UTF8ToString/getValue/setValue 复用。
//
// 平台差异：微信=wx，抖音=tt（两者 FileSystemManager API 基本同名）。默认取全局 wx。
// ─────────────────────────────────────────────────────────────────────────────

// ── 环境抽象：拿到当前小游戏平台的 FS + 用户目录 ────────────────────────────
function _env(host) {
    const g = (typeof host === 'object' && host) ? host
            : (typeof wx !== 'undefined') ? wx
            : (typeof tt !== 'undefined') ? tt
            : null;
    if (!g) throw new Error('[bp-wx-fs] no wx/tt global found');
    return {
        g,
        fs: g.getFileSystemManager(),
        // 用户可写根目录（持久化）。微信/抖音都提供 env.USER_DATA_PATH
        userDir: (g.env && g.env.USER_DATA_PATH) || `${g.env ? '' : ''}wxfile://usr`,
    };
}

// path 规整：把 Runtime 传来的逻辑路径映射到 wx 用户目录下的真实路径。
// 约定：Runtime 内部用相对/res 风格路径，这里统一挂到 userDir 下。
function _resolve(userDir, path) {
    let p = String(path || '');
    if (p.startsWith('wxfile://') || p.startsWith('ttfile://')) return p;
    // 去掉 res:// user:// 前缀
    p = p.replace(/^res:\/\//, '').replace(/^user:\/\//, '');
    while (p.startsWith('./')) p = p.slice(2);
    if (p.startsWith('/')) p = p.slice(1);
    return `${userDir}/${p}`;
}

// ═════════════════════════════════════════════════════════════════════════════
// 1) 文件读取桥
// ═════════════════════════════════════════════════════════════════════════════
//
// installFileReader(Module, { host?, userDir?, extraRoots? })
//   host: 'wx'|'tt'|自定义全局对象；缺省自动探测
//   userDir: 覆盖默认用户目录
//   extraRoots: 额外只读根（如随包资源目录），读不到 userDir 时按序回退
//
// 返回 { readFnPtr, existsFnPtr, freeFnPtr, uninstall() } 便于清理。
export function installFileReader(Module, opts = {}) {
    const { fs, userDir: envUserDir } = _env(opts.host);
    const userDir = opts.userDir || envUserDir;
    const extraRoots = Array.isArray(opts.extraRoots) ? opts.extraRoots : [];

    // 候选路径：userDir 优先，其次 extraRoots（随包只读资源）
    function candidates(path) {
        const list = [_resolve(userDir, path)];
        for (const root of extraRoots) list.push(_resolve(root, path));
        return list;
    }

    // read(path, outDataPtrPtr, outSizePtr, ud) -> 1 成功 / 0 失败
    // 成功时：malloc 一块 wasm 堆内存，把文件字节拷进去，写回 *outData / *outSize
    function read(pathPtr, outDataPtrPtr, outSizePtr, _ud) {
        const path = Module.UTF8ToString(pathPtr);
        for (const real of candidates(path)) {
            try {
                // 二进制安全：encoding 不传 → 返回 ArrayBuffer
                const ab = fs.readFileSync(real);
                const bytes = new Uint8Array(ab);
                const n = bytes.length;
                const buf = Module._malloc(n > 0 ? n : 1);
                if (!buf) return 0;
                Module.HEAPU8.set(bytes, buf);
                // 写回 char** outData（指针宽度 32 位，wasm32）
                Module.setValue(outDataPtrPtr, buf, 'i32');
                Module.setValue(outSizePtr, n, 'i32');
                return 1;
            } catch (_e) {
                // 试下一个候选路径
            }
        }
        return 0;
    }

    // free(data, ud) —— 释放 read 里 malloc 的内存
    function free(dataPtr, _ud) {
        if (dataPtr) Module._free(dataPtr);
    }

    // exists(path, ud) -> 1 / 0
    function exists(pathPtr, _ud) {
        const path = Module.UTF8ToString(pathPtr);
        for (const real of candidates(path)) {
            try {
                fs.accessSync(real);   // 不抛即存在
                return 1;
            } catch (_e) { /* next */ }
        }
        return 0;
    }

    // 注册函数指针。签名见 BlueprintCAPI.h：
    //   read:   int(const char*, char**, int*, void*)  -> 'iiiii'
    //   free:   void(char*, void*)                     -> 'vii'
    //   exists: int(const char*, void*)                -> 'iii'
    const readFnPtr   = Module.addFunction(read,   'iiiii');
    const freeFnPtr   = Module.addFunction(free,   'vii');
    const existsFnPtr = Module.addFunction(exists, 'iii');

    Module._BP_SetFileReader(readFnPtr, freeFnPtr, existsFnPtr, 0);

    return {
        readFnPtr, freeFnPtr, existsFnPtr,
        uninstall() {
            // 恢复内建 FS（read=NULL），再回收函数指针
            Module._BP_SetFileReader(0, 0, 0, 0);
            Module.removeFunction(readFnPtr);
            Module.removeFunction(freeFnPtr);
            Module.removeFunction(existsFnPtr);
        },
    };
}

// ═════════════════════════════════════════════════════════════════════════════
// 2) 小游戏热更 Updater
// ═════════════════════════════════════════════════════════════════════════════
//
// 决策层复用与 Native 同一套逻辑（见 game/framework/update/UpdatePlanner.gd 的等价实现）。
// 这里用 JS 重写同样的决策，保证三端一致；物理层走 wx.downloadFile + FS。
//
// 用法：
//   const up = new MiniGameUpdater({
//       host: 'wx',
//       versionUrl: 'https://cdn/xxx/version.json',
//       clientApiLevel: 102,
//       clientVersion: '1.2.0',
//       platformKey: 'wx',
//       onProgress: (p, s) => {...},
//   });
//   const result = await up.run();
//   if (result.needStoreUpgrade) { /* 提示更新小游戏版本 */ }
//   else if (result.updated || result.upToDate) {
//       runner.loadFromJson(up.readEntryText(mainBjsonName));
//   }

export class MiniGameUpdater {
    constructor(opts) {
        this.opts = opts;
        const env = _env(opts.host);
        this.fs = env.fs;
        this.userDir = opts.userDir || env.userDir;
        this.patchDir = `${this.userDir}/patches`;
        this.localVerFile = `${this.userDir}/installed_version.txt`;
        this._ensureDir(this.patchDir);
    }

    // ── 决策层（对齐 UpdatePlanner.gd）──────────────────────────────
    _semverGe(v1, v2) {
        const a = String(v1).split('.'), b = String(v2).split('.');
        const n = Math.max(a.length, b.length);
        for (let i = 0; i < n; i++) {
            const pa = parseInt(a[i] || '0', 10), pb = parseInt(b[i] || '0', 10);
            if (pa > pb) return true;
            if (pa < pb) return false;
        }
        return true;
    }

    async _plan(manifest) {
        const p = {
            ok: false, needStoreUpgrade: false, storeUrl: '',
            upToDate: false, remoteVersion: '', toDownload: [], skipped: [],
            allEntries: [], reason: '',
        };
        if (!manifest || typeof manifest !== 'object') { p.reason = 'empty_manifest'; return p; }
        p.ok = true;
        p.remoteVersion = String(manifest.version || '');

        // 闸一：兼容
        const minApi = parseInt(manifest.min_api_level || 0, 10);
        let compatible = true;
        if (minApi > 0) {
            compatible = (this.opts.clientApiLevel | 0) >= minApi;
        } else if (manifest.min_client_version) {
            compatible = this._semverGe(this.opts.clientVersion || '0.0.0', manifest.min_client_version);
        }
        if (!compatible) {
            p.needStoreUpgrade = true;
            const su = manifest.store_urls || {};
            p.storeUrl = su[this.opts.platformKey] || su.default || '';
            p.reason = 'need_store_upgrade';
            return p;
        }

        // 解析 entries（按 order 升序）
        const raw = Array.isArray(manifest.entries) ? manifest.entries : [];
        const entries = raw.map((d, i) => ({
            name: String(d.name != null ? d.name : `entry_${i}`),
            kind: String(d.kind || 'pck'),
            url: String(d.url || ''),
            sha256: String(d.sha256 || ''),
            size: parseInt(d.size || 0, 10),
            order: parseInt(d.order != null ? d.order : i, 10),
        }));
        entries.sort((a, b) => a.order - b.order);
        p.allEntries = entries;

        // 闸二：版本未变
        const localVer = this._readLocalVersion();
        if (p.remoteVersion && p.remoteVersion === localVer) {
            p.upToDate = true; p.reason = 'up_to_date'; return p;
        }
        if (entries.length === 0) { p.reason = 'no_entries'; return p; }

        // 逐条 hash diff（本地 hash 走异步 getFileInfo）
        for (const e of entries) {
            if (e.sha256) {
                const local = await this._localHash(e);
                if (local && local === e.sha256) { p.skipped.push(e); continue; }
            }
            p.toDownload.push(e);
        }
        p.reason = 'planned';
        return p;
    }

    // ── 主入口 ────────────────────────────────────────────────────
    async run() {
        const result = { success: false, updated: false, upToDate: false,
                         version: this._readLocalVersion(), reason: '',
                         needStoreUpgrade: false, storeUrl: '' };
        this._emit(0.02, '正在检查版本...');

        let manifest;
        try {
            manifest = await this._fetchJson(this.opts.versionUrl);
        } catch (_e) {
            result.success = true; result.reason = 'no_manifest_fallback';
            this._emit(1.0, '无法获取更新信息，使用内置资源');
            return result;
        }

        const plan = await this._plan(manifest);
        if (!plan.ok) {
            result.success = true; result.reason = 'invalid_manifest_fallback';
            this._emit(1.0, '更新信息无效，使用内置资源');
            return result;
        }
        this._emit(0.08, `远端版本 ${plan.remoteVersion}`);

        if (plan.needStoreUpgrade) {
            result.needStoreUpgrade = true; result.storeUrl = plan.storeUrl;
            result.reason = 'need_store_upgrade';
            this._emit(1.0, '版本过低，请更新小游戏');
            return result;
        }

        if (plan.upToDate) {
            result.success = true; result.upToDate = true;
            result.version = plan.remoteVersion || result.version;
            result.reason = 'up_to_date';
            this._emit(1.0, `已是最新 ${result.version}`);
            return result;
        }

        // 下载 toDownload
        const total = plan.toDownload.length;
        for (let i = 0; i < total; i++) {
            const e = plan.toDownload[i];
            this._emit(0.1 + 0.6 * (i / Math.max(1, total)), `下载 ${e.name} (${i + 1}/${total})`);
            let dst;
            try {
                dst = await this._download(e);
            } catch (_e) {
                result.success = true; result.reason = 'download_failed_fallback';
                this._emit(1.0, '下载失败，使用内置资源');
                return result;
            }
            // 校验（异步 sha256）
            if (e.sha256) {
                const got = await this._fileHash(dst);
                if (got && got !== e.sha256) {
                    result.success = true; result.reason = 'verify_failed_fallback';
                    this._emit(1.0, '校验失败，删除损坏文件，使用内置资源');
                    try { this.fs.unlinkSync(dst); } catch (_e) {}
                    return result;
                }
                if (!got) {
                    // 平台不支持 sha256 摘要时的兜底：仅按 size 粗校验（若 manifest 提供了 size）
                    if (e.size > 0) {
                        const sz = this._fileSize(dst);
                        if (sz > 0 && sz !== e.size) {
                            result.success = true; result.reason = 'size_mismatch_fallback';
                            this._emit(1.0, '大小校验失败，使用内置资源');
                            try { this.fs.unlinkSync(dst); } catch (_e) {}
                            return result;
                        }
                    }
                }
            }
        }

        this._writeLocalVersion(plan.remoteVersion);
        result.success = true; result.updated = true;
        result.version = plan.remoteVersion; result.reason = 'updated';
        this._emit(1.0, `更新完成 ${plan.remoteVersion}`);
        return result;
    }

    // 读取某条目内容（下载后的文件，供 runner.loadFromJson 用）
    readEntryText(entryName) {
        const path = this._entryPath(entryName);
        try { return this.fs.readFileSync(path, 'utf8'); }
        catch (_e) { return ''; }
    }

    // ── 物理层：wx FS + wx.downloadFile ───────────────────────────
    _entryPath(name) { return `${this.patchDir}/${name}`; }

    _download(e) {
        const { g } = _env(this.opts.host);
        const dst = this._entryPath(e.name);
        return new Promise((resolve, reject) => {
            g.downloadFile({
                url: e.url,
                success: (res) => {
                    if (res.statusCode !== 200) { reject(new Error('http ' + res.statusCode)); return; }
                    try {
                        // downloadFile 落临时路径，复制到持久 patchDir
                        this.fs.saveFileSync(res.tempFilePath, dst);
                        resolve(dst);
                    } catch (err) { reject(err); }
                },
                fail: (err) => reject(new Error(err && err.errMsg || 'downloadFile fail')),
            });
        });
    }

    _fetchJson(url) {
        const { g } = _env(this.opts.host);
        return new Promise((resolve, reject) => {
            g.request({
                url, method: 'GET', dataType: 'text', responseType: 'text',
                success: (res) => {
                    if (res.statusCode !== 200) { reject(new Error('http ' + res.statusCode)); return; }
                    try {
                        const txt = typeof res.data === 'string' ? res.data : JSON.stringify(res.data);
                        resolve(JSON.parse(txt));
                    } catch (err) { reject(err); }
                },
                fail: (err) => reject(new Error(err && err.errMsg || 'request fail')),
            });
        });
    }

    // 文件 sha256（异步）。wx/tt 的 getFileInfo 支持 digestAlgorithm:'sha256'。
    // 返回 hex 摘要；平台不支持或出错时返回 ''（run() 会退化到 size 粗校验）。
    _fileHash(path) {
        const { g } = _env(this.opts.host);
        return new Promise((resolve) => {
            try {
                g.getFileInfo({
                    filePath: path,
                    digestAlgorithm: 'sha256',
                    success: (res) => resolve(String(res.digest || '').toLowerCase()),
                    fail: () => resolve(''),
                });
            } catch (_e) {
                resolve('');
            }
        });
    }

    // 本地已下载条目的 sha256（异步）。文件不存在直接返回 ''。
    async _localHash(e) {
        const path = this._entryPath(e.name);
        try { this.fs.accessSync(path); }
        catch (_e) { return ''; }
        return await this._fileHash(path);
    }

    // 同步取文件大小（getFileInfo 的 size 是异步，这里用 statSync 同步版兜底）
    _fileSize(path) {
        try {
            const st = this.fs.statSync(path);
            // wx Stats: st.size；部分实现返回 { size }
            return (st && typeof st.size === 'number') ? st.size : 0;
        } catch (_e) {
            return 0;
        }
    }

    // ── 版本号持久化（wx FS）──────────────────────────────────────
    _readLocalVersion() {
        try { return this.fs.readFileSync(this.localVerFile, 'utf8').trim() || '0.0.0'; }
        catch (_e) { return '0.0.0'; }
    }
    _writeLocalVersion(v) {
        try { this.fs.writeFileSync(this.localVerFile, v, 'utf8'); } catch (_e) {}
    }
    _ensureDir(dir) {
        try { this.fs.accessSync(dir); } catch (_e) {
            try { this.fs.mkdirSync(dir, true); } catch (_e2) {}
        }
    }

    _emit(p, s) { if (this.opts.onProgress) this.opts.onProgress(p, s); }
}
