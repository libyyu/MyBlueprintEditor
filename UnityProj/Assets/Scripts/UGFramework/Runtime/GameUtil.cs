using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using UnityEngine;
using UnityEngine.Networking;
using UnityEngine.SceneManagement;
using XLua;
using XLua.LuaDLL;

namespace UGFramework.Runtime
{
    [LuaCallCSharp]
    public static class GameUtil
    {
        public static string PckPath => Application.dataPath + "/../pck";

        public static bool IsEditorEnv()
        {
#if UNITY_EDITOR
            return true;
#else
            return false;
#endif
        }

        public static bool IsWXEnv()
        {
#if UNITY_WEBGL && !UNITY_EDITOR
            return WebCommon.isRunEnvWX();
#else
            return false;
#endif
        }

        public static bool IsWebGLEnv()
        {
#if UNITY_WEBGL
            return true;
#else
            return false;
#endif
        }

        public static void CreateDirectory(string dir)
        {
            if (!Directory.Exists(dir))
            {
                Directory.CreateDirectory(dir);
            }
        }
        public static void CreateDirectoryForFile(string filepath)
        {
            Directory.CreateDirectory(System.IO.Path.GetDirectoryName(filepath));
        }
        public static bool IsDirectoryExist(string dir)
        {
            return Directory.Exists(dir);
        }

        public static string ToHexString(byte[] bytes, string sep = ",")
        {
            string byteStr = string.Empty;
            if (bytes != null && bytes.Length > 0)
            {
                int nPos = 0;
                foreach (var item in bytes)
                {
                    nPos++;
                    byteStr += string.Format("{0:X2}", item);
                    if (nPos < bytes.Length)
                        byteStr += sep;
                }
            }
            return byteStr;
        }

        public static string ToBytesString(byte[] bytes, string sep = ",")
        {
            string byteStr = string.Empty;
            if (bytes != null && bytes.Length > 0)
            {
                int nPos = 0;
                foreach (var item in bytes)
                {
                    nPos++;
                    byteStr += item.ToString();
                    if (nPos < bytes.Length)
                        byteStr += sep;
                }
            }
            return byteStr;
        }

        public static bool IsPointerOverUIObject()
        {
            UnityEngine.EventSystems.PointerEventData eventDataCurrentPosition = new UnityEngine.EventSystems.PointerEventData(UnityEngine.EventSystems.EventSystem.current);
            eventDataCurrentPosition.position = new Vector2(Input.mousePosition.x, Input.mousePosition.y);

            List<UnityEngine.EventSystems.RaycastResult> results = new List<UnityEngine.EventSystems.RaycastResult>();
            UnityEngine.EventSystems.EventSystem.current.RaycastAll(eventDataCurrentPosition, results);
            return results.Count > 0;
        }

        /// <summary>
        /// Cast a ray to test if screenPosition is over any UI object in canvas. This is a replacement
        /// for IsPointerOverGameObject() which does not work on Android in 4.6.0f3
        /// </summary>
        public static bool IsPointerOverUIObject(Canvas canvas, Vector2 screenPosition)
        {
            UnityEngine.EventSystems.PointerEventData eventDataCurrentPosition = new UnityEngine.EventSystems.PointerEventData(UnityEngine.EventSystems.EventSystem.current);
            eventDataCurrentPosition.position = screenPosition;

            UnityEngine.UI.GraphicRaycaster uiRaycaster = canvas.gameObject.GetComponent<UnityEngine.UI.GraphicRaycaster>();
            List<UnityEngine.EventSystems.RaycastResult> results = new List<UnityEngine.EventSystems.RaycastResult>();
            uiRaycaster.Raycast(eventDataCurrentPosition, results);
            return results.Count > 0;
        }
        
        /// <summary>
        /// 确保类型已注册到 xLua。
        /// 若类型尚未被 xLua wrap，主动触发 TryDelayWrapLoader 注册。
        /// </summary>
        private static bool EnsureTypeRegistered(System.IntPtr L, ObjectTranslator translator, string typeName)
        {
            Lua.luaL_getmetatable(L, typeName);
            bool exists = !Lua.lua_isnil(L, -1);
            Lua.lua_pop(L, 1);
            if (exists) return true;

            var translatorType = translator.GetType();
            var findTypeMethod = translatorType.GetMethod(
                "FindType",
                System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic,
                null,
                new System.Type[] { typeof(string), typeof(bool) },
                null);
            var tryWrapMethod = translatorType.GetMethod(
                "TryDelayWrapLoader",
                System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Public);

            if (findTypeMethod == null || tryWrapMethod == null)
            {
                UnityEngine.Debug.LogWarning("[GameUtil] Cannot reflect FindType/TryDelayWrapLoader");
                return false;
            }

            var type = (System.Type)findTypeMethod.Invoke(translator, new object[] { typeName, false });
            if (type == null)
            {
                //UnityEngine.Debug.LogWarning($"[GameUtil] Type not found: {typeName}");
                return false;
            }

            tryWrapMethod.Invoke(translator, new object[] { L, type });

            Lua.luaL_getmetatable(L, typeName);
            bool ok = !Lua.lua_isnil(L, -1);
            Lua.lua_pop(L, 1);
            if (!ok)
                UnityEngine.Debug.LogWarning($"[GameUtil] metatable still missing after wrap: {typeName}");
            return ok;
        }

        /// <summary>
        /// 返回类型的 obj_meta（xLua registry 以 typeName 为 key 的 metatable）。
        /// ⚠️ 直接向此表写入的字段不会被实例访问到，通常应使用 <see cref="GetMethodTable"/> 代替。
        /// </summary>
        public static LuaTable GetMetaTable(string typeName)
        {
            var luaEnv = LuaManager.Instance?.ActiveLuaEnv;
            if (luaEnv == null)
            {
                UnityEngine.Debug.LogWarning("[GameUtil.GetMetaTable] LuaEnv not ready");
                return null;
            }

            var L = luaEnv.L;
            ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            int top = Lua.lua_gettop(L);

            if (!EnsureTypeRegistered(L, translator, typeName))
            {
                Lua.lua_settop(L, top);
                return null;
            }

            Lua.luaL_getmetatable(L, typeName);
            LuaTable result = (LuaTable)translator.GetObject(L, -1);
            Lua.lua_settop(L, top);
            return result;
        }

        /// <summary>
        /// 返回类型的实例方法表（obj_field，即 obj_meta.__index 的 upvalue 1）。
        /// Override.lua 应使用此方法往实例注入扩展方法，而非 GetMetaTable()。
        /// 
        /// xLua 的 obj 实例查找路径：
        ///   userdata.__index → C闭包(gen_obj_indexer) → upvalue1=obj_field → 方法
        /// GetMetaTable 返回的是 obj_meta，写入 obj_meta 不会被 __index 查到。
        /// GetMethodTable 返回的是 obj_field，写入后实例可直接通过 : 调用。
        /// </summary>
        public static LuaTable GetMethodTable(string typeName)
        {
            var luaEnv = LuaManager.Instance?.ActiveLuaEnv;
            if (luaEnv == null)
            {
                UnityEngine.Debug.LogWarning("[GameUtil.GetMethodTable] LuaEnv not ready");
                return null;
            }

            var L = luaEnv.L;
            ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            int top = Lua.lua_gettop(L);

            if (!EnsureTypeRegistered(L, translator, typeName))
            {
                Lua.lua_settop(L, top);
                return null;
            }

            // 1. 取 obj_meta（registry[typeName]）
            Lua.luaL_getmetatable(L, typeName);  // stack: obj_meta
            if (Lua.lua_isnil(L, -1))
            {
                Lua.lua_settop(L, top);
                UnityEngine.Debug.LogWarning($"[GameUtil.GetMethodTable] No metatable for: {typeName}");
                return null;
            }

            // 2. 取 obj_meta.__index（是一个 C 闭包）
            Lua.xlua_pushasciistring(L, "__index");  // stack: obj_meta, "__index"
            Lua.lua_rawget(L, -2);                    // stack: obj_meta, __index_closure
            if (!Lua.lua_isfunction(L, -1))
            {
                Lua.lua_settop(L, top);
                UnityEngine.Debug.LogWarning($"[GameUtil.GetMethodTable] __index is not a function for: {typeName}");
                return null;
            }

            // 3. 取 upvalue 1 of __index_closure = obj_field（实例方法表）
            //    lua_getupvalue(L, funcIdx, n) 把 upvalue n 压栈并返回名字（IntPtr，零=无此upvalue）
            var upname = Lua.lua_getupvalue(L, -1, 1);  // stack: obj_meta, __index_closure, obj_field
            if (upname == IntPtr.Zero || Lua.lua_isnil(L, -1))
            {
                Lua.lua_settop(L, top);
                UnityEngine.Debug.LogWarning($"[GameUtil.GetMethodTable] Cannot get upvalue 1 of __index for: {typeName}");
                return null;
            }

            if (!Lua.lua_istable(L, -1))
            {
                Lua.lua_settop(L, top);
                UnityEngine.Debug.LogWarning($"[GameUtil.GetMethodTable] upvalue 1 is not a table for: {typeName}");
                return null;
            }

            LuaTable result = (LuaTable)translator.GetObject(L, -1);
            Lua.lua_settop(L, top);
            return result;
        }

        public static Texture2D LoadTexture2DFromFile(string path)
        {
            try
            {
                if (!File.Exists(path)) return null;

                // ����һ��Texture2D
                Texture2D texture = new Texture2D(1, 1);

                // ����ͼƬ����
                byte[] imageData = File.ReadAllBytes(path);

                // ��ͼƬ���ݼ��ص�Texture2D������
                texture.LoadImage(imageData);

                return texture;
            }
            catch (Exception e)
            {
                UnityEngine.Debug.LogException(e);
                return null;
            }
        }

        static IEnumerator _GetTexture(string url, Action<Texture2D> actionResult)
        {

            UnityWebRequest uwr = new UnityWebRequest(url);
            DownloadHandlerTexture downloadTexture = new DownloadHandlerTexture(true);
            uwr.downloadHandler = downloadTexture;

            yield return uwr.SendWebRequest();
            Texture2D t = null;
            if (uwr.result == UnityWebRequest.Result.Success)
            {
                t = downloadTexture.texture;
            }
            else
            {
                UnityEngine.Debug.LogWarning("Can't get texture: " + url);
            }

            if (actionResult != null)
            {
                actionResult(t);
            }
        }

        public static void AsyncLoadTextureFromPathOrUrl(string url, Action<Texture2D> cb)
        {
            GameLauncher.Instance.StartCoroutine(_GetTexture(url, (tex) =>
            {
                cb(tex);
            }));
        }

        public static bool SaveTextureToFile(Texture2D tex, string path)
        {
            try
            {
                CreateDirectoryForFile(path);
                Byte[] bytes = tex.EncodeToPNG();
                File.WriteAllBytes(path, bytes);
                return true;
            }
            catch (Exception e)
            {
                UnityEngine.Debug.LogException(e);
                return false;
            }
        }

        public static int AddGlobalTimer(float ttl, bool bOnce, FTimerList.TimerCallback callback, bool bLateUpdate = false)
        {
            if (GameLauncher.Instance == null) return -1;
            var comp = GameLauncher.Instance.GetComponent<FTimerListBehavior>();
            return comp.AddTimer(ttl, bOnce, callback, bLateUpdate);
        }

        public static void RemoveGlobalTimer(int timerId)
        {
            if (GameLauncher.Instance == null) return;
            var comp = GameLauncher.Instance.GetComponent<FTimerListBehavior>();
            comp.RemoveTimer(timerId);
        }

        public static int AddObjectTimer(GameObject go, float ttl, bool bOnce, FTimerList.TimerCallback callback, bool bLateUpdate = false)
        {
            if (go == null) return -1;
            var comp = go.GetComponent<FTimerListBehavior>();
            if(comp == null) comp = go.AddComponent<FTimerListBehavior>();
            return comp.AddTimer(ttl, bOnce, callback, bLateUpdate);
        }

        public static void RemoveObjectTimer(GameObject go, int timerId)
        {
            if (go == null) return;
            var comp = go.GetComponent<FTimerListBehavior>();
            if (comp == null) return;
            comp.RemoveTimer(timerId);
        }
        
        public static Type FindType(string qualifiedTypeName) 
        {
            Type t = Type.GetType(qualifiedTypeName);

            if (t != null)
            {
                return t;
            }
            var Assemblies = System.AppDomain.CurrentDomain.GetAssemblies();
            for (int n = 0; n < Assemblies.Length;n++ )
            {
                var asm = Assemblies[n];
                t = asm.GetType(qualifiedTypeName);
                if (t != null)
                    return t;
                Type[] types = asm.GetExportedTypes();
                foreach (Type ts in types)
                {
                    if (ts.Name.Equals(qualifiedTypeName))
                        return t;
                }
            }
            return null;
        }

        public static bool IsExtendByName(System.Object obj, string name)
        {
            Type t = FindType(name);
            return IsExtend(obj, t);
        }

        public static bool IsExtend(System.Object obj, Type type)
        {
            if (type == null) return false;
            bool ret = false;
            Type t = obj.GetType();
            while (t != null)
            {
                if (t == type)
                {
                    ret = true;
                    break;
                }

                t = t.BaseType;
            }

            return ret;
        }
    }
}
