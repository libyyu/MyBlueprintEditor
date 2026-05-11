// BuildScript.cs
// CI 自定义构建入口，供 game-ci/unity-builder 通过 -buildMethod 调用
//
// 用法（game-ci workflow）：
//   buildMethod: BuildScript.BuildWindows

using System;
using System.Linq;
using UnityEditor;
using UnityEditor.Build.Reporting;
using UnityEngine;

namespace CutRope.Editor
{
    public static class BuildScript
    {
        // 需要打包的场景（按顺序）
        private static readonly string[] Scenes =
        {
            "Assets/GameLauncher.unity",
        };

        [MenuItem("Tools/CutRope/Build Windows (CI)", priority = 50)]
        public static void BuildWindows()
        {
            Build(BuildTarget.StandaloneWindows64, "build/StandaloneWindows64/CutRope.exe");
        }

        public static void BuildAndroid()
        {
            Build(BuildTarget.Android, "build/Android/CutRope.apk");
        }

        public static void BuildWebGL()
        {
            Build(BuildTarget.WebGL, "build/WebGL");
        }

        // ── 内部 ──────────────────────────────────────────────────────

        static void Build(BuildTarget target, string outputPath)
        {
            // CI 环境：先静默执行 Setup 确保资源完整
            Debug.Log("[BuildScript] Running silent setup for CI...");
            try
            {
                CutRopeSetup.SetupTags();
                CutRopeSetup.CreatePrefabs();
                AssetDatabase.Refresh();
                CutRopeSetup.CreateScenes();
                CutRopeSetup.AddScenesToBuildSettings();
                AssetDatabase.SaveAssets();
                AssetDatabase.Refresh();
                Debug.Log("[BuildScript] Silent setup complete.");
            }
            catch (System.Exception e)
            {
                Debug.LogWarning($"[BuildScript] Setup warning (non-fatal): {e.Message}");
            }

            // 验证场景文件存在
            var missing = Scenes.Where(s => !System.IO.File.Exists(s)).ToArray();
            if (missing.Length > 0)
            {
                string msg = $"[BuildScript] Missing scenes:\n{string.Join("\n", missing)}\n" +
                             "Run Tools > CutRope > Setup All first.";
                Debug.LogError(msg);
                EditorApplication.Exit(1);
                return;
            }

            var options = new BuildPlayerOptions
            {
                scenes           = Scenes,
                locationPathName = outputPath,
                target           = target,
                options          = BuildOptions.None,
            };

            var report = BuildPipeline.BuildPlayer(options);
            var summary = report.summary;

            if (summary.result == BuildResult.Succeeded)
            {
                Debug.Log($"[BuildScript] Build succeeded: {outputPath}  ({summary.totalSize / 1024 / 1024} MB)");
                EditorApplication.Exit(0);
            }
            else
            {
                Debug.LogError($"[BuildScript] Build FAILED: {summary.result}  errors={summary.totalErrors}");
                EditorApplication.Exit(1);
            }
        }
    }
}
