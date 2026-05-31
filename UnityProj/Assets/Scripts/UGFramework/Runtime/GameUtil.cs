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
        public static bool IsEditorEnv()
        {
#if UNITY_EDITOR
            return true;
#else
            return false;
#endif
        }

        //��ǰ�Ƿ�������΢��С��Ϸ����
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
                UnityEngine.Debug.LogWarning("����ʧ�ܣ��������磬�������ص�ַ�Ƿ���ȷ�� " + url);
            }

            if (actionResult != null)
            {
                actionResult(t);
            }
        }

        /// <summary>
        /// 根据 C# 类型全名返回该类型在 Lua 中的 metatable（LuaTable）。
        /// 如果该类型还没有被 push 过，会主动触发 xLua 为其创建 metatable。
        /// Lua 侧用法：
        ///   local mt = CS.UGFramework.Runtime.GameUtil.GetMetaTable("UnityEngine.Component")
        ///   local raw = mt.__index
        ///   mt.__index = function(obj, key) ... return raw(obj, key) end
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

            // 1. 尝试直接从 registry 取（已经 push 过的类型）
            Lua.luaL_getmetatable(L, typeName);
            bool exists = !Lua.lua_isnil(L, -1);
            Lua.lua_pop(L, 1);

            if (!exists)
            {
                // 2. 类型还没注册过，用 translator.FindType + TryDelayWrapLoader 主动触发
                //    FindType / TryDelayWrapLoader 是 internal 方法，通过反射调用
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
                    UnityEngine.Debug.LogWarning("[GameUtil.GetMetaTable] Cannot reflect FindType/TryDelayWrapLoader");
                    return null;
                }

                var type = (System.Type)findTypeMethod.Invoke(translator, new object[] { typeName, false });
                if (type == null)
                {
                    UnityEngine.Debug.LogWarning($"[GameUtil.GetMetaTable] Type not found: {typeName}");
                    return null;
                }

                // TryDelayWrapLoader 会在 registry 里建好 metatable
                tryWrapMethod.Invoke(translator, new object[] { L, type });
            }

            // 3. 再次从 registry 取，这次必定有
            Lua.luaL_getmetatable(L, typeName);
            if (Lua.lua_isnil(L, -1))
            {
                Lua.lua_pop(L, 1);
                UnityEngine.Debug.LogWarning($"[GameUtil.GetMetaTable] metatable still missing after wrap: {typeName}");
                return null;
            }

            LuaTable result = (LuaTable)translator.GetObject(L, -1);
            Lua.lua_settop(L, top); // 恢复栈
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

    }
}
