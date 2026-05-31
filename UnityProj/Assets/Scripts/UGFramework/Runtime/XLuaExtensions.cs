// XLuaExtensions.cs
// xLua 扩展方法 —— 不修改 xLua 第三方源码，通过反射访问 private 字段实现。
//
// 提供：
//   LuaBaseExtensions.IsValid(this LuaBase)  ← C# extension method
//   GameUtil.IsLuaObjectValid(LuaBase)        ← [LuaCallCSharp] 暴露给 Lua
//
// 原理：
//   LuaBase.disposed    → protected，直接在 LuaBase 子类内可见，但 extension method 在外部，
//                         需要反射读取
//   LuaEnv.disposed     → private，通过反射读取
//   两个字段都缓存 FieldInfo，只反射一次，无性能问题。

using System.Reflection;
using XLua;

namespace UGFramework.Runtime
{
    /// <summary>
    /// C# extension method，供 C# 代码直接调用：luaObj.IsValid()
    /// </summary>
    public static class LuaBaseExtensions
    {
        // 缓存反射字段，只查一次
        static FieldInfo s_luaBaseDisposed;
        static FieldInfo s_luaEnvDisposed;
        static FieldInfo s_luaBaseEnv;

        static LuaBaseExtensions()
        {
            var baseFlags  = BindingFlags.Instance | BindingFlags.NonPublic;
            s_luaBaseDisposed = typeof(LuaBase).GetField("disposed", baseFlags);
            s_luaBaseEnv      = typeof(LuaBase).GetField("luaEnv",   baseFlags);
            s_luaEnvDisposed  = typeof(LuaEnv) .GetField("disposed", baseFlags);
        }

        /// <summary>
        /// 判断 LuaTable / LuaFunction 等 LuaBase 对象是否仍然有效。
        /// 等价于原来 LuaBase.IsValid() 的逻辑，但不修改 xLua 源码。
        /// </summary>
        public static bool IsValid(this LuaBase obj)
        {
            if (obj == null) return false;

            // 1. 检查 LuaBase.disposed（protected bool）
            if (s_luaBaseDisposed != null && (bool)s_luaBaseDisposed.GetValue(obj))
                return false;

            // 2. 检查 luaEnv 是否为 null
            var env = s_luaBaseEnv?.GetValue(obj) as LuaEnv;
            if (env == null) return false; // 没有 env 视为有效（兼容某些场景）

            // 3. 检查 LuaEnv.disposed（private bool）
            if (s_luaEnvDisposed != null && (bool)s_luaEnvDisposed.GetValue(env))
                return false;

            return true;
        }
    }
}
