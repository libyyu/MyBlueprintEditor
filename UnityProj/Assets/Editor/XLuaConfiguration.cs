using System;
using System.Collections.Generic;
using UnityEngine;
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
}