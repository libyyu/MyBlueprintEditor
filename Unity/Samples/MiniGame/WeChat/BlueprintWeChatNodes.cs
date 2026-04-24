// BlueprintWeChatNodes.cs
// ─────────────────────────────────────────────────────────────────────────────
// 把微信小游戏 wx.* API 注册为自定义蓝图节点
//
// 使用：
//   1. 场景中任意 GameObject 挂本脚本
//   2. Inspector 里把目标 BPRunner 拖进来（通常是 BlueprintService 的某个 Runner）
//   3. 蓝图里就能用 "WeChat.Share" / "WeChat.Toast" / "WeChat.ShareForReward" 节点
//
// 节点列表：
//   WeChat.Share          触发微信分享给好友
//   WeChat.Toast          显示原生 Toast 提示
//   WeChat.ShareForReward 分享后玩家回到游戏给奖励（配合 AwardPending 逻辑）
//   WeChat.IsInWeChat     查询当前是否在微信小游戏（蓝图里做条件分支）
//
// 注意：这些节点只影响"已注册 Runner"，不是全局。每个 NPC 的 Runner
//      都可以独立注册（或不注册）这些节点。
//
// 扩展：如要添加新节点，参考本文件的模式复制粘贴即可。
// ─────────────────────────────────────────────────────────────────────────────

using UnityEngine;
using BlueprintRuntime;

namespace BlueprintRuntime.Samples.MiniGame.WeChat
{
    public class BlueprintWeChatNodes : MonoBehaviour
    {
        [Tooltip("要注册 WeChat 节点的 Runner 所在的 Controller。\n" +
                 "也可以手动调用 RegisterAll(runner) 注入到任意 Runner。")]
        [SerializeField] private MonoBehaviour targetController;  // AINpcController/Streaming/Emotional

        void Start()
        {
            if (targetController == null) return;
            var runner = ExtractRunner(targetController);
            if (runner != null) RegisterAll(runner);
        }

        /// <summary>
        /// 把所有 WeChat.* 节点注册到指定 runner。
        /// 可在任意时机调用（例如 OpenWorldDialogPanel 打开时为当前 NPC 注册）。
        /// </summary>
        public static void RegisterAll(BPRunner runner)
        {
            if (runner == null) return;

            // ── WeChat.Share ────────────────────────────────────────
            runner.RegisterNodeDef(new BPNodeDef {
                id       = "WeChat.Share",
                name     = "WeChat Share",
                category = "MiniGame/WeChat",
                color    = "07C160",
                pins = new []
                {
                    new BPPinDef { name = "",         dataType = BPPinType.Unknown, isInput = true,  isExec = true },
                    new BPPinDef { name = "Title",    dataType = BPPinType.String,  isInput = true  },
                    new BPPinDef { name = "ImageUrl", dataType = BPPinType.String,  isInput = true  },
                    new BPPinDef { name = "Query",    dataType = BPPinType.String,  isInput = true  },
                    new BPPinDef { name = "",         dataType = BPPinType.Unknown, isInput = false, isExec = true },
                },
            });
            runner.RegisterHandler("WeChat.Share", ctx => {
                WeChatSDK.Share(
                    ReadString(ctx, "Title"),
                    ReadString(ctx, "ImageUrl"),
                    ReadString(ctx, "Query"));
                ctx.ActivateOutputFlow("");
                return true;
            });

            // ── WeChat.Toast ────────────────────────────────────────
            runner.RegisterNodeDef(new BPNodeDef {
                id       = "WeChat.Toast",
                name     = "WeChat Toast",
                category = "MiniGame/WeChat",
                color    = "07C160",
                pins = new []
                {
                    new BPPinDef { name = "",         dataType = BPPinType.Unknown, isInput = true,  isExec = true },
                    new BPPinDef { name = "Text",     dataType = BPPinType.String,  isInput = true  },
                    new BPPinDef { name = "Icon",     dataType = BPPinType.String,  isInput = true  },  // none|success|error|loading
                    new BPPinDef { name = "Duration", dataType = BPPinType.Integer, isInput = true  },  // ms
                    new BPPinDef { name = "",         dataType = BPPinType.Unknown, isInput = false, isExec = true },
                },
            });
            runner.RegisterHandler("WeChat.Toast", ctx => {
                string icon = ReadString(ctx, "Icon");
                WeChatSDK.ToastIcon ti = icon switch {
                    "success" => WeChatSDK.ToastIcon.Success,
                    "error"   => WeChatSDK.ToastIcon.Error,
                    "loading" => WeChatSDK.ToastIcon.Loading,
                    _         => WeChatSDK.ToastIcon.None,
                };
                int dur = (int)ctx.GetInputInt("Duration");
                if (dur <= 0) dur = 1500;
                WeChatSDK.ShowToast(ReadString(ctx, "Text"), ti, dur);
                ctx.ActivateOutputFlow("");
                return true;
            });

            // ── WeChat.IsInWeChat ───────────────────────────────────
            // 纯数据节点（无 exec）
            runner.RegisterNodeDef(new BPNodeDef {
                id       = "WeChat.IsInWeChat",
                name     = "Is In WeChat",
                category = "MiniGame/WeChat",
                color    = "07C160",
                pins = new []
                {
                    new BPPinDef { name = "Value", dataType = BPPinType.Boolean, isInput = false },
                },
            });
            runner.RegisterHandler("WeChat.IsInWeChat", ctx => {
                ctx.SetOutputBool("Value", WeChatSDK.IsInWeChat);
                return true;
            });
        }

        // ── 工具 ───────────────────────────────────────────────────
        private static string ReadString(BPContext ctx, string pin)
        {
            return ctx.GetInputString(pin) ?? "";
        }

        /// <summary>通过反射从 Controller 拿到 BPRunner（和 NpcMemory 同样做法）</summary>
        private static BPRunner ExtractRunner(MonoBehaviour controller)
        {
            var t = controller.GetType();
            var f = t.GetField("_runner", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);
            return f?.GetValue(controller) as BPRunner;
        }
    }
}
