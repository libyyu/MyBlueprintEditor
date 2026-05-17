using System;
using System.Collections.Generic;
using UnityEngine;
using XLua;

public static class XLuaConfig
{
    [CSharpCallLua]
    public static List<Type> CSharpCallLua = new List<Type>()
    {
        typeof(Action<bool, string>),
        typeof(Action<bool, string, string>),
        typeof(Action<int, int, long, long>),
        typeof(Action<bool, UnityEngine.Object, string>),
        typeof(Action<bool, GameObject, string>),
        typeof(Action<int, int>),
        typeof(Action<bool, string[], bool[], string>),
        typeof(FTimerList.TimerCallback)
    };
}