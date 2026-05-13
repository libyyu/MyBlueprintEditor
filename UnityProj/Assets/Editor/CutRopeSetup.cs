// CutRopeSetup.cs
// 一键搭建 CutRope Demo 工程结构
// 菜单：Tools > CutRope > Setup All
//
// 执行内容：
//   1. 添加 Tags（Monster / Spike / Trap）
//   2. 创建 Prefabs（RopeSegment / Candy / LoadingUI / MainMenu Canvas）
//   3. 创建场景（GameLauncher / MainMenu / Level_1_1）并加入 BuildSettings
//   4. 配置 YooAsset DefaultPackage Collector（Assets/Lua/ + Assets/UI/）

using System.IO;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine.SceneManagement;

namespace CutRope.Editor
{
    public static class CutRopeSetup
    {
        // ── 菜单入口 ──────────────────────────────────────────────────

        [MenuItem("Tools/CutRope/▶ Setup All (一键搭建)", priority = 0)]
        public static void SetupAll()
        {
            bool ok = EditorUtility.DisplayDialog(
                "CutRope 一键搭建",
                "将执行以下操作：\n" +
                "• 添加 Tags（Monster / Spike / Trap）\n" +
                "• 生成 Prefabs（RopeSegment / Candy / LoadingUI / MainMenuCanvas）\n" +
                "• 创建场景（GameLauncher / MainMenu / Level_1_1）\n" +
                "• 将三个场景加入 Build Settings\n\n" +
                "继续？",
                "搭建", "取消");

            if (!ok) return;

            SetupTags();
            CreatePrefabs();           // 创建 Prefab + 强制导入
            AssetDatabase.Refresh();   // 确保場景创建时 LoadAssetAtPath 能取到
            CreateScenes();
            AddScenesToBuildSettings();

            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh();

            EditorUtility.DisplayDialog("完成",
                "CutRope Demo 工程结构搭建完毕！\n\n" +
                "下一步：\n" +
                "1. Window > YooAsset > AssetBundleCollector\n" +
                "   → DefaultPackage 添加 Collector：\n" +
                "     Assets/UI/ (收集 UI Prefab)\n" +
                "     Assets/Lua/ (收集 Lua 文本)\n" +
                "2. 打开 GameLauncher 场景运行游戏",
                "OK");
        }

        [MenuItem("Tools/CutRope/Add Tags", priority = 10)]
        public static void SetupTags()
        {
            string[] tags = { "Monster", "Spike", "Trap" };
            foreach (var tag in tags)
                AddTag(tag);
            Debug.Log("[CutRopeSetup] Tags added: " + string.Join(", ", tags));
        }

        [MenuItem("Tools/CutRope/Create Prefabs", priority = 11)]
        public static void CreatePrefabs()
        {
            CreateRopeSegmentPrefab();
            CreateCandyPrefab();
            CreateLoadingUIPrefab();
            CreateMainMenuCanvasPrefab();
            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh();
            // 强制同步导入，确保后续 LoadAssetAtPath 能立即获取
            AssetDatabase.ImportAsset("Assets/DefaultPackage/Prefabs/Rope/RopeSegment.prefab", ImportAssetOptions.ForceUpdate);
            AssetDatabase.ImportAsset("Assets/DefaultPackage/Prefabs/Game/Candy.prefab",       ImportAssetOptions.ForceUpdate);
            Debug.Log("[CutRopeSetup] Prefabs created.");
        }

        [MenuItem("Tools/CutRope/Create Scenes", priority = 12)]
        public static void CreateScenes()
        {
            CreateGameLauncherScene();
            CreateMainMenuScene();
            CreateLevel1_1Scene();
            Debug.Log("[CutRopeSetup] Scenes created.");
        }

        // ── Tags ──────────────────────────────────────────────────────

        static void AddTag(string tag)
        {
            SerializedObject tagManager = new SerializedObject(
                AssetDatabase.LoadAllAssetsAtPath("ProjectSettings/TagManager.asset")[0]);
            SerializedProperty tagsProp = tagManager.FindProperty("tags");

            for (int i = 0; i < tagsProp.arraySize; i++)
                if (tagsProp.GetArrayElementAtIndex(i).stringValue == tag) return;

            tagsProp.InsertArrayElementAtIndex(tagsProp.arraySize);
            tagsProp.GetArrayElementAtIndex(tagsProp.arraySize - 1).stringValue = tag;
            tagManager.ApplyModifiedProperties();
        }

        // ── Prefabs ───────────────────────────────────────────────────

        static void EnsureDir(string path)
        {
            // 统一换正斜杠（Windows 下 Path.GetDirectoryName 返回反斜杠）
            path = path.Replace("\\", "/");
            if (AssetDatabase.IsValidFolder(path)) return;

            var parts = path.Split('/');
            string cur = parts[0];
            for (int i = 1; i < parts.Length; i++)
            {
                string next = cur + "/" + parts[i];
                if (!AssetDatabase.IsValidFolder(next))
                    AssetDatabase.CreateFolder(cur, parts[i]);
                cur = next;
            }
            // 确保目录已被 AssetDatabase 识别
            AssetDatabase.Refresh();
        }

        static void SavePrefab(GameObject go, string assetPath)
        {
            assetPath = assetPath.Replace("\\", "/");
            EnsureDir(Path.GetDirectoryName(assetPath).Replace("\\", "/"));
            PrefabUtility.SaveAsPrefabAsset(go, assetPath);
            Object.DestroyImmediate(go);
        }

        static void CreateRopeSegmentPrefab()
        {
            const string path = "Assets/DefaultPackage/Prefabs/Rope/RopeSegment.prefab";
            if (AssetDatabase.LoadAssetAtPath<GameObject>(path) != null) return;

            var go = new GameObject("RopeSegment");

            // 视觉：小圆
            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = AssetDatabase.GetBuiltinExtraResource<Sprite>("UI/Skin/Knob.psd");
            sr.color  = new Color(0.6f, 0.35f, 0.15f);
            go.transform.localScale = Vector3.one * 0.25f;

            // 物理
            var rb = go.AddComponent<Rigidbody2D>();
            rb.mass        = 0.1f;
            rb.drag        = 0.5f;
            rb.gravityScale = 1f;

            var col = go.AddComponent<CircleCollider2D>();
            col.radius = 0.5f;

            // 脚本（通过 Type 查找，避免硬依赖程序集名）
            var segType = GetTypeByName("CutRope.Game.RopeSegment");
            if (segType != null) go.AddComponent(segType);
            else Debug.LogWarning("[CutRopeSetup] RopeSegment script not found — add manually.");

            SavePrefab(go, path);
            Debug.Log("[CutRopeSetup] Created: " + path);
        }

        static void CreateCandyPrefab()
        {
            const string path = "Assets/DefaultPackage/Prefabs/Game/Candy.prefab";
            if (AssetDatabase.LoadAssetAtPath<GameObject>(path) != null) return;

            var go = new GameObject("Candy");

            var sr = go.AddComponent<SpriteRenderer>();
            sr.sprite = AssetDatabase.GetBuiltinExtraResource<Sprite>("UI/Skin/Knob.psd");
            sr.color  = new Color(1f, 0.4f, 0.6f);
            go.transform.localScale = Vector3.one * 0.5f;

            var rb = go.AddComponent<Rigidbody2D>();
            rb.mass        = 0.3f;
            rb.drag        = 0.2f;
            rb.gravityScale = 1f;

            var col = go.AddComponent<CircleCollider2D>();
            col.radius = 0.5f;

            var candyType = GetTypeByName("CutRope.Game.Candy");
            if (candyType != null) go.AddComponent(candyType);
            else Debug.LogWarning("[CutRopeSetup] Candy script not found — add manually.");

            SavePrefab(go, path);
            Debug.Log("[CutRopeSetup] Created: " + path);
        }

        static void CreateLoadingUIPrefab()
        {
            const string path = "Assets/DefaultPackage/UI/Prefab/LoadingUI.prefab";
            if (AssetDatabase.LoadAssetAtPath<GameObject>(path) != null) return;

            // Canvas
            var canvasGo = new GameObject("LoadingUI");
            var canvas   = canvasGo.AddComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            canvas.sortingOrder = 100;
            canvasGo.AddComponent<UnityEngine.UI.CanvasScaler>();
            canvasGo.AddComponent<UnityEngine.UI.GraphicRaycaster>();

            // 黑色背景
            var bg  = new GameObject("Background");
            bg.transform.SetParent(canvasGo.transform, false);
            var bgImg = bg.AddComponent<UnityEngine.UI.Image>();
            bgImg.color = Color.black;
            var bgRect = bg.GetComponent<RectTransform>();
            bgRect.anchorMin = Vector2.zero;
            bgRect.anchorMax = Vector2.one;
            bgRect.sizeDelta = Vector2.zero;

            // 进度条背景
            var barBg  = new GameObject("BarBackground");
            barBg.transform.SetParent(canvasGo.transform, false);
            var barBgImg  = barBg.AddComponent<UnityEngine.UI.Image>();
            barBgImg.color = new Color(0.2f, 0.2f, 0.2f);
            var barBgRect = barBg.GetComponent<RectTransform>();
            barBgRect.anchorMin = new Vector2(0.1f, 0.45f);
            barBgRect.anchorMax = new Vector2(0.9f, 0.55f);
            barBgRect.sizeDelta = Vector2.zero;

            // 进度条填充
            var bar     = new GameObject("Bar");
            bar.transform.SetParent(barBg.transform, false);
            var barImg  = bar.AddComponent<UnityEngine.UI.Image>();
            barImg.color = new Color(0.2f, 0.8f, 0.3f);
            var barRect = bar.GetComponent<RectTransform>();
            barRect.anchorMin = Vector2.zero;
            barRect.anchorMax = new Vector2(0.5f, 1f); // 初始50%，Lua 动态更新
            barRect.sizeDelta = Vector2.zero;

            // LoadingUIController 脚本
            var ctrlType = GetTypeByName("CutRope.Framework.LoadingUIController");
            if (ctrlType != null)
            {
                var ctrl = canvasGo.AddComponent(ctrlType) as MonoBehaviour;
                // 通过 SerializedObject 设置 barRect 引用
                var so = new SerializedObject(ctrl);
                // LoadingUIController 用 Slider，先跳过自动注入，Editor 里手动拖
                so.ApplyModifiedProperties();
            }
            else Debug.LogWarning("[CutRopeSetup] LoadingUIController not found — add manually.");

            SavePrefab(canvasGo, path);
            Debug.Log("[CutRopeSetup] Created: " + path);
        }

        static void CreateMainMenuCanvasPrefab()
        {
            const string path = "Assets/DefaultPackage/UI/Prefab/MainMenu.prefab";
            if (AssetDatabase.LoadAssetAtPath<GameObject>(path) != null) return;

            var canvasGo = new GameObject("MainMenu");
            var canvas   = canvasGo.AddComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            canvasGo.AddComponent<UnityEngine.UI.CanvasScaler>();
            canvasGo.AddComponent<UnityEngine.UI.GraphicRaycaster>();

            // 标题
            var titleGo = new GameObject("Title");
            titleGo.transform.SetParent(canvasGo.transform, false);
            var title = titleGo.AddComponent<TMPro.TextMeshProUGUI>();
            title.text      = "CUT THE ROPE";
            title.fontSize  = 72;
            title.alignment = TMPro.TextAlignmentOptions.Center;
            title.color     = new Color(1f, 0.85f, 0.1f);
            var titleRect = titleGo.GetComponent<RectTransform>();
            titleRect.anchorMin = new Vector2(0f, 0.6f);
            titleRect.anchorMax = new Vector2(1f, 0.85f);
            titleRect.sizeDelta = Vector2.zero;

            // Play 按钮
            CreateButton(canvasGo.transform, "BtnPlay",  "PLAY",
                new Vector2(0.3f, 0.35f), new Vector2(0.7f, 0.5f), new Color(0.2f, 0.8f, 0.3f));

            // UIController 脚本
            var ctrlType = GetTypeByName("CutRope.Game.UIController");
            if (ctrlType != null) canvasGo.AddComponent(ctrlType);
            else Debug.LogWarning("[CutRopeSetup] UIController not found — add manually.");

            SavePrefab(canvasGo, path);
            Debug.Log("[CutRopeSetup] Created: " + path);
        }

        static void CreateButton(Transform parent, string name, string label,
            Vector2 anchorMin, Vector2 anchorMax, Color color)
        {
            var btnGo  = new GameObject(name);
            btnGo.transform.SetParent(parent, false);
            var img   = btnGo.AddComponent<UnityEngine.UI.Image>();
            img.color = color;
            var btn   = btnGo.AddComponent<UnityEngine.UI.Button>();
            var rect  = btnGo.GetComponent<RectTransform>();
            rect.anchorMin = anchorMin;
            rect.anchorMax = anchorMax;
            rect.sizeDelta = Vector2.zero;

            var textGo  = new GameObject("Label");
            textGo.transform.SetParent(btnGo.transform, false);
            var tmp = textGo.AddComponent<TMPro.TextMeshProUGUI>();
            tmp.text      = label;
            tmp.fontSize  = 36;
            tmp.alignment = TMPro.TextAlignmentOptions.Center;
            tmp.color     = Color.white;
            var textRect = textGo.GetComponent<RectTransform>();
            textRect.anchorMin = Vector2.zero;
            textRect.anchorMax = Vector2.one;
            textRect.sizeDelta = Vector2.zero;
        }

        // ── 场景 ──────────────────────────────────────────────────────

        static void CreateGameLauncherScene()
        {
            const string path = "Assets/Scenes/GameLauncher.unity";
            EnsureDir("Assets/Scenes");
            if (File.Exists(path)) return;

            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Additive);

            // Root GameObject — 所有 Framework 组件挂这里
            var root = new GameObject("GameLauncher");

            var launcherType  = GetTypeByName("CutRope.Framework.GameLauncher");
            var bootstrapType = GetTypeByName("CutRope.Game.GameBootstrap");
            if (launcherType  != null) root.AddComponent(launcherType);
            if (bootstrapType != null) root.AddComponent(bootstrapType);

            // 摄像机
            var camGo = new GameObject("Main Camera");
            camGo.tag = "MainCamera";
            var cam = camGo.AddComponent<Camera>();
            cam.clearFlags        = CameraClearFlags.SolidColor;
            cam.backgroundColor   = Color.black;
            cam.orthographic      = true;
            cam.orthographicSize  = 9f;

            EditorSceneManager.SaveScene(scene, path);
            EditorSceneManager.CloseScene(scene, true);
            Debug.Log("[CutRopeSetup] Created scene: " + path);
        }

        static void CreateMainMenuScene()
        {
            const string path = "Assets/DefaultPackage/Scenes/MainMenu.unity";
            EnsureDir("Assets/DefaultPackage/Scenes");
            if (File.Exists(path)) return;

            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Additive);

            var camGo = new GameObject("Main Camera");
            camGo.tag = "MainCamera";
            var cam = camGo.AddComponent<Camera>();
            cam.clearFlags       = CameraClearFlags.SolidColor;
            cam.backgroundColor  = new Color(0.1f, 0.6f, 0.9f); // 蓝天
            cam.orthographic     = true;
            cam.orthographicSize = 9f;

            // 占位背景（正式版换成 Sprite）
            var bg  = new GameObject("Background");
            var bgSr = bg.AddComponent<SpriteRenderer>();
            bgSr.color = new Color(0.1f, 0.6f, 0.9f);

            EditorSceneManager.SaveScene(scene, path);
            EditorSceneManager.CloseScene(scene, true);
            Debug.Log("[CutRopeSetup] Created scene: " + path);
        }

        static void CreateLevel1_1Scene()
        {
            const string path = "Assets/DefaultPackage/Scenes/Level_1_1.unity";
            EnsureDir("Assets/DefaultPackage/Scenes");
            if (File.Exists(path)) return;

            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Additive);

            // 摄像机
            var camGo = new GameObject("Main Camera");
            camGo.tag = "MainCamera";
            var cam = camGo.AddComponent<Camera>();
            cam.clearFlags       = CameraClearFlags.SolidColor;
            cam.backgroundColor  = new Color(0.15f, 0.15f, 0.25f);
            cam.orthographic     = true;
            cam.orthographicSize = 9f;

            // LevelController
            var lcGo     = new GameObject("LevelController");
            var lcType   = GetTypeByName("CutRope.Game.LevelController");
            if (lcType != null) lcGo.AddComponent(lcType);

            // CutInput（全屏切割）
            var cutGo   = new GameObject("CutInput");
            var cutType = GetTypeByName("CutRope.Game.CutInput");
            if (cutType != null) cutGo.AddComponent(cutType);

            // 怪兽占位（Monster tag）
            var monster       = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            monster.name      = "Monster";
            monster.tag       = "Monster";
            monster.transform.position = new Vector3(0f, -6f, 0f);
            monster.transform.localScale = Vector3.one * 1.2f;
            var mr = monster.GetComponent<MeshRenderer>();
            if (mr) mr.material = new Material(Shader.Find("Sprites/Default")) { color = new Color(0.3f, 0.8f, 0.3f) };
            // 触发器（怪兽嘴）
            var mouthCol = monster.GetComponent<SphereCollider>();
            mouthCol.isTrigger = true;

            // 绳子挂点（天花板钉子）
            var anchor       = new GameObject("RopeAnchor");
            anchor.transform.position = new Vector3(0f, 7f, 0f);
            var anchorSr     = anchor.AddComponent<SpriteRenderer>();
            anchorSr.sprite  = AssetDatabase.GetBuiltinExtraResource<Sprite>("UI/Skin/Knob.psd");
            anchorSr.color   = Color.gray;
            anchor.transform.localScale = Vector3.one * 0.3f;

            // Candy GameObject（放在绳子末端起始位置，由 Lua/RopeSpawner.Spawn(candy) 连接）
            var candyPrefab = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/DefaultPackage/Prefabs/Game/Candy.prefab");
            GameObject candyGo;
            if (candyPrefab != null)
                candyGo = (GameObject)PrefabUtility.InstantiatePrefab(candyPrefab);
            else
            {
                candyGo      = new GameObject("Candy");
                var candyType = GetTypeByName("CutRope.Game.Candy");
                if (candyType != null) candyGo.AddComponent(candyType);
                var candySr   = candyGo.AddComponent<SpriteRenderer>();
                candySr.sprite = AssetDatabase.GetBuiltinExtraResource<Sprite>("UI/Skin/Knob.psd");
                candySr.color  = new Color(1f, 0.4f, 0.6f);
                candyGo.AddComponent<Rigidbody2D>();
                candyGo.AddComponent<CircleCollider2D>();
            }
            // 放在锚点正下方，Lua level_controller 会在 on_ready 里调用 RopeSpawner.Spawn(candy)
            candyGo.transform.position = anchor.transform.position + Vector3.down * 2f;

            // RopeSpawner（由 Lua 调用 Spawn(candy)，无需在此自动触发）
            var rsGo   = new GameObject("RopeSpawner");
            var rsType = GetTypeByName("CutRope.Game.RopeSpawner");
            if (rsType != null)
            {
                var rs = rsGo.AddComponent(rsType) as MonoBehaviour;
                rsGo.transform.position = anchor.transform.position;

                var segPrefab = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/DefaultPackage/Prefabs/Rope/RopeSegment.prefab");
                if (rs != null && segPrefab != null)
                {
                    var so = new SerializedObject(rs);
                    var segProp = so.FindProperty("segmentPrefab");
                    if (segProp != null) { segProp.objectReferenceValue = segPrefab; }
                    so.ApplyModifiedProperties();
                }
            }
            else Debug.LogWarning("[CutRopeSetup] RopeSpawner not found.");

            EditorSceneManager.SaveScene(scene, path);
            EditorSceneManager.CloseScene(scene, true);
            Debug.Log("[CutRopeSetup] Created scene: " + path);
        }

        // ── Build Settings ────────────────────────────────────────────

        public static void AddScenesToBuildSettings()
        {
            var scenePaths = new[]
            {
                "Assets/Scenes/GameLauncher.unity",  //启动场景，其他场景通过YooAsset打包
            };

            var existing = new List<EditorBuildSettingsScene>(EditorBuildSettings.scenes);
            foreach (var sp in scenePaths)
            {
                bool found = false;
                foreach (var e in existing)
                    if (e.path == sp) { found = true; break; }
                if (!found)
                    existing.Add(new EditorBuildSettingsScene(sp, true));
            }
            EditorBuildSettings.scenes = existing.ToArray();
            Debug.Log("[CutRopeSetup] Build Settings updated.");
        }

        // ── 工具方法 ─────────────────────────────────────────────────

        static System.Type GetTypeByName(string fullName)
        {
            foreach (var asm in System.AppDomain.CurrentDomain.GetAssemblies())
            {
                var t = asm.GetType(fullName);
                if (t != null) return t;
            }
            return null;
        }
    }
}
