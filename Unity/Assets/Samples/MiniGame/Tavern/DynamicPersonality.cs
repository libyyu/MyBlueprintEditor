// DynamicPersonality.cs — 动态人设注入
// 挂到每个 NPC 上，每次 Say() 之前自动更新 Personality + 状态变量

using UnityEngine;
using BlueprintRuntime.Samples.AINpc;

namespace BlueprintRuntime.Samples.MiniGame.Tavern
{
    [RequireComponent(typeof(AINpcStreamingController))]
    public class DynamicPersonality : MonoBehaviour
    {
        private AINpcStreamingController _ctrl;
        private string _npcId;

        void Start()
        {
            _ctrl = GetComponent<AINpcStreamingController>();
            _npcId = gameObject.name;

            // 订阅 OnThinking 事件——在 LLM 请求发出前更新 Personality
            _ctrl.OnThinking += UpdatePersonality;
        }

        void UpdatePersonality()
        {
            int affinity = 50;
            var memory = GetComponent<Storage.NpcMemory>();
            if (memory != null)
                affinity = memory.Data.affinity;

            int tavernLevel = TavernManager.Instance?.Data.level ?? 1;
            int dayCount = TavernManager.Instance?.Data.dayCount ?? 1;

            // 生成动态 Prompt
            string prompt = NpcPersonalities.BuildPrompt(_npcId, affinity, tavernLevel, dayCount);

            // 注入到蓝图变量（会在 Execute 时被 LLM.Chat 读取）
            // 注意：这里不能用 SetField 改 _ctrl.personality（它是 SerializeField）
            // 而是直接设蓝图变量 "Personality"
            var svc = AINpc.BlueprintService.Instance;
            if (svc == null) return;

            // 通过反射拿到 _ctrl 的 _runner
            var field = typeof(AINpcStreamingController).GetField("_runner",
                System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);
            if (field == null) return;

            var runner = field.GetValue(_ctrl) as BPRunner;
            if (runner == null) return;

            runner.SetVariable("Personality", prompt);

            // 同时注入酒馆状态
            TavernManager.Instance?.InjectInto(runner);

            // 注入好感度
            runner.SetVariable("Affinity", affinity.ToString());
            runner.SetVariable("MeetCount", (memory?.Data.meetCount ?? 0).ToString());
        }
    }
}
