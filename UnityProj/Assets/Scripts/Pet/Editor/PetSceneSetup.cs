// PetSceneSetup.cs
// 桌宠工程的 Editor 辅助工具：
//   1. 一键生成桌宠 LaunchProfile（Pet.asset）+ 启动场景 PetLauncher.unity
//      —— 场景复用现有 GameLauncher，profile 指向 Pet.asset（数据驱动，无模式分支）
//   2. 以 PetLauncher.unity 为唯一入口独立打包 Windows（"同一工程，独立打包"）
//
// 菜单：Pet/Setup/Create PetLauncher Scene、Pet/Build/Windows x64

using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using Pet.Runtime;
using UGFramework.Runtime;

namespace Pet.Editor
{
    public static class PetSceneSetup
    {
        private const string ScenePath   = "Assets/Scenes/PetLauncher.unity";
        private const string ProfilePath = "Assets/Scenes/Pet.asset";
        private const string PetLuaEntry = "pet.PetLogic";

        // 与 PetWindowService.colorKey 对应的透明色键 #010101
        private static readonly Color ColorKey = new Color(1f / 255f, 1f / 255f, 1f / 255f, 1f);

        [MenuItem("Pet/Setup/Create PetLauncher Scene", priority = 1)]
        public static void CreatePetScene()
        {
            // ── 1. 生成 / 复用 桌宠 LaunchProfile（数据驱动差异）──
            var petProfile = AssetDatabase.LoadAssetAtPath<LaunchProfile>(ProfilePath);
            if (petProfile == null)
            {
                petProfile = ScriptableObject.CreateInstance<LaunchProfile>();
                AssetDatabase.CreateAsset(petProfile, ProfilePath);
            }
            petProfile.packageName  = "DefaultPackage";
            petProfile.skipUpdate   = true;            // 桌宠跳过更新阶段
            petProfile.gameLuaEntry = PetLuaEntry;     // 入口指向桌宠逻辑
            EditorUtility.SetDirty(petProfile);
            AssetDatabase.SaveAssets();

            // ── 2. 生成启动场景 ──
            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);

            // 相机：纯色背景 = 透明色键，正交，用于 2D 桌宠
            var camGo = new GameObject("Main Camera", typeof(Camera), typeof(AudioListener));
            camGo.tag = "MainCamera";
            var cam = camGo.GetComponent<Camera>();
            cam.clearFlags = CameraClearFlags.SolidColor;
            cam.backgroundColor = ColorKey;
            cam.orthographic = true;
            cam.transform.position = new Vector3(0, 0, -10);

            // 启动根：复用 GameLauncher（RequireComponent 自动补齐基础设施）+ 桌宠窗口
            var root = new GameObject("PetRoot");
            var launcher = root.AddComponent<GameLauncher>();
            root.AddComponent<PetWindowService>();

            // 数据驱动：只需把 profile 指过去，无任何模式 if/else
            launcher.profile = petProfile;

            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene, ScenePath);
            AddSceneToBuildSettings(ScenePath);

            Debug.Log($"[PetSceneSetup] Created {ScenePath} + {ProfilePath} " +
                      $"(GameLauncher.profile→Pet, skipUpdate=true, entry={PetLuaEntry}). " +
                      "请在 Pet.asset 上指定 UITK PanelSettings，并配置 YooAssetInitializer 的 CDN。");
        }

        [MenuItem("Pet/Build/Windows x64", priority = 20)]
        public static void BuildWindows()
        {
            if (!File.Exists(ScenePath))
            {
                Debug.LogError("[PetSceneSetup] PetLauncher.unity 不存在，请先执行 Pet/Setup/Create PetLauncher Scene");
                return;
            }

            // 桌宠窗口需要后台运行（失焦不暂停）
            PlayerSettings.runInBackground = true;

            string outDir = Path.GetFullPath(Path.Combine(Application.dataPath, "../Build/Pet"));
            Directory.CreateDirectory(outDir);

            var options = new BuildPlayerOptions
            {
                scenes = new[] { ScenePath },              // 独立打包：只含桌宠场景
                locationPathName = Path.Combine(outDir, "DesktopPet.exe"),
                target = BuildTarget.StandaloneWindows64,
                options = BuildOptions.None,
            };

            var report = BuildPipeline.BuildPlayer(options);
            Debug.Log($"[PetSceneSetup] Build {report.summary.result} -> {outDir}");
        }

        private static void AddSceneToBuildSettings(string path)
        {
            var scenes = new System.Collections.Generic.List<EditorBuildSettingsScene>(EditorBuildSettings.scenes);
            if (!scenes.Exists(s => s.path == path))
            {
                scenes.Add(new EditorBuildSettingsScene(path, true));
                EditorBuildSettings.scenes = scenes.ToArray();
            }
        }
    }
}
