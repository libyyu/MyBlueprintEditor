#if USE_UNI_LUA
using LuaAPI = UniLua.Lua;
using RealStatePtr = UniLua.ILuaState;
using LuaCSFunction = UniLua.CSharpFunctionDelegate;
#else
using LuaAPI = XLua.LuaDLL.Lua;
using RealStatePtr = System.IntPtr;
using LuaCSFunction = XLua.LuaDLL.lua_CSFunction;
#endif

using XLua;
using System.Collections.Generic;


namespace XLua.CSObjectWrap
{
    using Utils = XLua.Utils;
    
    public class UnityEngineUIElementsDisplayStyleWrap
    {
		public static void __Register(RealStatePtr L)
        {
		    ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
		    Utils.BeginObjectRegister(typeof(UnityEngine.UIElements.DisplayStyle), L, translator, 0, 0, 0, 0);
			Utils.EndObjectRegister(typeof(UnityEngine.UIElements.DisplayStyle), L, translator, null, null, null, null, null);
			
			Utils.BeginClassRegister(typeof(UnityEngine.UIElements.DisplayStyle), L, null, 3, 0, 0);

            
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "Flex", UnityEngine.UIElements.DisplayStyle.Flex);
            
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "None", UnityEngine.UIElements.DisplayStyle.None);
            

			Utils.RegisterFunc(L, Utils.CLS_IDX, "__CastFrom", __CastFrom);
            
            Utils.EndClassRegister(typeof(UnityEngine.UIElements.DisplayStyle), L, translator);
        }
		
		[MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CastFrom(RealStatePtr L)
		{
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			LuaTypes lua_type = LuaAPI.lua_type(L, 1);
            if (lua_type == LuaTypes.LUA_TNUMBER)
            {
                translator.PushUnityEngineUIElementsDisplayStyle(L, (UnityEngine.UIElements.DisplayStyle)LuaAPI.xlua_tointeger(L, 1));
            }
			
            else if(lua_type == LuaTypes.LUA_TSTRING)
            {

			    if (LuaAPI.xlua_is_eq_str(L, 1, "Flex"))
                {
                    translator.PushUnityEngineUIElementsDisplayStyle(L, UnityEngine.UIElements.DisplayStyle.Flex);
                }
				else if (LuaAPI.xlua_is_eq_str(L, 1, "None"))
                {
                    translator.PushUnityEngineUIElementsDisplayStyle(L, UnityEngine.UIElements.DisplayStyle.None);
                }
				else
                {
                    return LuaAPI.luaL_error(L, "invalid string for UnityEngine.UIElements.DisplayStyle!");
                }

            }
			
            else
            {
                return LuaAPI.luaL_error(L, "invalid lua type for UnityEngine.UIElements.DisplayStyle! Expect number or string, got + " + lua_type);
            }

            return 1;
		}
	}
    
    public class UnityEngineUIElementsVisibilityWrap
    {
		public static void __Register(RealStatePtr L)
        {
		    ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
		    Utils.BeginObjectRegister(typeof(UnityEngine.UIElements.Visibility), L, translator, 0, 0, 0, 0);
			Utils.EndObjectRegister(typeof(UnityEngine.UIElements.Visibility), L, translator, null, null, null, null, null);
			
			Utils.BeginClassRegister(typeof(UnityEngine.UIElements.Visibility), L, null, 3, 0, 0);

            
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "Visible", UnityEngine.UIElements.Visibility.Visible);
            
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "Hidden", UnityEngine.UIElements.Visibility.Hidden);
            

			Utils.RegisterFunc(L, Utils.CLS_IDX, "__CastFrom", __CastFrom);
            
            Utils.EndClassRegister(typeof(UnityEngine.UIElements.Visibility), L, translator);
        }
		
		[MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CastFrom(RealStatePtr L)
		{
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			LuaTypes lua_type = LuaAPI.lua_type(L, 1);
            if (lua_type == LuaTypes.LUA_TNUMBER)
            {
                translator.PushUnityEngineUIElementsVisibility(L, (UnityEngine.UIElements.Visibility)LuaAPI.xlua_tointeger(L, 1));
            }
			
            else if(lua_type == LuaTypes.LUA_TSTRING)
            {

			    if (LuaAPI.xlua_is_eq_str(L, 1, "Visible"))
                {
                    translator.PushUnityEngineUIElementsVisibility(L, UnityEngine.UIElements.Visibility.Visible);
                }
				else if (LuaAPI.xlua_is_eq_str(L, 1, "Hidden"))
                {
                    translator.PushUnityEngineUIElementsVisibility(L, UnityEngine.UIElements.Visibility.Hidden);
                }
				else
                {
                    return LuaAPI.luaL_error(L, "invalid string for UnityEngine.UIElements.Visibility!");
                }

            }
			
            else
            {
                return LuaAPI.luaL_error(L, "invalid lua type for UnityEngine.UIElements.Visibility! Expect number or string, got + " + lua_type);
            }

            return 1;
		}
	}
    
}