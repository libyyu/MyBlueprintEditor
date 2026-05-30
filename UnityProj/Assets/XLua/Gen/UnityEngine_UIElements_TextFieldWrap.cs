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
    public class UnityEngineUIElementsTextFieldWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(UnityEngine.UIElements.TextField);
			Utils.BeginObjectRegister(type, L, translator, 0, 1, 2, 2);
			
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "SetValueWithoutNotify", _m_SetValueWithoutNotify);
			
			
			Utils.RegisterFunc(L, Utils.GETTER_IDX, "multiline", _g_get_multiline);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "value", _g_get_value);
            
			Utils.RegisterFunc(L, Utils.SETTER_IDX, "multiline", _s_set_multiline);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "value", _s_set_value);
            
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 4, 0, 0);
			
			
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "ussClassName", UnityEngine.UIElements.TextField.ussClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "labelUssClassName", UnityEngine.UIElements.TextField.labelUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "inputUssClassName", UnityEngine.UIElements.TextField.inputUssClassName);
            
			
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            
			try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
				if(LuaAPI.lua_gettop(L) == 1)
				{
					
					var gen_ret = new UnityEngine.UIElements.TextField();
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 5 && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2) && LuaTypes.LUA_TBOOLEAN == LuaAPI.lua_type(L, 3) && LuaTypes.LUA_TBOOLEAN == LuaAPI.lua_type(L, 4) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 5))
				{
					int _maxLength = LuaAPI.xlua_tointeger(L, 2);
					bool _multiline = LuaAPI.lua_toboolean(L, 3);
					bool _isPasswordField = LuaAPI.lua_toboolean(L, 4);
					char _maskChar = (char)LuaAPI.xlua_tointeger(L, 5);
					
					var gen_ret = new UnityEngine.UIElements.TextField(_maxLength, _multiline, _isPasswordField, _maskChar);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 2 && (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING))
				{
					string _label = LuaAPI.lua_tostring(L, 2);
					
					var gen_ret = new UnityEngine.UIElements.TextField(_label);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 6 && (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 3) && LuaTypes.LUA_TBOOLEAN == LuaAPI.lua_type(L, 4) && LuaTypes.LUA_TBOOLEAN == LuaAPI.lua_type(L, 5) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 6))
				{
					string _label = LuaAPI.lua_tostring(L, 2);
					int _maxLength = LuaAPI.xlua_tointeger(L, 3);
					bool _multiline = LuaAPI.lua_toboolean(L, 4);
					bool _isPasswordField = LuaAPI.lua_toboolean(L, 5);
					char _maskChar = (char)LuaAPI.xlua_tointeger(L, 6);
					
					var gen_ret = new UnityEngine.UIElements.TextField(_label, _maxLength, _multiline, _isPasswordField, _maskChar);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				
			}
			catch(System.Exception gen_e) {
				return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
			}
            return LuaAPI.luaL_error(L, "invalid arguments to UnityEngine.UIElements.TextField constructor!");
            
        }
        
		
        
		
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_SetValueWithoutNotify(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.TextField gen_to_be_invoked = (UnityEngine.UIElements.TextField)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    string _newValue = LuaAPI.lua_tostring(L, 2);
                    
                    gen_to_be_invoked.SetValueWithoutNotify( _newValue );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_multiline(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.TextField gen_to_be_invoked = (UnityEngine.UIElements.TextField)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushboolean(L, gen_to_be_invoked.multiline);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_value(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.TextField gen_to_be_invoked = (UnityEngine.UIElements.TextField)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushstring(L, gen_to_be_invoked.value);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_multiline(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.TextField gen_to_be_invoked = (UnityEngine.UIElements.TextField)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.multiline = LuaAPI.lua_toboolean(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_value(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.TextField gen_to_be_invoked = (UnityEngine.UIElements.TextField)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.value = LuaAPI.lua_tostring(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
		
		
		
		
    }
}
