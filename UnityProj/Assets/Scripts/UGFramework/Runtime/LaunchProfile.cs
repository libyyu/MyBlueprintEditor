// LaunchProfile.cs
// 启动配置（数据驱动）——把"游戏 / 桌宠 等不同形态"的启动差异抽成纯数据，
// 使 GameLauncher 对具体形态完全无感知，彻底消除 if (isPetMode) 之类分支。
//
// 用法：
//   1. 右键 Create/UGFramework/Launch Profile 生成一个 .asset
//   2. 在场景的 GameLauncher.profile 字段指向它
//   游戏场景 → Game.asset (skipUpdate=false, gameLuaEntry="GameLogic")
//   桌宠场景 → Pet.asset  (skipUpdate=true,  gameLuaEntry="pet.PetLogic")

using UnityEngine;
using UnityEngine.UIElements;

namespace UGFramework.Runtime
{
    [CreateAssetMenu(fileName = "LaunchProfile", menuName = "UGFramework/Launch Profile", order = 0)]
    public class LaunchProfile : ScriptableObject
    {
        [Header("资源包")]
        public string packageName = "DefaultPackage";

        [Header("更新阶段")]
        [Tooltip("是否跳过更新阶段（true=不执行 UpdateLogic）。桌宠=true。")]
        public bool skipUpdate = false;

        [Tooltip("更新阶段 Lua 入口（require 路径，无 .lua 后缀）")]
        public string updateLuaEntry = "UpdateLogic";

        [Header("游戏/主逻辑阶段")]
        [Tooltip("主逻辑 Lua 入口（require 路径，无 .lua 后缀）")]
        public string gameLuaEntry = "GameLogic";

        [Header("UI Toolkit PanelSettings")]
        [Tooltip("常规面板（HUD / 菜单 / 桌宠气泡等），sortingOrder 10~50000")]
        public PanelSettings uitkGameSettings;

        [Tooltip("全局遮罩 / 系统弹窗，sortingOrder 90000+")]
        public PanelSettings uitkOverlaySettings;

        /// <summary>按 sortingOrder 返回对应 PanelSettings（>=90000 用 overlay，其余用 game）。</summary>
        public PanelSettings GetPanelSettingsForOrder(int sortingOrder)
        {
            var target = sortingOrder >= 90000 ? uitkOverlaySettings : uitkGameSettings;
            return target != null ? target : (uitkGameSettings ?? uitkOverlaySettings);
        }
    }
}
