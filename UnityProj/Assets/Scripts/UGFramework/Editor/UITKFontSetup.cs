// UITKFontSetup.cs
// 一键为 UI Toolkit 配置中文字体，解决"打包后 UITK 文字不显示 / 字体拖不进 Default Font Asset"。
//
// 菜单：
//   Tools > UITK Font > Create Dynamic FontAsset From Selected TTF
//       选中一个 .ttf/.otf → 生成 TextCore(UI Toolkit) 的 Dynamic FontAsset，
//       放到 Assets/UI Toolkit/Resources/Fonts & Materials/ 下。
//   Tools > UITK Font > Assign To Panel Text Settings...
//       把一个 FontAsset 赋给指定的 PanelTextSettings 的 Default Font Asset（并可加入 Fallback）。
//   Tools > UITK Font > One-Click Setup (TTF -> FontAsset -> PanelTextSettings)
//       上面两步合一：选中 .ttf 后一键生成并赋给项目里所有 PanelTextSettings。
//
// 背景（为什么会出问题）：
//   - PanelTextSettings.Default Font Asset 只接受 UnityEngine.TextCore.Text.FontAsset，
//     而不是 TMPro.TMP_FontAsset（TMP 的 SDF 资产）→ 类型不符拖不进去。
//   - 该字段的选择器只列出位于 PanelTextSettings 配置的 Resources 路径下的字体资产。
//   - 中文字符集庞大，必须用 Dynamic 模式（运行时按需取字模），且 .ttf 需勾 Include Font Data
//     才会被打进包，否则打包后无字模 → 文字不显示。

using System.IO;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.TextCore.Text;     // FontAsset (UI Toolkit / TextCore)
using UnityEngine.UIElements;        // PanelTextSettings (UI Toolkit)
using UnityEditor;

namespace UGFramework.Editor
{
    public static class UITKFontSetup
    {
        // PanelTextSettings 默认的字体资产存放路径（必须是 Resources 子目录）
        private const string FontAssetDir = "Assets/UI Toolkit/Resources/Fonts & Materials";

        // ── 菜单 1：从选中的 TTF 生成 Dynamic FontAsset ──────────────────────
        [MenuItem("Tools/UITK Font/① Create Dynamic FontAsset From Selected TTF", priority = 0)]
        public static void CreateFromSelectedTTF()
        {
            var font = GetSelectedFont();
            if (font == null)
            {
                EditorUtility.DisplayDialog("UITK Font", "请先在 Project 里选中一个 .ttf / .otf 字体文件。", "OK");
                return;
            }
            var asset = CreateDynamicFontAsset(font);
            if (asset != null)
            {
                Selection.activeObject = asset;
                EditorGUIUtility.PingObject(asset);
                EditorUtility.DisplayDialog("UITK Font",
                    "已创建 Dynamic FontAsset：\n" + AssetDatabase.GetAssetPath(asset), "OK");
            }
        }

        // ── 菜单 2：一键全流程 ───────────────────────────────────────────────
        [MenuItem("Tools/UITK Font/② One-Click Setup (TTF → FontAsset → PanelTextSettings)", priority = 1)]
        public static void OneClickSetup()
        {
            var font = GetSelectedFont();
            if (font == null)
            {
                EditorUtility.DisplayDialog("UITK Font", "请先在 Project 里选中一个 .ttf / .otf 字体文件。", "OK");
                return;
            }

            // 1) 确保 .ttf 设为 Dynamic + Include Font Data（Dynamic 打包必须）
            EnsureFontImporterDynamic(font);

            // 2) 生成 Dynamic FontAsset
            var asset = CreateDynamicFontAsset(font);
            if (asset == null) return;

            // 3) 赋给项目里所有 PanelTextSettings 的 Default Font Asset（并加入 Fallback）
            int n = AssignToAllPanelTextSettings(asset);

            AssetDatabase.SaveAssets();
            EditorUtility.DisplayDialog("UITK Font",
                string.Format("完成！\n\nFontAsset: {0}\n已赋给 {1} 个 PanelTextSettings 的 Default Font Asset。\n\n" +
                              "若仍不显示，请检查用到的 PanelSettings 的 Text Settings 字段是否指向该 PanelTextSettings。",
                    AssetDatabase.GetAssetPath(asset), n), "OK");
        }

        // ── 核心：创建 Dynamic TextCore FontAsset ───────────────────────────
        private static FontAsset CreateDynamicFontAsset(Font sourceFont)
        {
            EnsureDir(FontAssetDir);

            string srcPath = AssetDatabase.GetAssetPath(sourceFont);
            string baseName = Path.GetFileNameWithoutExtension(srcPath);
            string outPath = AssetDatabase.GenerateUniqueAssetPath(
                FontAssetDir + "/" + baseName + " SDF.asset");

            // Dynamic + SDFAA：中文按需生成字模，源字体随包嵌入（配合 Include Font Data）。
            // 512 图集起步，开启 Multi Atlas 以容纳大量 CJK 字形。
            FontAsset fa = FontAsset.CreateFontAsset(
                sourceFont,
                90,                                  // sampling point size
                9,                                   // atlas padding
                UnityEngine.TextCore.LowLevel.GlyphRenderMode.SDFAA,
                1024, 1024,                          // atlas 宽/高
                AtlasPopulationMode.Dynamic,
                true);                               // enableMultiAtlasSupport

            if (fa == null)
            {
                Debug.LogError("[UITKFontSetup] FontAsset.CreateFontAsset 返回 null：" + srcPath);
                return null;
            }
            fa.name = baseName + " SDF";

            AssetDatabase.CreateAsset(fa, outPath);

            // FontAsset 内含 atlas 纹理与 material 两个子对象，需一并写入资产文件
            if (fa.atlasTextures != null)
            {
                foreach (var tex in fa.atlasTextures)
                {
                    if (tex != null && !AssetDatabase.Contains(tex))
                    {
                        tex.name = fa.name + " Atlas";
                        AssetDatabase.AddObjectToAsset(tex, fa);
                    }
                }
            }
            if (fa.material != null && !AssetDatabase.Contains(fa.material))
            {
                fa.material.name = fa.name + " Material";
                AssetDatabase.AddObjectToAsset(fa.material, fa);
            }

            EditorUtility.SetDirty(fa);
            AssetDatabase.SaveAssets();
            AssetDatabase.ImportAsset(outPath);
            Debug.Log("[UITKFontSetup] Created Dynamic FontAsset: " + outPath);
            return AssetDatabase.LoadAssetAtPath<FontAsset>(outPath);
        }

        // ── 赋给所有 PanelTextSettings ──────────────────────────────────────
        private static int AssignToAllPanelTextSettings(FontAsset fontAsset)
        {
            int count = 0;
            string[] guids = AssetDatabase.FindAssets("t:PanelTextSettings");
            foreach (var guid in guids)
            {
                string path = AssetDatabase.GUIDToAssetPath(guid);
                var pts = AssetDatabase.LoadAssetAtPath<PanelTextSettings>(path);
                if (pts == null) continue;

                var so = new SerializedObject(pts);

                // Default Font Asset
                var defProp = so.FindProperty("m_DefaultFontAsset");
                if (defProp != null)
                {
                    defProp.objectReferenceValue = fontAsset;
                }

                // Fallback 列表追加（去重）
                var fbProp = so.FindProperty("m_FallbackFontAssets");
                if (fbProp != null && fbProp.isArray)
                {
                    bool exists = false;
                    for (int i = 0; i < fbProp.arraySize; i++)
                    {
                        if (fbProp.GetArrayElementAtIndex(i).objectReferenceValue == fontAsset)
                        {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists)
                    {
                        fbProp.arraySize += 1;
                        fbProp.GetArrayElementAtIndex(fbProp.arraySize - 1).objectReferenceValue = fontAsset;
                    }
                }

                so.ApplyModifiedPropertiesWithoutUndo();
                EditorUtility.SetDirty(pts);
                count++;
                Debug.Log("[UITKFontSetup] Assigned font to PanelTextSettings: " + path);
            }
            if (count == 0)
                Debug.LogWarning("[UITKFontSetup] 项目里没有找到 PanelTextSettings 资产。");
            return count;
        }

        // ── 辅助 ─────────────────────────────────────────────────────────
        private static Font GetSelectedFont()
        {
            return Selection.activeObject as Font;
        }

        private static void EnsureFontImporterDynamic(Font font)
        {
            string path = AssetDatabase.GetAssetPath(font);
            var importer = AssetImporter.GetAtPath(path) as TrueTypeFontImporter;
            if (importer == null) return;

            bool dirty = false;
            if (importer.fontTextureCase != FontTextureCase.Dynamic)
            {
                importer.fontTextureCase = FontTextureCase.Dynamic; // = Character: Dynamic
                dirty = true;
            }
            // Include Font Data：Dynamic 打包必须包含源字体二进制
            if (!importer.includeFontData)
            {
                importer.includeFontData = true;
                dirty = true;
            }
            if (dirty)
            {
                importer.SaveAndReimport();
                Debug.Log("[UITKFontSetup] TTF importer set to Dynamic + IncludeFontData: " + path);
            }
        }

        private static void EnsureDir(string dir)
        {
            if (AssetDatabase.IsValidFolder(dir)) return;
            string[] parts = dir.Split('/');
            string cur = parts[0]; // "Assets"
            for (int i = 1; i < parts.Length; i++)
            {
                string next = cur + "/" + parts[i];
                if (!AssetDatabase.IsValidFolder(next))
                    AssetDatabase.CreateFolder(cur, parts[i]);
                cur = next;
            }
        }
    }
}
