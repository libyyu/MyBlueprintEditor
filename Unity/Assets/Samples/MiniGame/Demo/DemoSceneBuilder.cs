// DemoSceneBuilder.cs — 一键生成 AI 酒馆 Demo 场景
// 全部使用 UnityEngine.UI.Text（不依赖 TMP，天然支持中文）

#if UNITY_EDITOR
using UnityEngine;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine.UI;
using BlueprintRuntime.Samples.AINpc;
using BlueprintRuntime.Samples.AINpc.OpenWorld;
using BlueprintRuntime.Samples.MiniGame;
using BlueprintRuntime.Samples.MiniGame.Storage;

namespace BlueprintRuntime.Samples.MiniGame.Demo
{
    public static class DemoSceneBuilder
    {
        struct NpcDef
        {
            public string name, displayName, personality;
            public Color color;
            public Vector3 position;
        }

        static readonly NpcDef[] NPCs = {
            new NpcDef {
                name = "NPC_MeowMeow", displayName = "喵喵店长",
                personality = "你是一家幻想世界酒馆的猫咪店长，叫喵喵。说话温柔好奇，句尾常加\"喵~\"。玩家是冒险者。用 2~3 句话回复。",
                color = new Color(0.95f, 0.6f, 0.2f), position = new Vector3(-2f, 0f, 3f)
            },
            new NpcDef {
                name = "NPC_IronHammer", displayName = "铁锤大叔",
                personality = "你是酒馆里的铁匠铁锤，壮汉，声如洪钟，豪爽直接。喜欢喝酒和讲冒险故事。用 2~3 句话回复，语气粗犷。",
                color = new Color(0.7f, 0.3f, 0.2f), position = new Vector3(3f, 0f, 2f)
            },
            new NpcDef {
                name = "NPC_Luna", displayName = "月灵术士",
                personality = "你是酒馆常客月灵，一个神秘的精灵术士。说话文雅、略带高冷，对魔法和星辰很有研究。用 2~3 句话回复。",
                color = new Color(0.4f, 0.3f, 0.8f), position = new Vector3(-3f, 0f, -1f)
            },
            new NpcDef {
                name = "NPC_Pippin", displayName = "皮皮鼠",
                personality = "你是酒馆的吉祥物皮皮，一只会说话的小老鼠。胆小但超级八卦，爱用\"嘻嘻\"。用 2~3 句话回复，语气活泼。",
                color = new Color(0.5f, 0.8f, 0.3f), position = new Vector3(1f, 0f, -2f)
            },
        };

        [MenuItem("Blueprint/生成 AI 酒馆 Demo 场景", false, 200)]
        public static void BuildDemoScene()
        {
            if (!EditorUtility.DisplayDialog("生成 AI 酒馆 Demo",
                "将生成完整 AI 酒馆场景（纯 UI.Text，无 TMP 依赖）。\n继续？", "生成", "取消"))
                return;

            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
            PlayerSettings.defaultScreenWidth = 750;
            PlayerSettings.defaultScreenHeight = 1334;

            CreateEnvironment();
            var player = CreatePlayer();
            CreateBootstrap();
            foreach (var def in NPCs) CreateNPC(def, player.transform);
            var dialogUI = Create2DDialogUI();

            // EventSystem
            if (!Object.FindObjectOfType<UnityEngine.EventSystems.EventSystem>())
            {
                var es = new GameObject("EventSystem");
                es.AddComponent<UnityEngine.EventSystems.EventSystem>();
                es.AddComponent<UnityEngine.EventSystems.StandaloneInputModule>();
            }

            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene, "Assets/AITavernDemo.unity");
            Debug.Log("✅ AI 酒馆 Demo 场景已生成！点 Play 试玩。WASD 移动，右键旋转相机，靠近 NPC 按 E 对话。");
        }

        // ══════════════════════════════════════════════════════════════
        // 环境
        // ══════════════════════════════════════════════════════════════
        static void CreateEnvironment()
        {
            // 地面
            var floor = GameObject.CreatePrimitive(PrimitiveType.Plane);
            floor.name = "Floor";
            floor.transform.localScale = new Vector3(3, 1, 3);
            SetColor(floor, new Color(0.35f, 0.25f, 0.18f));

            // 墙壁
            MakeBox("WallBack",  new Vector3(0, 2.5f, 15),  new Vector3(30, 5, 0.3f), new Color(0.4f, 0.3f, 0.2f));
            MakeBox("WallLeft",  new Vector3(-15, 2.5f, 0), new Vector3(0.3f, 5, 30), new Color(0.38f, 0.28f, 0.2f));
            MakeBox("WallRight", new Vector3(15, 2.5f, 0),  new Vector3(0.3f, 5, 30), new Color(0.38f, 0.28f, 0.2f));

            // 吧台
            MakeBox("BarCounter", new Vector3(-2, 0.6f, 5), new Vector3(6, 1.2f, 1), new Color(0.3f, 0.2f, 0.1f));
            MakeBox("Table1", new Vector3(3, 0.4f, 0),   new Vector3(1.5f, 0.8f, 1.5f), new Color(0.45f, 0.3f, 0.15f));
            MakeBox("Table2", new Vector3(-3, 0.4f, -3),  new Vector3(1.5f, 0.8f, 1.5f), new Color(0.45f, 0.3f, 0.15f));

            // 灯光
            var light = new GameObject("MainLight").AddComponent<Light>();
            light.transform.position = new Vector3(0, 4.5f, 0);
            light.transform.rotation = Quaternion.Euler(50, -30, 0);
            light.type = LightType.Directional;
            light.color = new Color(1f, 0.85f, 0.6f);
            light.intensity = 0.8f;
            light.shadows = LightShadows.Soft;

            var fire = new GameObject("FireLight").AddComponent<Light>();
            fire.transform.position = new Vector3(0, 1.5f, 13);
            fire.type = LightType.Point;
            fire.color = new Color(1f, 0.5f, 0.1f);
            fire.intensity = 2f;
            fire.range = 10f;

            RenderSettings.ambientMode = UnityEngine.Rendering.AmbientMode.Flat;
            RenderSettings.ambientLight = new Color(0.15f, 0.1f, 0.08f);
        }

        // ══════════════════════════════════════════════════════════════
        // 玩家
        // ══════════════════════════════════════════════════════════════
        static GameObject CreatePlayer()
        {
            var go = GameObject.CreatePrimitive(PrimitiveType.Capsule);
            go.name = "Player";
            go.tag = "Player";
            go.transform.position = new Vector3(0, 0, -5);
            SetColor(go, new Color(0.2f, 0.5f, 0.9f));

            Object.DestroyImmediate(go.GetComponent<CapsuleCollider>());
            var cc = go.AddComponent<CharacterController>();
            cc.height = 2f; cc.radius = 0.4f; cc.center = new Vector3(0, 1, 0);

            go.AddComponent<SimplePlayerController>();

            var cam = new GameObject("Main Camera");
            cam.tag = "MainCamera";
            cam.transform.SetParent(go.transform);
            cam.transform.localPosition = new Vector3(0, 6, -3);
            cam.transform.localRotation = Quaternion.Euler(40, 0, 0);
            var c = cam.AddComponent<Camera>();
            c.clearFlags = CameraClearFlags.Skybox;
            c.fieldOfView = 50;
            c.nearClipPlane = 0.1f;
            cam.AddComponent<AudioListener>();

            return go;
        }

        // ══════════════════════════════════════════════════════════════
        // NPC
        // ══════════════════════════════════════════════════════════════
        static void CreateNPC(NpcDef def, Transform player)
        {
            var go = GameObject.CreatePrimitive(PrimitiveType.Capsule);
            go.name = def.name;
            go.transform.position = def.position + Vector3.up;
            SetColor(go, def.color);

            // 头顶名牌（World Space Canvas + UI.Text）
            var nameGO = new GameObject("Nameplate");
            nameGO.transform.SetParent(go.transform);
            nameGO.transform.localPosition = new Vector3(0, 1.4f, 0);
            var nameCanvas = nameGO.AddComponent<Canvas>();
            nameCanvas.renderMode = RenderMode.WorldSpace;
            nameGO.GetComponent<RectTransform>().sizeDelta = new Vector2(200, 40);
            nameGO.transform.localScale = Vector3.one * 0.01f;
            var nameText = MakeUIText(nameGO, "Name", def.displayName, 28, def.color, TextAnchor.MiddleCenter);

            // Controller
            var ctrl = go.AddComponent<AINpcStreamingController>();
            Set(ctrl, "npcName", def.displayName);
            Set(ctrl, "personality", def.personality);
            Set(ctrl, "cooldown", 1.0f);

            var bjson = AssetDatabase.LoadAssetAtPath<TextAsset>("Assets/Data/examples/wxgame/AI_NPC_Streaming.bjson");
            if (bjson != null) Set(ctrl, "streamingBlueprint", bjson);
            else Debug.LogWarning($"[DemoBuilder] 未找到 AI_NPC_Streaming.bjson");

            // ProximityTrigger
            var trigger = go.AddComponent<NpcProximityTrigger>();
            Set(trigger, "player", player);
            Set(trigger, "streamNpc", ctrl);
            Set(trigger, "greetRadius", 8f);
            Set(trigger, "interactRadius", 3f);

            // Memory
            var mem = go.AddComponent<NpcMemory>();
            Set(mem, "npcId", def.name);
        }

        // ══════════════════════════════════════════════════════════════
        // 2D 对话 UI（纯 UnityEngine.UI）
        // ══════════════════════════════════════════════════════════════
        static GameObject Create2DDialogUI()
        {
            // Canvas
            var canvasGO = new GameObject("UICanvas");
            canvasGO.layer = 5;
            var canvas = canvasGO.AddComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            canvas.sortingOrder = 100;
            var scaler = canvasGO.AddComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(750, 1334);
            scaler.matchWidthOrHeight = 0.5f;
            canvasGO.AddComponent<GraphicRaycaster>();

            // ── 按 E 提示 ──
            var prompt = MakePanel(canvasGO, "PromptPanel",
                new Vector2(0, 0.15f), new Vector2(1, 0.2f), new Color(0, 0, 0, 0.7f));
            prompt.SetActive(false);
            var promptText = MakeUIText(prompt, "Text", "按 [E] 与 NPC 对话", 26, Color.white, TextAnchor.MiddleCenter);

            // ── 对话面板（底部 45%） ──
            var dialog = MakePanel(canvasGO, "DialogPanel",
                new Vector2(0, 0), new Vector2(1, 0.45f), new Color(0.05f, 0.05f, 0.08f, 0.92f));
            dialog.SetActive(false);

            // NPC 名字栏
            var nameBar = MakePanel(dialog, "NameBar",
                new Vector2(0, 0.88f), new Vector2(1, 1), new Color(0.15f, 0.12f, 0.1f, 0.95f));
            // 头像色块
            var avatar = MakePanel(nameBar, "Avatar",
                new Vector2(0, 0), new Vector2(0.08f, 1), Color.white);
            var avatarImg = avatar.GetComponent<Image>();
            // 名字
            var npcName = MakeUIText(nameBar, "NpcName", "NPC", 24,
                new Color(1f, 0.85f, 0.4f), TextAnchor.MiddleLeft);
            npcName.GetComponent<RectTransform>().offsetMin = new Vector2(60, 0);

            // NPC 当前对话（大字）
            var dialogArea = MakePanel(dialog, "DialogArea",
                new Vector2(0.02f, 0.25f), new Vector2(0.98f, 0.86f), new Color(0, 0, 0, 0.3f));
            var dialogScroll = dialogArea.AddComponent<ScrollRect>();
            dialogScroll.horizontal = false;

            var dialogContent = new GameObject("Content");
            dialogContent.transform.SetParent(dialogArea.transform, false);
            var dcRT = dialogContent.AddComponent<RectTransform>();
            dcRT.anchorMin = new Vector2(0, 1); dcRT.anchorMax = Vector2.one;
            dcRT.pivot = new Vector2(0.5f, 1);
            dcRT.offsetMin = Vector2.zero; dcRT.offsetMax = Vector2.zero;
            dialogContent.AddComponent<ContentSizeFitter>().verticalFit = ContentSizeFitter.FitMode.PreferredSize;
            dialogScroll.content = dcRT;

            var dialogText = MakeUIText(dialogContent, "DialogText", "", 22, Color.white, TextAnchor.UpperLeft);
            var dtRT = dialogText.GetComponent<RectTransform>();
            dtRT.anchorMin = Vector2.zero; dtRT.anchorMax = Vector2.one;
            dtRT.offsetMin = new Vector2(10, 5); dtRT.offsetMax = new Vector2(-10, -5);

            // 历史记录（小字）
            var historyText = MakeUIText(dialogContent, "HistoryText", "", 16,
                new Color(0.7f, 0.7f, 0.7f), TextAnchor.UpperLeft);
            historyText.transform.SetSiblingIndex(0); // 在 dialogText 上面

            // ── 输入行 ──
            var inputRow = MakePanel(dialog, "InputRow",
                new Vector2(0.02f, 0.02f), new Vector2(0.98f, 0.22f), new Color(0, 0, 0, 0));

            // 输入框
            var inputGO = MakePanel(inputRow, "InputField",
                new Vector2(0, 0), new Vector2(0.78f, 1), new Color(0.15f, 0.15f, 0.2f));
            var inputField = inputGO.AddComponent<InputField>();
            var inputText = MakeUIText(inputGO, "Text", "", 20, Color.white, TextAnchor.MiddleLeft);
            inputText.GetComponent<RectTransform>().offsetMin = new Vector2(10, 0);
            inputField.textComponent = inputText.GetComponent<Text>();
            var ph = MakeUIText(inputGO, "Placeholder", "说点什么...", 20,
                new Color(1, 1, 1, 0.3f), TextAnchor.MiddleLeft);
            ph.GetComponent<RectTransform>().offsetMin = new Vector2(10, 0);
            inputField.placeholder = ph.GetComponent<Text>();

            // 发送按钮
            var btnGO = MakePanel(inputRow, "SendBtn",
                new Vector2(0.8f, 0), new Vector2(1, 1), new Color(0.2f, 0.65f, 0.35f));
            var btn = btnGO.AddComponent<Button>();
            btn.targetGraphic = btnGO.GetComponent<Image>();
            MakeUIText(btnGO, "Text", "发送", 20, Color.white, TextAnchor.MiddleCenter);

            // ── 左上角帮助 ──
            var help = MakePanel(canvasGO, "HelpPanel",
                new Vector2(0, 0.88f), new Vector2(0.45f, 1), new Color(0, 0, 0, 0.5f));
            MakeUIText(help, "Text",
                "<b>AI 魔法酒馆</b>\nWASD 移动 | 右键旋转\nE 对话 | ESC 关闭",
                14, new Color(0.8f, 0.8f, 0.8f), TextAnchor.UpperLeft)
                .GetComponent<RectTransform>().offsetMin = new Vector2(8, 4);

            // ── ScreenDialogUI 组件 ──
            var ui = canvasGO.AddComponent<ScreenDialogUI>();
            Set(ui, "promptPanel", prompt);
            Set(ui, "promptText", promptText.GetComponent<Text>());
            Set(ui, "dialogPanel", dialog);
            Set(ui, "npcNameText", npcName.GetComponent<Text>());
            Set(ui, "npcAvatarImage", avatarImg);
            Set(ui, "dialogText", dialogText.GetComponent<Text>());
            Set(ui, "historyText", historyText.GetComponent<Text>());
            Set(ui, "inputField", inputField);
            Set(ui, "sendButton", btn);
            Set(ui, "historyScroll", dialogScroll);

            return canvasGO;
        }

        // ══════════════════════════════════════════════════════════════
        // Bootstrap
        // ══════════════════════════════════════════════════════════════
        static void CreateBootstrap()
        {
            var go = new GameObject("Bootstrap");
            var boot = go.AddComponent<MiniGameBootstrap>();
            var cfg = AssetDatabase.LoadAssetAtPath<LlmProxyConfig>("Assets/LlmConfig.asset");
            if (cfg != null) Set(boot, "llmConfig", cfg);
            else Debug.LogWarning("[DemoBuilder] 未找到 Assets/LlmConfig.asset");
        }

        // ══════════════════════════════════════════════════════════════
        // 工具函数
        // ══════════════════════════════════════════════════════════════
        static void MakeBox(string name, Vector3 pos, Vector3 scale, Color c)
        {
            var go = GameObject.CreatePrimitive(PrimitiveType.Cube);
            go.name = name; go.transform.position = pos; go.transform.localScale = scale;
            SetColor(go, c);
        }

        static void SetColor(GameObject go, Color c)
        {
            var r = go.GetComponent<Renderer>();
            if (r == null) return;
            var m = new Material(Shader.Find("Standard"));
            m.color = c; r.material = m;
        }

        static GameObject MakePanel(GameObject parent, string name,
            Vector2 anchorMin, Vector2 anchorMax, Color bgColor)
        {
            var go = new GameObject(name);
            go.layer = 5;
            go.transform.SetParent(parent.transform, false);
            var rt = go.AddComponent<RectTransform>();
            rt.anchorMin = anchorMin; rt.anchorMax = anchorMax;
            rt.offsetMin = Vector2.zero; rt.offsetMax = Vector2.zero;
            var img = go.AddComponent<Image>();
            img.color = bgColor;
            return go;
        }

        static GameObject MakeUIText(GameObject parent, string name,
            string text, int fontSize, Color color, TextAnchor align)
        {
            var go = new GameObject(name);
            go.layer = 5;
            go.transform.SetParent(parent.transform, false);
            var rt = go.AddComponent<RectTransform>();
            rt.anchorMin = Vector2.zero; rt.anchorMax = Vector2.one;
            rt.offsetMin = Vector2.zero; rt.offsetMax = Vector2.zero;
            var t = go.AddComponent<Text>();
            t.text = text;
            t.fontSize = fontSize;
            t.color = color;
            t.alignment = align;
            t.horizontalOverflow = HorizontalWrapMode.Wrap;
            t.verticalOverflow = VerticalWrapMode.Overflow;
            t.supportRichText = true;
            t.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            // 如果Unity版本不支持 LegacyRuntime，fallback 到 Arial
            if (t.font == null) t.font = Resources.GetBuiltinResource<Font>("Arial.ttf");
            if (t.font == null) t.font = Font.CreateDynamicFontFromOSFont("Microsoft YaHei", fontSize);
            return go;
        }

        static void Set(object target, string name, object value)
        {
            for (var type = target.GetType(); type != null; type = type.BaseType)
            {
                var f = type.GetField(name,
                    System.Reflection.BindingFlags.Instance |
                    System.Reflection.BindingFlags.NonPublic |
                    System.Reflection.BindingFlags.Public);
                if (f != null) { f.SetValue(target, value); return; }
            }
        }
    }
}
#endif
