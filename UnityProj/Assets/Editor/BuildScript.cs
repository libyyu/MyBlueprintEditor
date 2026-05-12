// BuildScript.cs
// CI 自定义构建入口，供 game-ci/unity-builder 通过 -buildMethod 调用
//
// 构建顺序：
//   1. Setup Tags / Prefabs / Scenes（幂等，已存在则跳过）
//   2. YooAsset 打包 DefaultPackage（BuiltinBuildPipeline）
//   3. YooAsset ClearAndCopyAll 自动拷贝内置包到 StreamingAssets
//   4. BuildPlayer

using System;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEngine;
using YooAsset;
using YooAsset.Editor;

namespace CutRope.Editor
{
    public static class BuildScript
    {
        // 打包的场景
        private static readonly string[] Scenes =
        {
            "Assets/GameLauncher.unity",
        };

        // YooAsset 包名
        private const string PackageName    = "DefaultPackage";
        // 包版本（CI 可通过环境变量覆盖）
        private static string PackageVersion =>
            Environment.GetEnvironmentVariable("PACKAGE_VERSION") ?? "1.0.0";

        // ── 菜单入口 ──────────────────────────────────────────────────

        [MenuItem("Tools/CutRope/Build Windows (CI)", priority = 50)]
        public static void BuildWindows()
        {
            Build(BuildTarget.StandaloneWindows64, "build/StandaloneWindows64/CutRope.exe");
        }

        [MenuItem("Tools/CutRope/Build Android (CI)", priority = 51)]
        public static void BuildAndroid()
        {
            Build(BuildTarget.Android, "build/Android/CutRope.apk");
        }

        [MenuItem("Tools/CutRope/Build WebGL (CI)", priority = 52)]
        public static void BuildWebGL()
        {
            Build(BuildTarget.WebGL, "build/WebGL");
        }

        // ── 核心流程 ──────────────────────────────────────────────────

        static void Build(BuildTarget target, string outputPath)
        {
            try
            {
                // ① 场景 / Prefab / Tags 幂等初始化
                RunSetup();

                // ② YooAsset 资源打包
                if (!BuildYooAsset(target))
                {
                    Debug.LogError("[BuildScript] YooAsset build failed, aborting.");
                    EditorApplication.Exit(1);
                    return;
                }

                // ③ 验证场景
                var missing = Scenes.Where(s => !File.Exists(s)).ToArray();
                if (missing.Length > 0)
                {
                    Debug.LogError($"[BuildScript] Missing scenes:\n{string.Join("\n", missing)}");
                    EditorApplication.Exit(1);
                    return;
                }

                // ④ BuildPlayer
                var opt = new BuildPlayerOptions
                {
                    scenes           = Scenes,
                    locationPathName = outputPath,
                    target           = target,
                    options          = BuildOptions.None,
                };

                var report  = BuildPipeline.BuildPlayer(opt);
                var summary = report.summary;

                if (summary.result == UnityEditor.Build.Reporting.BuildResult.Succeeded)
                {
                    Debug.Log($"[BuildScript] ✓ Build succeeded → {outputPath}  ({summary.totalSize / 1024 / 1024} MB)");
                    EditorApplication.Exit(0);
                }
                else
                {
                    Debug.LogError($"[BuildScript] ✗ Build FAILED: {summary.result}  errors={summary.totalErrors}");
                    EditorApplication.Exit(1);
                }
            }
            catch (Exception e)
            {
                Debug.LogError($"[BuildScript] Exception: {e}");
                EditorApplication.Exit(1);
            }
        }

        // ── Step 1：场景 / Prefab 幂等初始化 ─────────────────────────

        static void RunSetup()
        {
            Debug.Log("[BuildScript] Step 1: Setup Tags / Prefabs / Scenes...");
            try
            {
                CutRopeSetup.SetupTags();
                CutRopeSetup.CreatePrefabs();
                AssetDatabase.Refresh();
                CutRopeSetup.CreateScenes();
                CutRopeSetup.AddScenesToBuildSettings();
                AssetDatabase.SaveAssets();
                AssetDatabase.Refresh();
                Debug.Log("[BuildScript] Step 1: Done.");
            }
            catch (Exception e)
            {
                // 已存在时 CreateScenes 会 return，log 为非致命警告
                Debug.LogWarning($"[BuildScript] Step 1 warning (non-fatal): {e.Message}");
            }
        }

        // ── Step 2：YooAsset 资源打包 ─────────────────────────────────

        static bool BuildYooAsset(BuildTarget target)
        {
            Debug.Log($"[BuildScript] Step 2: YooAsset build — package={PackageName} ver={PackageVersion} target={target}");

            // 输出目录：项目根/YooAssetBundles
            string buildOutputRoot  = Path.Combine(Application.dataPath, "../Bundles");
            // 内置包目录：StreamingAssets（Unity 会自动打进包体）
            string buildinFileRoot  = Application.streamingAssetsPath;

            var buildParameters = new BuiltinBuildParameters
            {
                BuildOutputRoot      = buildOutputRoot,
                BuildinFileRoot      = buildinFileRoot,
                BuildPipeline        = EBuildPipeline.BuiltinBuildPipeline.ToString(),
                BuildBundleType      = (int)EBuildBundleType.AssetBundle,
                BuildTarget          = target,
                PackageName          = PackageName,
                PackageVersion       = PackageVersion,
                CompressOption       = ECompressOption.LZ4,
                VerifyBuildingResult = true,
                EnableSharePackRule  = true,
                ClearBuildCacheFiles = false,   // 增量打包
                BuildinFileCopyOption = EBuildinFileCopyOption.ClearAndCopyAll,
            };

            var pipeline = new BuiltinBuildPipeline();
            var result   = pipeline.Run(buildParameters, enableLog: true);

            if (result.Success)
            {
                Debug.Log($"[BuildScript] Step 2: YooAsset build succeeded → {result.OutputPackageDirectory}");
                // ClearAndCopyAll 已自动拷贝内置包到 StreamingAssets，无需手动复制
                AssetDatabase.Refresh();
                return true;
            }
            else
            {
                Debug.LogError($"[BuildScript] Step 2: YooAsset build FAILED: {result.ErrorInfo}");
                return false;
            }
        }

    }
}
