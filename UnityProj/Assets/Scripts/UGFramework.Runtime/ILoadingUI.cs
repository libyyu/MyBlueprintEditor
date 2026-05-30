// ILoadingUI.cs
// Loading UI 统一接口
//
// BootstrapLoadingUI（极简内置）和正式 LoadingUI prefab 都实现此接口，
// GameLauncher 通过 ActiveLoadingUI 属性统一调用，无需关心当前是哪个实现。
//
// 正式 LoadingUI prefab 上挂的脚本需实现此接口（见 LoadingUIController.cs 模板）。

namespace UGFramework.Runtime
{
    public interface ILoadingUI
    {
        /// <summary>显示 Loading UI，可传入初始进度避免从 0 闪一帧</summary>
        void Show(float initialProgress = 0f, string initialLabel = null);

        /// <summary>更新进度（0~1）和提示文字</summary>
        void SetProgress(float t, string label = null);

        /// <summary>显示错误面板，onRetry 为重试回调</summary>
        void ShowError(string message, System.Action onRetry);

        /// <summary>隐藏并销毁（下一阶段接管或游戏开始后调用）</summary>
        void Hide();
    }
}
