using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;
using XLua;
using static CutRope.Framework.SceneLoader;
using static YooAssetsLuaBridge;

public static class XLuaConfig
{
    [CSharpCallLua]
    public static List<Type> CSharpCallLua = new List<Type>()
    {

        //SceneLoader
        typeof(LuaProgressCallback),
        typeof(LuaCompleteCallback),
        //YooAssetsLuaBridge
        typeof(YooAssetsLuaBridge.LuaBoolStringCallback),
        typeof(YooAssetsLuaBridge.LuaBoolStringStringCallback),
        typeof(YooAssetsLuaBridge.LuaResourceDownloadProgressCallback),
        typeof(YooAssetsLuaBridge.LuaLoadAssetCallback),
        typeof(YooAssetsLuaBridge.LuaInstantiateCallback),
        typeof(YooAssetsLuaBridge.LuaLoadRawFileCallback),
        typeof(YooAssetsLuaBridge.LuaLoadAllLuaFilesProgressCallback),
        typeof(YooAssetsLuaBridge.LuaLoadAllLuaFilesCompleteCallback),
        //FTimerList
        typeof(FTimerList.TimerCallback)
    };

    [LuaCallCSharp]
    public static List<Type> LuaCallCSharp_Extra = new List<Type>()
    {
        typeof(CutRope.Game.RopeRenderer),
        typeof(CutRope.Game.RopeVisualSetup),
        typeof(CutRope.Game.GameVisualEnhancer),
    };

    // ── UI Toolkit + 双后端绑定 ────────────────────────────────────────
    // Lua 侧通过 IUIPanelBackend 接口、UITKLuaBridge、UITKPanelBackend
    // 操作 UI Toolkit 元素，需要在此注册供 xLua 代码生成
    [LuaCallCSharp]
    public static List<Type> LuaCallCSharp_UITK = new List<Type>()
    {
        // 后端接口与实现
        typeof(CutRope.Framework.IUIPanelBackend),
        typeof(CutRope.Framework.UGUIPanelBackend),
        typeof(CutRope.Framework.UITKPanelBackend),

        // UITK 事件桥（等价 UILuaBehaviour）
        typeof(CutRope.Framework.UITKLuaBridge),

        // UnityEngine.UIElements 核心类型
        // Lua 可通过 bridge:Q() 拿到 VisualElement 并操作属性
        typeof(VisualElement),
        typeof(VisualElementExtensions),  // Q<T>() 扩展方法所在类
        typeof(UQueryExtensions),          // Query / ToList
        typeof(UIDocument),
        typeof(PanelSettings),
        typeof(Button),
        typeof(Label),
        typeof(TextField),
        typeof(Toggle),
        typeof(Slider),
        typeof(ScrollView),
        typeof(ProgressBar),
        typeof(DropdownField),
        typeof(StyleEnum<DisplayStyle>),
        typeof(StyleEnum<Visibility>),
        typeof(DisplayStyle),
        typeof(Visibility),
    };
}