# 热更新方案（Godot 版）

> 本文只给**方案**，不含最终代码实现。目标：把 Unity `UpdateLogic.lua` 那套
> 「版本检查 → 下载 → 挂载 → 校验 → 弱网回退」的热更能力，用 Godot 原生机制落地。
> 双 VM 的**框架机制**（`begin/end_update_phase`）已在 `docs/03` 完成并验证；本文补齐
> 中间缺的「资源下载 + `.pck` 挂载 + 版本管理」那几步。

---

## 1. 目标与边界

| 要热更的东西 | 能否热更 | 机制 |
|---|---|---|
| **游戏资源**（贴图/场景/音频/蓝图 .bjson/Lua 脚本） | ✅ | Godot `.pck` 补丁包 + `ProjectSettings.load_resource_pack` |
| **Lua 玩法逻辑** | ✅ | 打进 `.pck` 的 `res://lua/**.lua`，双 VM 切换后重新 `require` |
| **GDScript / 场景绑定的脚本** | ✅（`.gd` 打进 pck） | 同上，`.pck` 覆盖 res:// |
| **GDExtension（C++ 引擎 dll/so）** | ❌ | 原生二进制不能热更；桌面可整包替换，移动/商店受限，Web/小程序不可 |
| **引擎本体升级** | ❌ | 走整包发版 |

> 铁律：**能进 `.pck` 的都能热更；原生二进制（引擎 dll、GDExtension）不能热更**。
> 这与 Unity 时代「C# 层不能热更、Lua+资源能热更」是同一条边界，只是 Godot 里
> 「不可热更的原生层」= 引擎 dll + GDExtension，「可热更层」= res:// 下一切资源与脚本。

---

## 2. 为什么用 `.pck` 而不是移植 YooAsset

Unity 用 YooAsset 管 AB 包、版本、下载。**Godot 不需要移植它**——Godot 原生就有对等能力：

| YooAsset 概念 | Godot 原生对应 |
|---|---|
| AB 包 / 资源包 | `.pck`（或 `.zip`）资源包 |
| `LoadAssetAsync` | `ResourceLoader.load` / `load_threaded_request`（已实现于 `SceneService`）|
| 补丁包覆盖 | `ProjectSettings.load_resource_pack(path, replace=true)` |
| Manifest / 版本清单 | 自定义一个 `version.json`（见 §4）|
| CDN 下载 | `HTTPRequest` / 引擎 `http.*` 节点 |

挂载 `.pck` 后，`FileAccess` / `ResourceLoader` 会**自动**从新包读取覆盖的文件——
这正是我们的**文件桥 + Lua require resolver 零改动**的原因：它们本就走 `FileAccess`，
补丁挂上后透明生效。

---

## 3. 整体流程（六步，对齐上方流程图）

```
setup()  →  begin_update_phase(VM#1)  →  ①版本  →  ②diff  →  ③下载pck  →  ④校验+挂载  →  end_update_phase(→VM#2)  →  主逻辑
                                            └──────── 任一步网络失败 → 回退内置资源，照常进主VM ────────┘
```

已完成（`docs/03` 验证过）：`setup` / `begin_update_phase` / `end_update_phase` / VM 切换。
**本方案要新增的只有 ①②③④** —— 即「更新阶段内部具体干什么」。

### ① 拉远端版本号
- 从 CDN 拉 `version.json`（含最新版本号 + 各 pck 的 url/hash/size）。
- 下载手段二选一：
  - **A. Godot `HTTPRequest`**（GDScript 更新逻辑用）——最简单，Godot 原生。
  - **B. 引擎 `http.*` Lua 节点**（Lua 更新逻辑用）——已有 `BP_InitDefaultHttpClient`。
- **5s 超时**，超时/无网 → 回退（见 §6）。

### ② 版本 diff
- 比对远端版本号与本地已装版本号（本地版本号存 `user://ver.txt` 或 `ProjectSettings` meta）。
- 相同 → 跳过下载，直接 `end_update_phase` 进主逻辑。
- 不同 → 进入下载。

### ③ 下载补丁 `.pck`
- 按 `version.json` 列出的待下载 pck 逐个下载到 **`user://patches/`**（`user://` 是唯一可写目录，桌面/移动/Web 都有）。
- 进度回调映射到更新 UI 的 30%→100%（沿用 Unity 的进度区间约定）。
- 断点续传/并发可后续加，首版顺序下载即可。

### ④ 校验 + 挂载
- **校验**：对下载的 pck 算 hash（`FileAccess.get_sha256` 或引擎提供的），比对 `version.json` 里的期望值。校验失败 → 丢弃该包 → 回退。
- **挂载**：`ProjectSettings.load_resource_pack("user://patches/xxx.pck", true)`（`replace=true` 让补丁覆盖同名 res:// 文件）。
- 挂载成功后**写入本地版本号**（下次启动 ② 才能正确 diff）。
- 挂载**必须在 `end_update_phase()` 之前**完成：这样 VM#2 首次 `require`/加载蓝图时，读到的已经是补丁里的新文件。

### ⑤⑥ 双 VM 切换 + 主逻辑
- 已实现：`end_update_phase()` 销毁 VM#1（`lua_close`，旧模块缓存全清）→ 建全新 VM#2。
- VM#2 加载主蓝图/主 Lua，`require` 命中的是**挂载后的新代码**（旧缓存已随 VM#1 销毁）——
  这就是双 VM 的意义：避免更新阶段 `require` 的旧模块污染主逻辑。

---

## 4. version.json 约定（建议格式）

放 CDN 根，客户端每次启动先拉它：

```json
{
  "version": "1.0.3",
  "min_engine": "4.5",
  "packs": [
    { "name": "game.pck",  "url": "https://cdn/xxx/1.0.3/game.pck",  "sha256": "…", "size": 10485760 },
    { "name": "lua.pck",   "url": "https://cdn/xxx/1.0.3/lua.pck",   "sha256": "…", "size": 524288 }
  ]
}
```

- `min_engine`：若当前引擎版本低于它，提示用户去整包升级（原生层不能热更）。
- 可拆多个 pck（资源包 / Lua 包分离），按需下载。

---

## 5. 打补丁 pck 的方法（发版侧）

Godot 导出补丁 pck 有两条路：

- **命令行导出**：`godot --headless --export-pack "<preset>" out.pck`（导出全量或指定目录）。
- **增量脚本**：用 `PCKPacker` 类只把改动的 `res://` 文件打进一个小 pck。
  ```gdscript
  var p := PCKPacker.new()
  p.pck_start("game_1.0.3.pck")
  p.add_file("res://lua/demo/main_menu_logic.lua", "本地/新版/main_menu_logic.lua")
  p.flush()
  ```
- 补丁 pck 里的路径必须是 `res://...`，挂载时 `replace=true` 才能覆盖旧文件。

---

## 6. 弱网 / 失败回退（沿用 Unity 容错哲学）

**任何**网络步骤失败都不阻断启动——回退用内置资源照常进主逻辑：

| 失败点 | 处理 |
|---|---|
| 拉 version.json 超时/失败 | 用内置版本，跳过更新，直接进主逻辑 |
| 下载 pck 失败 | 丢弃、用内置资源进主逻辑（下次启动再试）|
| pck 校验失败 | 丢弃该包、不挂载、用内置资源 |
| 挂载失败 | 记日志、用内置资源 |

内置资源 = 首包里的 res://（导出时打进主 pck 的那份）。所以「更新失败」永远等价于「玩旧版」，不会白屏/卡死。

---

## 7. 更新逻辑用 GDScript 还是 Lua（两套都支持）

| 方案 | 更新逻辑写在 | VM | 何时选 |
|---|---|---|---|
| **A（推荐先做）** | GDScript（`HTTPRequest` + `load_resource_pack`）| 单 VM，延迟建主 runner | 更新逻辑不需要复用 Lua 玩法代码时，最简单直接 |
| **B（对齐 Unity）** | Lua（`http.*` 节点 + 引擎回调挂 pck）| 双 VM，`begin/end_update_phase` | 更新阶段 UI/逻辑也想用 Lua 写、复用 Lua 生态时 |

> 关键提醒：即使走方案 B，**挂载 `.pck` 这一步的能力**（`ProjectSettings.load_resource_pack`）
> 仍在 GDScript/GDExtension 侧——Lua 通过一个新的 `Update.mount_pck(path)` 桥接函数调它
> （和现有 `Scene.*`/`UI.*` 同样的项目层绑定套路，引擎零改动）。这是本方案唯一需要新增的桥接点。

---

## 8. 落地工作量清单（供拍板）

新增（都在**项目层 GodotIntegration**，引擎零改动）：

1. **`UpdateService.gd`（Autoload）**：`check_version()` / `download_pck()` / `verify()` / `mount_pck()` / 本地版本号读写。走 `HTTPRequest` + `load_resource_pack`。
2. **更新 UI 进度**：复用现有 `UpdatePanel`（已有 set_progress/set_status）。
3. **Boot 流程串联**：`setup → begin_update_phase → UpdateService 跑①~④ → end_update_phase → 主场景`。
4. **（仅方案 B 需要）Lua 桥**：`Update.mount_pck(path)` / `Update.check_version()` 等，注册进 Lua VM（同 `godot_lua_bindings` 套路）。
5. **发版侧脚本**：`PCKPacker` 打增量补丁 + 生成 `version.json`（含 sha256）。

**不需要**：移植 YooAsset、改引擎、改文件桥/require（挂 pck 后自动生效）。

---

## 9. 与已完成部分的关系

- `docs/03` = 双 VM **框架机制**（怎么切 VM）——已完成验证。
- `docs/06`（本文）= 热更**业务流程**（切 VM 之间具体下载/挂载什么）。
- 二者组合 = 完整热更。本方案的新增全部落在「更新阶段内部」，不动已跑通的框架。

---

## 10. 实现与验证（方案 A，已跑通）

方案 A 已落地并 headless 验证通过。涉及文件：

| 文件 | 角色 |
|---|---|
| `game/framework/UpdateService.gd` | Autoload 热更服务：`run_update()` 跑①~④，`progress` 信号驱动 UI |
| `game/tools/make_patch.gd` | 发版侧：`PCKPacker` 打补丁 pck + sha256 + 生成 `version.json` |
| `game/patch_src/**` | 补丁包的「新版」源文件（本 Demo 是改标题的 main_menu_logic.lua）|
| `game/fake_cdn/` | **本地假 CDN**：`version.json` + `1.0.1/game.pck`（打进主包，无需真服务器）|
| `game/framework/Boot.gd` | 串联：setup → begin_update_phase → `UpdateService.run_update()` → end_update_phase → 主场景 |

### 假 CDN 的巧思
`UpdateService.cdn_base_url` 默认 `res://fake_cdn`。传输层按 scheme 分流：
- `res://` / `user://` → `FileAccess`（本地假 CDN，Demo 用）
- `http(s)://` → `HTTPRequest`（真 CDN，改一行 `cdn_base_url` 即可切换）

所以从假 CDN 到真 CDN **只改一个 URL**，下载/校验/挂载逻辑完全复用。

### 生成补丁
```bash
godot --headless --path <game_dir> --script res://tools/make_patch.gd
# 产出 res://fake_cdn/version.json + res://fake_cdn/1.0.1/game.pck
```

### 验证结果（headless 实跑）
```
# 首次（本地 0.0.0，远端 1.0.1）
[Boot] update result: success=true updated=true version=1.0.1 reason=updated
[GameScene] Lua 设置的标题 = '主菜单 (热更 v1.0.1)'    ← 补丁里的新字符串，证明热更生效

# 第二次（本地已 1.0.1）
reason=up_to_date + 重新挂载本地补丁 → 标题仍是 '主菜单 (热更 v1.0.1)'

# 弱网（CDN 指向不存在目录）
success=true reason=no_manifest_fallback  → 不阻断，用内置资源进主逻辑
```

### 关键实现要点（踩坑记录）
- **`load_resource_pack` 不持久**：每次进程启动都要重挂。所以 `up_to_date`（版本没变）时也要 `_remount_local_packs()` 重挂本地已下载的补丁，否则读回内置旧版。
- **挂载必须在 `end_update_phase()` 之前**：这样新主 VM 首次 `require`/加载读到的是补丁新文件。
- **补丁 pck 内路径必须是 `res://...`** + `load_resource_pack(path, true)` 的 `replace=true` 才能覆盖同名内置文件。
- **GDScript 静态类型**：`Dictionary.get()` 返回 Variant，`var x := dict.get(...)` 会因「从 Variant 推断」被当 warning-as-error 拒编，须显式 `var x: String = str(...)`。
