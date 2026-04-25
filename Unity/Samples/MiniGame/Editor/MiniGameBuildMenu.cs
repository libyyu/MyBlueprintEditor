// MiniGameBuildMenu.cs
// ─────────────────────────────────────────────────────────────────────────────
// Unity Editor 菜单 — "Blueprint/构建 WebGL → 微信小游戏"
//
// 一键设置 PlayerSettings 到微信小游戏推荐配置 + Build。
// 构建完成后，用微信 Unity WebGL 转换工具把 WebGL 输出转成 wxa：
//   https://github.com/wechat-miniprogram/minigame-unity-webgl-transform
//
// 本脚本只能在 UnityEditor 里运行，不进运行时。
// ─────────────────────────────────────────────────────────────────────────────

#if UNITY_EDITOR
using System;
using System.IO;
using UnityEditor;
using UnityEditor.Build.Reporting;
using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame.EditorTools
{
    public static class MiniGameBuildMenu
    {
        private const string MenuRoot = "Blueprint/";

        // ─────────────────────────────────────────────────────────────
        [MenuItem(MenuRoot + "应用微信小游戏推荐 PlayerSettings")]
        public static void ApplyWeChatPlayerSettings()
        {
            // WebGL 压缩：Brotli（微信支持；体积最小）
            PlayerSettings.WebGL.compressionFormat = WebGLCompressionFormat.Brotli;

            // 代码优化：Size（比 Speed 小 20~30%，运行速度对 AI 对话类影响不大）
// #if UNITY_2021_2_OR_NEWER
//             PlayerSettings.WebGL.codeOptimization = WebGLCodeOptimization.Size;
// #endif

            // 异常处理：Explicitly thrown exceptions only（小 + 足够）
            PlayerSettings.WebGL.exceptionSupport = WebGLExceptionSupport.ExplicitlyThrownExceptionsOnly;

            // 内存初始 32MB（Runtime + 基础游戏够用；需要时自动增长）
#if UNITY_2020_2_OR_NEWER
            PlayerSettings.WebGL.memorySize = 32;
#endif

            // 单独资源 Bundle：OFF（减少首次下载文件数）
            PlayerSettings.WebGL.dataCaching = true;

            // 模版：默认（微信转换工具会替换）
            PlayerSettings.WebGL.template = "APPLICATION:Default";

            // IL2CPP + .NET Standard 2.1
            PlayerSettings.SetScriptingBackend(BuildTargetGroup.WebGL, ScriptingImplementation.IL2CPP);
#if UNITY_2021_2_OR_NEWER
            PlayerSettings.SetApiCompatibilityLevel(BuildTargetGroup.WebGL, ApiCompatibilityLevel.NET_Standard_2_0);
#endif

            EditorUtility.DisplayDialog("Blueprint",
                "PlayerSettings 已应用微信小游戏推荐配置：\n" +
                " - 压缩：Brotli\n" +
                " - 代码优化：Size\n" +
                " - 异常：Only Explicit\n" +
                " - 初始内存：32MB\n" +
                " - IL2CPP + .NET Standard 2.1\n\n" +
                "下一步：Blueprint → 构建 WebGL",
                "OK");
        }

        // ─────────────────────────────────────────────────────────────
        [MenuItem(MenuRoot + "构建 WebGL → 微信小游戏")]
        public static void BuildWeChatWebGL()
        {
            // 选择输出目录
            string outDir = EditorUtility.SaveFolderPanel("选择 WebGL 输出目录",
                Path.GetDirectoryName(Application.dataPath), "Build_WebGL");
            if (string.IsNullOrEmpty(outDir))
                return;

            // 场景列表：用当前 BuildSettings 里勾选的场景
            var scenes = EditorBuildSettings.scenes;
            if (scenes.Length == 0)
            {
                EditorUtility.DisplayDialog("Blueprint", "Build Settings 里没有场景，请先添加场景", "OK");
                return;
            }

            // 切到 WebGL 目标
            if (EditorUserBuildSettings.activeBuildTarget != BuildTarget.WebGL)
            {
                if (!EditorUtility.DisplayDialog("Blueprint",
                    $"当前构建目标是 {EditorUserBuildSettings.activeBuildTarget}，需要切到 WebGL。\n切换会重新导入所有资源（可能几分钟）。继续？",
                    "继续", "取消"))
                    return;
                EditorUserBuildSettings.SwitchActiveBuildTarget(BuildTargetGroup.WebGL, BuildTarget.WebGL);
            }

            var opts = new BuildPlayerOptions
            {
                scenes           = Array.ConvertAll(scenes, s => s.path),
                locationPathName = outDir,
                target           = BuildTarget.WebGL,
                options          = BuildOptions.None,
            };

            Debug.Log($"[Blueprint] 开始构建 WebGL → {outDir}");
            BuildReport report = BuildPipeline.BuildPlayer(opts);

            if (report.summary.result == BuildResult.Succeeded)
            {
                long sizeKB = (long)(report.summary.totalSize / 1024);
                EditorUtility.DisplayDialog("Blueprint",
                    $"✓ 构建成功\n" +
                    $"耗时：{report.summary.totalTime}\n" +
                    $"总大小：{sizeKB / 1024.0:F1} MB\n\n" +
                    $"输出：{outDir}\n\n" +
                    "下一步：用微信 Unity WebGL 转换工具把本目录转成 wxa：\n" +
                    "https://github.com/wechat-miniprogram/minigame-unity-webgl-transform",
                    "打开输出目录");
                EditorUtility.RevealInFinder(outDir);
            }
            else
            {
                EditorUtility.DisplayDialog("Blueprint",
                    $"✗ 构建失败：{report.summary.result}\n详见 Console",
                    "OK");
            }
        }

        // ─────────────────────────────────────────────────────────────
        [MenuItem(MenuRoot + "打开微信 Unity 转换工具 GitHub")]
        public static void OpenWxTransformDocs()
        {
            Application.OpenURL("https://github.com/wechat-miniprogram/minigame-unity-webgl-transform");
        }
    }
}
#endif
