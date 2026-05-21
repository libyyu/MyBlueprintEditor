// VisualEnhancerSetup.cs
// 编辑器工具：一键为场景里的绳子和场景添加视觉增强组件
// 菜单：Tools > CutRope > ✨ Enhance Visuals

using UnityEngine;
using UnityEditor;

namespace CutRope.Editor
{
    public static class VisualEnhancerSetup
    {
        [MenuItem("Tools/CutRope/✨ Enhance Visuals (视觉增强)", priority = 5)]
        public static void EnhanceVisuals()
        {
            int count = 0;

            // 1. 给所有 RopeSpawner 添加 RopeVisualSetup（如果还没有）
            var spawners = Object.FindObjectsByType<CutRope.Game.RopeSpawner>(FindObjectsSortMode.None);
            foreach (var spawner in spawners)
            {
                if (spawner.GetComponent<CutRope.Game.RopeVisualSetup>() == null)
                {
                    spawner.gameObject.AddComponent<CutRope.Game.RopeVisualSetup>();
                    count++;
                    Debug.Log($"[VisualEnhancer] Added RopeVisualSetup to {spawner.gameObject.name}");
                }

                // 确保有 LineRenderer
                if (spawner.GetComponent<LineRenderer>() == null)
                    spawner.gameObject.AddComponent<LineRenderer>();
            }

            // 2. 在场景里添加 GameVisualEnhancer（如果还没有）
            var enhancer = Object.FindFirstObjectByType<CutRope.Game.GameVisualEnhancer>();
            if (enhancer == null)
            {
                // 找到 LevelController，在同一个 GameObject 上添加
                var lc = Object.FindFirstObjectByType<CutRope.Game.LevelController>();
                if (lc != null)
                {
                    lc.gameObject.AddComponent<CutRope.Game.GameVisualEnhancer>();
                    Debug.Log($"[VisualEnhancer] Added GameVisualEnhancer to {lc.gameObject.name}");
                }
                else
                {
                    // 新建一个空对象
                    var go = new GameObject("GameVisualEnhancer");
                    go.AddComponent<CutRope.Game.GameVisualEnhancer>();
                    Debug.Log("[VisualEnhancer] Added GameVisualEnhancer to new GameObject");
                }
                count++;
            }

            // 3. 改善摄像机背景
            var cam = Camera.main;
            if (cam != null)
            {
                cam.backgroundColor = new Color(0.08f, 0.10f, 0.20f);

                // 添加 CameraShaker（如果还没有）
                if (cam.GetComponent<CutRope.Game.CameraShaker>() == null)
                {
                    cam.gameObject.AddComponent<CutRope.Game.CameraShaker>();
                    Debug.Log("[VisualEnhancer] Added CameraShaker to Main Camera");
                    count++;
                }
            }

            UnityEditor.SceneManagement.EditorSceneManager.MarkAllScenesDirty();

            EditorUtility.DisplayDialog("视觉增强完成",
                $"已处理 {spawners.Length} 条绳子，添加/更新了 {count} 个视觉增强组件。\n\n" +
                "• RopeSpawner → RopeVisualSetup + RopeRenderer（贝塞尔曲线平滑绳子）\n" +
                "• GameVisualEnhancer（渐变背景 + 切割特效 + 糖果美化）\n" +
                "• CameraShaker（震屏效果）\n\n" +
                "请保存场景。",
                "OK");
        }

        [MenuItem("Tools/CutRope/🎨 Improve Rope Style", priority = 6)]
        public static void ImproveRopeStyle()
        {
            var spawners = Object.FindObjectsByType<CutRope.Game.RopeSpawner>(FindObjectsSortMode.None);
            if (spawners.Length == 0)
            {
                EditorUtility.DisplayDialog("未找到绳子", "场景中没有 RopeSpawner。", "OK");
                return;
            }

            foreach (var spawner in spawners)
            {
                // 更新 RopeRenderer 参数（如果已存在）
                var rr = spawner.GetComponent<CutRope.Game.RopeRenderer>();
                if (rr != null)
                {
                    rr.interpolationSteps = 8;   // 更平滑
                    rr.tension            = 0.5f;
                    rr.widthTop           = 0.12f;
                    rr.widthBottom        = 0.07f;
                    rr.colorTop           = new Color(0.50f, 0.28f, 0.06f);
                    rr.colorMid           = new Color(0.85f, 0.55f, 0.18f);
                    rr.colorBottom        = new Color(0.60f, 0.33f, 0.08f);
                    rr.showAnchorPin      = true;
                    rr.pinRadius          = 0.13f;
                    rr.pinColor           = new Color(0.75f, 0.75f, 0.80f);
                    EditorUtility.SetDirty(rr);
                    Debug.Log($"[VisualEnhancer] Updated RopeRenderer on {spawner.gameObject.name}");
                }
                else
                {
                    Debug.LogWarning($"[VisualEnhancer] {spawner.gameObject.name} has no RopeRenderer. Run Enhance Visuals first.");
                }
            }

            UnityEditor.SceneManagement.EditorSceneManager.MarkAllScenesDirty();
            Debug.Log("[VisualEnhancer] Rope style updated.");
        }
    }
}
