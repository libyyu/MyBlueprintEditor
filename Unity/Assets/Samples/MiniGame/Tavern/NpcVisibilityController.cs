// NpcVisibilityController.cs — 根据酒馆解锁状态控制 NPC 显示
// 挂到场景根物体上，Start 时扫描所有 NPC 并控制可见性

using UnityEngine;
using BlueprintRuntime.Samples.AINpc;

namespace BlueprintRuntime.Samples.MiniGame.Tavern
{
    public class NpcVisibilityController : MonoBehaviour
    {
        void Start()
        {
            Refresh();
            if (TavernManager.Instance != null)
                TavernManager.Instance.OnNpcUnlocked += _ => Refresh();
        }

        void Refresh()
        {
            var tm = TavernManager.Instance;
            if (tm == null) return;

            foreach (var ctrl in Resources.FindObjectsOfTypeAll<AINpcStreamingController>())
            {
                string npcId = ctrl.gameObject.name;
                bool unlocked = tm.IsNpcUnlocked(npcId);
                ctrl.gameObject.SetActive(unlocked);
            }
        }
    }
}
