// ─────────────────────────────────────────────────────────────────────
// NpcConversationDemo.cs — NPC 互相对话演示场景
//
// 放到场景中的一个空 GameObject 上。
// 它会找到所有 NpcGossip 组件，定期让两个 NPC 自发交谈。
// 玩家可以旁观，也可以随时插入。
//
// 演示效果：
//   NPC_A 头顶气泡："铁匠，你听说了吗？有个冒险者送喵喵礼物了！"
//   NPC_B 头顶气泡："啊，那个人？我看见了，出手挺大方的。"
//   NPC_A 头顶气泡："真羡慕喵喵，我这铺子都没人光顾..."
//   (两个 NPC 面对面，说话时轮流播放气泡)
// ─────────────────────────────────────────────────────────────────────

using BlueprintRuntime.Samples.AINpc;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame
{
    public class NpcConversationDemo : MonoBehaviour
    {
        [Header("配置")]
        [Tooltip("两个 NPC 之间的最远对话距离")]
        [SerializeField] private float conversationRange = 15f;

        [Tooltip("NPC 自发聊天的间隔（秒）")]
        [SerializeField] private float gossipInterval = 30f;

        [Tooltip("最大同时进行的 NPC 对话数")]
        [SerializeField] private int maxConcurrent = 1;

        [Header("视觉")]
        [Tooltip("NPC 对话时互相面向对方")]
        [SerializeField] private bool faceEachOther = true;

        [Tooltip("对话轮数（A→B→A→B 算 4 轮）")]
        [SerializeField] private int maxRounds = 4;

        [Tooltip("每轮之间的间隔")]
        [SerializeField] private float roundDelay = 2.5f;

        // ── 内部 ────────────────────────────────────────────────────
        private NpcGossip[] _allGossips;
        private float _timer;
        private int _activeConversations;

        void Start()
        {
            _allGossips = FindObjectsOfType<NpcGossip>();
            _timer = gossipInterval * 0.5f;  // 首次聊天快一点
        }

        void Update()
        {
            if (_activeConversations >= maxConcurrent) return;

            _timer -= Time.deltaTime;
            if (_timer > 0f) return;
            _timer = gossipInterval;

            TryStartConversation();
        }

        private void TryStartConversation()
        {
            if (_allGossips == null || _allGossips.Length < 2) return;

            // 随机选一个发起者
            var candidates = new List<NpcGossip>(_allGossips);
            Shuffle(candidates);

            foreach (var initiator in candidates)
            {
                if (!initiator.enabled || !initiator.gameObject.activeInHierarchy) continue;

                // 找最近的对话伙伴
                NpcGossip partner = FindNearestPartner(initiator, candidates);
                if (partner == null) continue;

                StartCoroutine(RunConversation(initiator, partner));
                return;
            }
        }

        private NpcGossip FindNearestPartner(NpcGossip initiator, List<NpcGossip> all)
        {
            NpcGossip best = null;
            float bestDist = conversationRange;

            foreach (var g in all)
            {
                if (g == initiator) continue;
                if (!g.enabled || !g.gameObject.activeInHierarchy) continue;

                float d = Vector3.Distance(initiator.transform.position, g.transform.position);
                if (d < bestDist)
                {
                    bestDist = d;
                    best = g;
                }
            }

            return best;
        }

        private IEnumerator RunConversation(NpcGossip npcA, NpcGossip npcB)
        {
            _activeConversations++;
            Debug.Log($"[NpcConversation] {npcA.name} 和 {npcB.name} 开始聊天");

            // 面向对方
            if (faceEachOther)
            {
                FaceTarget(npcA.transform, npcB.transform.position);
                FaceTarget(npcB.transform, npcA.transform.position);
            }

            // 获取世界事件作为话题
            string topic = "";
            if (WorldState.Instance != null)
            {
                var events = WorldState.Instance.Data.recentEvents;
                if (events != null && events.Count > 0)
                    topic = events[Random.Range(0, events.Count)];
            }

            // 多轮对话
            NpcGossip speaker = npcA;
            NpcGossip listener = npcB;
            string lastSaid = "";

            for (int round = 0; round < maxRounds; round++)
            {
                // 构造输入
                string input;
                if (round == 0)
                {
                    input = string.IsNullOrEmpty(topic)
                        ? $"（你主动找{listener.name}聊天，随便聊点什么）"
                        : $"（你听说了一件事：{topic}，找{listener.name}聊聊）";
                }
                else
                {
                    input = $"（{listener.name}刚才对你说：\"{lastSaid}\"，你怎么回应？）";
                }

                // 让 speaker 说话
                string reply = "";
                bool done = false;

                // 通过 Controller 触发（用 OnReply 收集回复）
                var ctrl = speaker.GetComponent<AINpcController>()
                        ?? speaker.GetComponent<AINpcStreamingController>() as MonoBehaviour
                        ?? speaker.GetComponent<EmotionalNpcController>() as MonoBehaviour;

                if (ctrl == null) break;

                System.Action<string> captureReply = (msg) => { reply = msg; done = true; };

                var basic = ctrl as AINpcController;
                var stream = ctrl as AINpcStreamingController;
                var emo = ctrl as EmotionalNpcController;

                if (basic != null)       basic.OnReply += captureReply;
                else if (stream != null) stream.OnReplyDone += captureReply;
                else if (emo != null)    emo.OnReply += captureReply;

                // Say
                if (basic != null) basic.Say(input);
                else if (stream != null) stream.Say(input);

                // 等回复（最多 15 秒）
                float wait = 0f;
                while (!done && wait < 15f)
                {
                    wait += Time.deltaTime;
                    yield return null;
                }

                // 取消订阅
                if (basic != null)       basic.OnReply -= captureReply;
                else if (stream != null) stream.OnReplyDone -= captureReply;
                else if (emo != null)    emo.OnReply -= captureReply;

                if (string.IsNullOrEmpty(reply))
                {
                    Debug.LogWarning($"[NpcConversation] {speaker.name} 没有回复，结束对话");
                    break;
                }

                lastSaid = reply;

                // 交换角色
                (speaker, listener) = (listener, speaker);

                yield return new WaitForSeconds(roundDelay);
            }

            Debug.Log($"[NpcConversation] {npcA.name} 和 {npcB.name} 聊天结束");
            _activeConversations--;
        }

        private static void FaceTarget(Transform self, Vector3 target)
        {
            var dir = target - self.position;
            dir.y = 0f;
            if (dir.sqrMagnitude > 0.01f)
                self.rotation = Quaternion.LookRotation(dir);
        }

        private static void Shuffle<T>(List<T> list)
        {
            for (int i = list.Count - 1; i > 0; i--)
            {
                int j = Random.Range(0, i + 1);
                (list[i], list[j]) = (list[j], list[i]);
            }
        }
    }
}
