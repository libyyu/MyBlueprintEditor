// GameLauncher.cs
// 游戏启动总入口（挂到 Bootstrap 场景的第一个 GameObject）
//
// 启动流程：
//
//   [原生平台 iOS / Android / PC]
//   Step 1  YooAsset Phase 1：初始化本地文件系统（内置包 or 缓存，不访问网络）
//   Step 2  从本地加载正式 LoadingUI prefab 并显示（内置版 or 上次缓存的最新版）
//   Step 3  YooAsset Phase 2：请求版本号 → 更新 Manifest → 下载缺失 bundle（进度显示）
//   Step 4  xLua 启动，执行 main.lua
//   Step 5  跳转主场景
//
//   [WebGL / 微信小游戏]
//   Step 1  极简 BootstrapLoadingUI 立即显示（纯代码生成，零资源依赖，秒显示）
//   Step 2  YooAsset Phase 1：初始化远端文件系统
//   Step 3  从 CDN 加载正式 LoadingUI bundle，替换极简 UI（WaitForEndOfFrame 避免闪烁）
//   Step 4  YooAsset Phase 2：下载缺失 bundle（进度显示）
//   Step 5  xLua 启动，执行 main.lua
//   Step 6  跳转主场景
//
//   再次启动（所有平台）：
//     Phase 1 读缓存/内置，Phase 2 只下本次差量
//
// 挂载要求：
//   - 同一 GameObject 上需有 YooAssetInitializer、LuaManager
//   - Execution Order 设为 -100（早于一切游戏逻辑）

using Cysharp.Threading.Tasks;
using System.Collections;
using UnityEngine;
using UnityEngine.SceneManagement;
using XLua;
using YooAsset;

namespace CutRope.Framework
{
    [DefaultExecutionOrder(-100)]
    [RequireComponent(typeof(FileLoggger))]
    //[RequireComponent(typeof(YooAssetInitializer))]
    //[RequireComponent(typeof(LuaManager))]
    //[RequireComponent(typeof(SceneLoader))]
    //[RequireComponent(typeof(UIManager))]
    public class GameLauncherNew : MonoBehaviour
    {
        // ── 私有引用 ─────────────────────────────────────────────────
        private YooAssetInitializer _yooInit;

        // ── 生命周期 ─────────────────────────────────────────────────
        private void Awake()
        {
            Debug.Log("[GameLauncher] Awake...");
            _yooInit = GetComponent<YooAssetInitializer>();
        }

        private async void Start()
        {
            Debug.Log("[GameLauncher] Starting...");

            // Phase 1 初始化更新阶段的资源系统（真机内置文件）
            bool success = await _yooInit.LaunchInitUpdateStage();
            Debug.Log($"[GameLauncher] Starting... {success}");
            if (!success) return;



            // Phase 2启动更新阶段lua虚拟机

            // Phase 2加载UpdateLogic.lua执行更新阶段主逻辑

            // Phase 2更新阶段完成，关闭Lua虚拟机

            // Phase 3启动游戏逻辑lua虚拟器

            // Phase 3加载GameLogic.lua执行游戏主逻辑
        }


    }
}
