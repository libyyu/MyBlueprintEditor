// DemoSceneBuilder.cs
// ─────────────────────────────────────────────────────────────────────────────
// Editor 菜单：Blueprint → 生成 AI 酒馆 Demo 场景
// 一键创建完整可玩场景，包括：
//   - 地面 + 酒馆灯光
//   - 可控玩家（WASD + 鼠标）
//   - 4 个 AI NPC（不同人设 + 颜色）
//   - 头顶气泡 + "按 E 对话" 提示
//   - 全屏对话 UI（输入框 + 发送按钮 + 历史记录）
//   - Bootstrap（自动初始化所有系统）
// ─────────────────────────────────────────────────────────────────────────────

#if UNITY_EDITOR
using UnityEngine;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine.SceneManagement;
using UnityEngine.UI;
using TMPro;
using BlueprintRuntime.Samples.AINpc;
using BlueprintRuntime.Samples.AINpc.OpenWorld;
using BlueprintRuntime.Samples.MiniGame;
using BlueprintRuntime.Samples.MiniGame.Storage;

namespace BlueprintRuntime.Samples.MiniGame.Demo
{
    public static class DemoSceneBuilder
    {
        // NPC 定义
        struct NpcDef
        {
            public string name;
            public string displayName;
            public string personality;
            public Color  color;
            public Vector3 position;
            public string blueprintAsset; // bjson 文件名（Assets/Data/examples/wxgame/下）
        }

        static readonly NpcDef[] NPCs = new NpcDef[]
        {
            new NpcDef {
                name = "NPC_MeowMeow",
                displayName = "喵喵店长",
                personality = "你是一家幻想世界酒馆的猫咪店长，叫喵喵。说话温柔好奇，句尾常加\"喵~\"。玩家是冒险者。用 2~3 句话回复。",
                color = new Color(0.95f, 0.6f, 0.2f),  // 橙色
                position = new Vector3(-2f, 0f, 3f),
                blueprintAsset = "AI_NPC_Streaming"
            },
            new NpcDef {
                name = "NPC_IronHammer",
                displayName = "铁锤大叔",
                personality = "你是酒馆里的铁匠铁锤，壮汉，声如洪钟，豪爽直接。喜欢喝酒和讲冒险故事。偶尔推荐自己打的武器。用 2~3 句话回复，语气粗犷。",
                color = new Color(0.7f, 0.3f, 0.2f),   // 深红
                position = new Vector3(3f, 0f, 2f),
                blueprintAsset = "AI_NPC_Streaming"
            },
            new NpcDef {
                name = "NPC_Luna",
                displayName = "月灵术士",
                personality = "你是酒馆常客月灵，一个神秘的精灵术士。说话文雅、略带高冷，对魔法和星辰很有研究。偶尔说出令人深思的话。用 2~3 句话回复。",
                color = new Color(0.4f, 0.3f, 0.8f),   // 紫色
                position = new Vector3(-3f, 0f, -1f),
                blueprintAsset = "AI_NPC_Streaming"
            },
            new NpcDef {
                name = "NPC_Pippin",
                displayName = "皮皮鼠",
                personality = "你是酒馆的吉祥物皮皮，一只会说话的小老鼠。胆小但超级八卦，喜欢偷听别人说话然后告诉玩家。说话快，爱用\"嘻嘻\"。用 2~3 句话回复，语气活泼。",
                color = new Color(0.5f, 0.8f, 0.3f),   // 绿色
                position = new Vector3(1f, 0f, -2f),
                blueprintAsset = "AI_NPC_Streaming"
            },
        };

        [MenuItem("Blueprint/生成 AI 酒馆 Demo 场景", false, 200)]
        public static void BuildDemoScene()
        {
            if (!EditorUtility.DisplayDialog("生成 AI 酒馆 Demo",
                "将清空当前场景并生成完整的 AI 酒馆 Demo 场景。\n\n" +
                "确保已完成：\n" +
                "1. Assets/ 下有 LlmConfig.asset（填了 API Key）\n" +
                "2. Assets/Data/examples/wxgame/ 下有 .bjson 蓝图\n" +
                "3. 已导入 TextMeshPro（中文字体更佳）\n\n" +
                "继续？", "生成", "取消"))
                return;

            // 新建空场景
            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);

            // ── 1. 环境 ─────────────────────────────────────────────────
            CreateEnvironment();

            // ── 2. Player ───────────────────────────────────────────────
            var player = CreatePlayer();

            // ── 3. Bootstrap ────────────────────────────────────────────
            CreateBootstrap();

            // ── 4. NPC ──────────────────────────────────────────────────
            foreach (var def in NPCs)
                CreateNPC(def, player.transform);

            // ── 5. UI Canvas ────────────────────────────────────────────
            CreateUICanvas();

            // ── 6. EventSystem ──────────────────────────────────────────
            if (Object.FindObjectOfType<UnityEngine.EventSystems.EventSystem>() == null)
            {
                var es = new GameObject("EventSystem");
                es.AddComponent<UnityEngine.EventSystems.EventSystem>();
                es.AddComponent<UnityEngine.EventSystems.StandaloneInputModule>();
            }

            // 保存
            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene, "Assets/AITavernDemo.unity");
            Debug.Log("✅ AI 酒馆 Demo 场景已生成：Assets/AITavernDemo.unity");

            Selection.activeGameObject = player;
            SceneView.lastActiveSceneView?.FrameSelected();
        }

        // ── 环境 ────────────────────────────────────────────────────────
        static void CreateEnvironment()
        {
            // 地面
            var floor = GameObject.CreatePrimitive(PrimitiveType.Plane);
            floor.name = "TavernFloor";
            floor.transform.localScale = new Vector3(3, 1, 3);
            var floorMat = new Material(Shader.Find("Standard"));
            floorMat.color = new Color(0.35f, 0.25f, 0.18f); // 深棕木地板
            floor.GetComponent<Renderer>().material = floorMat;

            // 天花板（可选视觉效果）
            var ceiling = GameObject.CreatePrimitive(PrimitiveType.Plane);
            ceiling.name = "Ceiling";
            ceiling.transform.position = new Vector3(0, 5, 0);
            ceiling.transform.rotation = Quaternion.Euler(180, 0, 0);
            ceiling.transform.localScale = new Vector3(3, 1, 3);
            var ceilingMat = new Material(Shader.Find("Standard"));
            ceilingMat.color = new Color(0.25f, 0.18f, 0.12f);
            ceiling.GetComponent<Renderer>().material = ceilingMat;

            // 墙壁
            CreateWall("WallBack",  new Vector3(0, 2.5f, 15), new Vector3(30, 5, 0.2f), new Color(0.4f, 0.3f, 0.2f));
            CreateWall("WallLeft",  new Vector3(-15, 2.5f, 0), new Vector3(0.2f, 5, 30), new Color(0.38f, 0.28f, 0.2f));
            CreateWall("WallRight", new Vector3(15, 2.5f, 0), new Vector3(0.2f, 5, 30), new Color(0.38f, 0.28f, 0.2f));

            // 主光（暖色调酒馆灯光）
            var lightGO = new GameObject("TavernLight");
            lightGO.transform.position = new Vector3(0, 4.5f, 0);
            lightGO.transform.rotation = Quaternion.Euler(50, -30, 0);
            var light = lightGO.AddComponent<Light>();
            light.type = LightType.Directional;
            light.color = new Color(1f, 0.85f, 0.6f); // 暖色
            light.intensity = 0.8f;
            light.shadows = LightShadows.Soft;

            // 点光源（壁炉效果）
            var firelight = new GameObject("FireLight");
            firelight.transform.position = new Vector3(0, 1.5f, 13);
            var fl = firelight.AddComponent<Light>();
            fl.type = LightType.Point;
            fl.color = new Color(1f, 0.5f, 0.1f);
            fl.intensity = 2f;
            fl.range = 10f;

            // 吧台（一个长方体）
            var bar = GameObject.CreatePrimitive(PrimitiveType.Cube);
            bar.name = "BarCounter";
            bar.transform.position = new Vector3(-2, 0.6f, 5);
            bar.transform.localScale = new Vector3(6, 1.2f, 1);
            var barMat = new Material(Shader.Find("Standard"));
            barMat.color = new Color(0.3f, 0.2f, 0.1f);
            bar.GetComponent<Renderer>().material = barMat;

            // 桌子
            CreateTable(new Vector3(3, 0, 0));
            CreateTable(new Vector3(-3, 0, -3));

            // 环境光
            RenderSettings.ambientMode = UnityEngine.Rendering.AmbientMode.Flat;
            RenderSettings.ambientLight = new Color(0.15f, 0.1f, 0.08f);
        }

        static void CreateWall(string name, Vector3 pos, Vector3 scale, Color color)
        {
            var wall = GameObject.CreatePrimitive(PrimitiveType.Cube);
            wall.name = name;
            wall.transform.position = pos;
            wall.transform.localScale = scale;
            var mat = new Material(Shader.Find("Standard"));
            mat.color = color;
            wall.GetComponent<Renderer>().material = mat;
        }

        static void CreateTable(Vector3 pos)
        {
            var table = GameObject.CreatePrimitive(PrimitiveType.Cube);
            table.name = "Table";
            table.transform.position = pos + Vector3.up * 0.4f;
            table.transform.localScale = new Vector3(1.5f, 0.8f, 1.5f);
            var mat = new Material(Shader.Find("Standard"));
            mat.color = new Color(0.45f, 0.3f, 0.15f);
            table.GetComponent<Renderer>().material = mat;
        }

        // ── 玩家 ────────────────────────────────────────────────────────
        static GameObject CreatePlayer()
        {
            var player = GameObject.CreatePrimitive(PrimitiveType.Capsule);
            player.name = "Player";
            player.tag = "Player";
            player.transform.position = new Vector3(0, 1, -5);

            // 玩家颜色
            var mat = new Material(Shader.Find("Standard"));
            mat.color = new Color(0.2f, 0.5f, 0.9f);
            player.GetComponent<Renderer>().material = mat;

            // 物理
            var rb = player.AddComponent<Rigidbody>();
            rb.constraints = RigidbodyConstraints.FreezeRotation;
            rb.collisionDetectionMode = CollisionDetectionMode.Continuous;

            // 控制器
            player.AddComponent<SimplePlayerController>();

            // 相机挂到玩家
            var camGO = new GameObject("Main Camera");
            camGO.tag = "MainCamera";
            camGO.transform.SetParent(player.transform);
            camGO.transform.localPosition = new Vector3(0, 0.5f, 0);
            var cam = camGO.AddComponent<Camera>();
            cam.clearFlags = CameraClearFlags.Skybox;
            cam.fieldOfView = 70;
            cam.nearClipPlane = 0.1f;
            camGO.AddComponent<AudioListener>();

            return player;
        }

        // ── NPC ─────────────────────────────────────────────────────────
        static void CreateNPC(NpcDef def, Transform player)
        {
            // 身体
            var npc = GameObject.CreatePrimitive(PrimitiveType.Capsule);
            npc.name = def.name;
            npc.transform.position = def.position + Vector3.up;

            var mat = new Material(Shader.Find("Standard"));
            mat.color = def.color;
            npc.GetComponent<Renderer>().material = mat;

            // Trigger Collider（ProximityTrigger 用距离检测，但 SphereCollider 做视觉参考）
            var col = npc.GetComponent<CapsuleCollider>();
            if (col) col.isTrigger = false; // 保留物理碰撞

            // 名牌（头顶 3D Text）
            var nameplate = new GameObject("Nameplate");
            nameplate.transform.SetParent(npc.transform);
            nameplate.transform.localPosition = new Vector3(0, 1.5f, 0);
            var nameTMP = nameplate.AddComponent<TextMeshPro>();
            nameTMP.text = def.displayName;
            nameTMP.fontSize = 4;
            nameTMP.alignment = TextAlignmentOptions.Center;
            nameTMP.color = def.color;

            // AINpcStreamingController
            var ctrl = npc.AddComponent<AINpcStreamingController>();
            // 通过反射设私有字段（Inspector 正常可拖）
            SetField(ctrl, "npcName", def.displayName);
            SetField(ctrl, "personality", def.personality);
            SetField(ctrl, "cooldown", 1.0f);

            // 加载蓝图 TextAsset
            string bjsonPath = $"Assets/Data/examples/wxgame/{def.blueprintAsset}.bjson";
            var bjson = AssetDatabase.LoadAssetAtPath<TextAsset>(bjsonPath);
            if (bjson != null)
                SetField(ctrl, "streamingBlueprint", bjson);
            else
                Debug.LogWarning($"[DemoBuilder] 未找到蓝图: {bjsonPath}");

            // NpcProximityTrigger
            var trigger = npc.AddComponent<NpcProximityTrigger>();
            SetField(trigger, "player", player);
            SetField(trigger, "streamNpc", ctrl);
            SetField(trigger, "greetRadius", 8f);
            SetField(trigger, "interactRadius", 3f);

            // NpcMemory
            var memory = npc.AddComponent<NpcMemory>();
            SetField(memory, "npcId", def.name);

            // 头顶气泡
            CreateSpeechBubble(npc, ctrl, player);
        }

        static void CreateSpeechBubble(GameObject npc, AINpcStreamingController ctrl, Transform player)
        {
            var bubbleRoot = new GameObject("SpeechBubble");
            bubbleRoot.transform.SetParent(npc.transform);
            bubbleRoot.transform.localPosition = new Vector3(0, 2.2f, 0);

            // Canvas (World Space)
            var canvas = bubbleRoot.AddComponent<Canvas>();
            canvas.renderMode = RenderMode.WorldSpace;
            var rt = bubbleRoot.GetComponent<RectTransform>();
            rt.sizeDelta = new Vector2(300, 100);
            rt.localScale = Vector3.one * 0.01f;

            var scaler = bubbleRoot.AddComponent<CanvasScaler>();
            scaler.dynamicPixelsPerUnit = 10;

            var canvasGroup = bubbleRoot.AddComponent<CanvasGroup>();

            // 气泡背景
            var bg = new GameObject("BubbleBG");
            bg.transform.SetParent(bubbleRoot.transform, false);
            var bgRT = bg.AddComponent<RectTransform>();
            bgRT.anchorMin = Vector2.zero;
            bgRT.anchorMax = Vector2.one;
            bgRT.offsetMin = Vector2.zero;
            bgRT.offsetMax = Vector2.zero;
            var bgImg = bg.AddComponent<Image>();
            bgImg.color = new Color(0, 0, 0, 0.7f);

            // 文字
            var textGO = new GameObject("BubbleText");
            textGO.transform.SetParent(bubbleRoot.transform, false);
            var textRT = textGO.AddComponent<RectTransform>();
            textRT.anchorMin = new Vector2(0.05f, 0.05f);
            textRT.anchorMax = new Vector2(0.95f, 0.95f);
            textRT.offsetMin = Vector2.zero;
            textRT.offsetMax = Vector2.zero;
            var tmp = textGO.AddComponent<TextMeshProUGUI>();
            tmp.fontSize = 24;
            tmp.color = Color.white;
            tmp.alignment = TextAlignmentOptions.Center;
            tmp.enableWordWrapping = true;
            tmp.text = "";

            // WorldSpeechBubble 组件
            var wsb = bubbleRoot.AddComponent<WorldSpeechBubble>();
            SetField(wsb, "streamNpc", ctrl);
            SetField(wsb, "followTarget", npc.transform);
            SetField(wsb, "worldOffset", new Vector3(0, 2.2f, 0));
            SetField(wsb, "canvasGroup", canvasGroup);
            SetField(wsb, "bubbleText", tmp);
            SetField(wsb, "billboard", true);
            SetField(wsb, "baseScale", 0.01f);
            SetField(wsb, "maxVisibleDistance", 20f);
            SetField(wsb, "autoHideDelay", 8f);
        }

        // ── Bootstrap ───────────────────────────────────────────────────
        static void CreateBootstrap()
        {
            var go = new GameObject("Bootstrap");
            var boot = go.AddComponent<MiniGameBootstrap>();

            // 找 LlmConfig
            var config = AssetDatabase.LoadAssetAtPath<LlmProxyConfig>("Assets/LlmConfig.asset");
            if (config != null)
                SetField(boot, "llmConfig", config);
            else
                Debug.LogWarning("[DemoBuilder] 未找到 Assets/LlmConfig.asset，请手动拖到 Bootstrap 的 LlmConfig 字段");
        }

        // ── UI Canvas ───────────────────────────────────────────────────
        static void CreateUICanvas()
        {
            // Screen Space Canvas
            var canvasGO = new GameObject("UICanvas");
            canvasGO.layer = LayerMask.NameToLayer("UI");
            var canvas = canvasGO.AddComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            canvas.sortingOrder = 100;
            canvasGO.AddComponent<CanvasScaler>().uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            canvasGO.GetComponent<CanvasScaler>().referenceResolution = new Vector2(1920, 1080);
            canvasGO.AddComponent<GraphicRaycaster>();

            // ── 交互提示面板 ──
            var promptPanel = CreateUIElement(canvasGO, "InteractPromptPanel", new Vector2(0.5f, 0.3f));
            var promptBG = promptPanel.AddComponent<Image>();
            promptBG.color = new Color(0, 0, 0, 0.6f);
            var promptRT = promptPanel.GetComponent<RectTransform>();
            promptRT.sizeDelta = new Vector2(400, 60);

            var promptText = CreateTextElement(promptPanel, "PromptText", "按 <color=#FFCC00>[E]</color> 与 NPC 对话", 22);
            promptPanel.SetActive(false);

            var prompt = promptPanel.AddComponent<InteractionPrompt>();
            SetField(prompt, "panel", promptPanel);
            SetField(prompt, "hintText", promptText.GetComponent<TextMeshProUGUI>());

            // ── 对话面板 ──
            CreateDialogPanel(canvasGO);

            // ── 操作提示（左上角） ──
            var helpPanel = CreateUIElement(canvasGO, "HelpPanel",
                new Vector2(0, 1), new Vector2(0, 1), new Vector2(0, 1));
            var helpRT = helpPanel.GetComponent<RectTransform>();
            helpRT.anchoredPosition = new Vector2(15, -15);
            helpRT.sizeDelta = new Vector2(280, 130);
            helpRT.pivot = new Vector2(0, 1);
            var helpBG = helpPanel.AddComponent<Image>();
            helpBG.color = new Color(0, 0, 0, 0.5f);

            var helpText = CreateTextElement(helpPanel, "HelpText",
                "<b>AI 魔法酒馆</b>\n" +
                "<color=#AAA>WASD</color> 移动  <color=#AAA>鼠标</color> 转向\n" +
                "<color=#FFCC00>E</color> 与 NPC 对话\n" +
                "<color=#FFCC00>ESC</color> 关闭对话\n" +
                "<color=#FFCC00>Tab</color> 释放鼠标", 16);
            var helpTMP = helpText.GetComponent<TextMeshProUGUI>();
            helpTMP.alignment = TextAlignmentOptions.TopLeft;
            var helpTextRT = helpText.GetComponent<RectTransform>();
            helpTextRT.anchorMin = new Vector2(0.05f, 0.05f);
            helpTextRT.anchorMax = new Vector2(0.95f, 0.95f);
        }

        static void CreateDialogPanel(GameObject canvas)
        {
            // 对话面板根（默认隐藏）
            var panel = CreateUIElement(canvas, "DialogPanel",
                new Vector2(0.5f, 0), new Vector2(0.5f, 0), new Vector2(0.5f, 0));
            var panelRT = panel.GetComponent<RectTransform>();
            panelRT.anchoredPosition = new Vector2(0, 20);
            panelRT.sizeDelta = new Vector2(700, 400);
            var panelBG = panel.AddComponent<Image>();
            panelBG.color = new Color(0.05f, 0.05f, 0.08f, 0.9f);
            panel.SetActive(false);

            // NPC 名字
            var nameBar = CreateUIElement(panel, "NameBar",
                new Vector2(0, 1), new Vector2(1, 1), new Vector2(0.5f, 1));
            var nameRT = nameBar.GetComponent<RectTransform>();
            nameRT.anchoredPosition = new Vector2(0, 0);
            nameRT.sizeDelta = new Vector2(0, 40);
            var nameBG = nameBar.AddComponent<Image>();
            nameBG.color = new Color(0.15f, 0.12f, 0.1f, 0.95f);

            var nameText = CreateTextElement(nameBar, "NpcNameText", "NPC", 20);
            nameText.GetComponent<TextMeshProUGUI>().color = new Color(1f, 0.85f, 0.4f);

            // 历史滚动区域
            var scrollGO = CreateUIElement(panel, "HistoryScroll",
                new Vector2(0, 0.12f), new Vector2(1, 0.88f), new Vector2(0.5f, 0.5f));
            scrollGO.GetComponent<RectTransform>().offsetMin = new Vector2(10, 0);
            scrollGO.GetComponent<RectTransform>().offsetMax = new Vector2(-10, 0);
            var scrollImg = scrollGO.AddComponent<Image>();
            scrollImg.color = new Color(0, 0, 0, 0.3f);
            var scroll = scrollGO.AddComponent<ScrollRect>();
            scroll.horizontal = false;

            var content = CreateUIElement(scrollGO, "Content",
                new Vector2(0, 1), new Vector2(1, 1), new Vector2(0.5f, 1));
            content.GetComponent<RectTransform>().sizeDelta = new Vector2(0, 0);
            var csf = content.AddComponent<ContentSizeFitter>();
            csf.verticalFit = ContentSizeFitter.FitMode.PreferredSize;
            var vlg = content.AddComponent<VerticalLayoutGroup>();
            vlg.childForceExpandWidth = true;
            vlg.childForceExpandHeight = false;
            vlg.spacing = 8;
            vlg.padding = new RectOffset(10, 10, 10, 10);
            scroll.content = content.GetComponent<RectTransform>();

            // 输入行
            var inputRow = CreateUIElement(panel, "InputRow",
                new Vector2(0, 0), new Vector2(1, 0.12f), new Vector2(0.5f, 0));
            var inputRowRT = inputRow.GetComponent<RectTransform>();
            inputRowRT.offsetMin = new Vector2(10, 5);
            inputRowRT.offsetMax = new Vector2(-10, -5);

            // 输入框
            var inputGO = CreateUIElement(inputRow, "InputField",
                new Vector2(0, 0), new Vector2(0.8f, 1), new Vector2(0, 0.5f));
            var inputBG = inputGO.AddComponent<Image>();
            inputBG.color = new Color(0.15f, 0.15f, 0.2f);
            var inputField = inputGO.AddComponent<TMP_InputField>();

            var inputText = CreateTextElement(inputGO, "Text", "", 18);
            inputText.GetComponent<RectTransform>().offsetMin = new Vector2(10, 0);
            inputField.textComponent = inputText.GetComponent<TextMeshProUGUI>();

            var placeholder = CreateTextElement(inputGO, "Placeholder", "说点什么...", 18);
            placeholder.GetComponent<TextMeshProUGUI>().color = new Color(1, 1, 1, 0.3f);
            placeholder.GetComponent<RectTransform>().offsetMin = new Vector2(10, 0);
            inputField.placeholder = placeholder.GetComponent<TextMeshProUGUI>();

            // 发送按钮
            var btnGO = CreateUIElement(inputRow, "SendButton",
                new Vector2(0.82f, 0), new Vector2(1, 1), new Vector2(1, 0.5f));
            var btnImg = btnGO.AddComponent<Image>();
            btnImg.color = new Color(0.2f, 0.6f, 0.3f);
            var btn = btnGO.AddComponent<Button>();
            btn.targetGraphic = btnImg;
            var btnColors = btn.colors;
            btnColors.highlightedColor = new Color(0.3f, 0.8f, 0.4f);
            btnColors.pressedColor = new Color(0.15f, 0.4f, 0.2f);
            btn.colors = btnColors;
            var btnText = CreateTextElement(btnGO, "Text", "发送", 18);
            btnText.GetComponent<TextMeshProUGUI>().color = Color.white;

            // 关闭按钮
            var closeBtnGO = CreateUIElement(panel, "CloseButton",
                new Vector2(1, 1), new Vector2(1, 1), new Vector2(1, 1));
            closeBtnGO.GetComponent<RectTransform>().anchoredPosition = new Vector2(-5, -5);
            closeBtnGO.GetComponent<RectTransform>().sizeDelta = new Vector2(35, 35);
            var closeBG = closeBtnGO.AddComponent<Image>();
            closeBG.color = new Color(0.8f, 0.2f, 0.2f);
            closeBtnGO.AddComponent<Button>().targetGraphic = closeBG;
            var closeText = CreateTextElement(closeBtnGO, "X", "✕", 20);
            closeText.GetComponent<TextMeshProUGUI>().color = Color.white;

            // OpenWorldDialogPanel 组件
            var dialogComp = panel.AddComponent<OpenWorldDialogPanel>();
            SetField(dialogComp, "panelRoot", panel);
            SetField(dialogComp, "npcNameText", nameText.GetComponent<TextMeshProUGUI>());
            SetField(dialogComp, "inputField", inputField);
            SetField(dialogComp, "sendButton", btn);
            SetField(dialogComp, "closeButton", closeBtnGO.GetComponent<Button>());
            SetField(dialogComp, "historyScroll", scroll);
            SetField(dialogComp, "historyContent", content.transform);
        }

        // ── UI 工具函数 ─────────────────────────────────────────────────
        static GameObject CreateUIElement(GameObject parent, string name,
            Vector2 anchorCenter)
        {
            return CreateUIElement(parent, name, anchorCenter, anchorCenter, new Vector2(0.5f, 0.5f));
        }

        static GameObject CreateUIElement(GameObject parent, string name,
            Vector2 anchorMin, Vector2 anchorMax, Vector2 pivot)
        {
            var go = new GameObject(name);
            go.layer = LayerMask.NameToLayer("UI");
            go.transform.SetParent(parent.transform, false);
            var rt = go.AddComponent<RectTransform>();
            rt.anchorMin = anchorMin;
            rt.anchorMax = anchorMax;
            rt.pivot = pivot;
            rt.offsetMin = Vector2.zero;
            rt.offsetMax = Vector2.zero;
            return go;
        }

        static GameObject CreateTextElement(GameObject parent, string name, string text, float fontSize)
        {
            var go = new GameObject(name);
            go.layer = LayerMask.NameToLayer("UI");
            go.transform.SetParent(parent.transform, false);
            var rt = go.AddComponent<RectTransform>();
            rt.anchorMin = Vector2.zero;
            rt.anchorMax = Vector2.one;
            rt.offsetMin = Vector2.zero;
            rt.offsetMax = Vector2.zero;
            var tmp = go.AddComponent<TextMeshProUGUI>();
            tmp.text = text;
            tmp.fontSize = fontSize;
            tmp.alignment = TextAlignmentOptions.Center;
            tmp.enableWordWrapping = true;
            return go;
        }

        // ── 反射设值（Editor 脚本用） ───────────────────────────────────
        static void SetField(object target, string fieldName, object value)
        {
            var type = target.GetType();
            while (type != null)
            {
                var field = type.GetField(fieldName,
                    System.Reflection.BindingFlags.Instance |
                    System.Reflection.BindingFlags.NonPublic |
                    System.Reflection.BindingFlags.Public);
                if (field != null)
                {
                    field.SetValue(target, value);
                    return;
                }
                type = type.BaseType;
            }
            Debug.LogWarning($"[DemoBuilder] Field '{fieldName}' not found on {target.GetType().Name}");
        }
    }
}
#endif
