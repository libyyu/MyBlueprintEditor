// GameBootstrap.cs
// Game 层初始化入口 — 负责向 Framework 层注入 Game 层实现
//
// 职责：
//   - 注册 UIManager.ViewFactory（让 Framework 能创建 UIController 而不产生反向依赖）
//
// 挂载：GameLauncher GameObject（与 GameLauncher 同节点）
// ExecutionOrder：-99（紧跟 GameLauncher 的 -100 之后执行）

using UnityEngine;
using CutRope.Framework;
using CutRope.Game.UI;

namespace CutRope.Game
{
    [DefaultExecutionOrder(-99)]
    public class GameBootstrap : MonoBehaviour
    {
        private void Awake()
        {
            // 向 Framework 注入默认 IView 工厂
            // 当 Prefab 上没有挂 IView 实现时，自动添加通用 UIController 组件
            UIManager.RegisterViewFactory(go => go.AddComponent<UIController>());

            Debug.Log("[GameBootstrap] UIManager.ViewFactory registered.");
        }
    }
}
