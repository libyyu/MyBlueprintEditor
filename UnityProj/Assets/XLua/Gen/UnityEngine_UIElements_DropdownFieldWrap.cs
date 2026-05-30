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
    public class UnityEngineUIElementsDropdownFieldWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(UnityEngine.UIElements.DropdownField);
			Utils.BeginObjectRegister(type, L, translator, 0, 0, 0, 0);
			
			
			
			
			
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 1, 0, 0);
			
			
            
			
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            
			try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
				if(LuaAPI.lua_gettop(L) == 1)
				{
					
					var gen_ret = new UnityEngine.UIElements.DropdownField();
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 2 && (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING))
				{
					string _label = LuaAPI.lua_tostring(L, 2);
					
					var gen_ret = new UnityEngine.UIElements.DropdownField(_label);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 5 && translator.Assignable<System.Collections.Generic.List<string>>(L, 2) && (LuaAPI.lua_isnil(L, 3) || LuaAPI.lua_type(L, 3) == LuaTypes.LUA_TSTRING) && translator.Assignable<System.Func<string, string>>(L, 4) && translator.Assignable<System.Func<string, string>>(L, 5))
				{
					System.Collections.Generic.List<string> _choices = (System.Collections.Generic.List<string>)translator.GetObject(L, 2, typeof(System.Collections.Generic.List<string>));
					string _defaultValue = LuaAPI.lua_tostring(L, 3);
					System.Func<string, string> _formatSelectedValueCallback = translator.GetDelegate<System.Func<string, string>>(L, 4);
					System.Func<string, string> _formatListItemCallback = translator.GetDelegate<System.Func<string, string>>(L, 5);
					
					var gen_ret = new UnityEngine.UIElements.DropdownField(_choices, _defaultValue, _formatSelectedValueCallback, _formatListItemCallback);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 4 && translator.Assignable<System.Collections.Generic.List<string>>(L, 2) && (LuaAPI.lua_isnil(L, 3) || LuaAPI.lua_type(L, 3) == LuaTypes.LUA_TSTRING) && translator.Assignable<System.Func<string, string>>(L, 4))
				{
					System.Collections.Generic.List<string> _choices = (System.Collections.Generic.List<string>)translator.GetObject(L, 2, typeof(System.Collections.Generic.List<string>));
					string _defaultValue = LuaAPI.lua_tostring(L, 3);
					System.Func<string, string> _formatSelectedValueCallback = translator.GetDelegate<System.Func<string, string>>(L, 4);
					
					var gen_ret = new UnityEngine.UIElements.DropdownField(_choices, _defaultValue, _formatSelectedValueCallback);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 3 && translator.Assignable<System.Collections.Generic.List<string>>(L, 2) && (LuaAPI.lua_isnil(L, 3) || LuaAPI.lua_type(L, 3) == LuaTypes.LUA_TSTRING))
				{
					System.Collections.Generic.List<string> _choices = (System.Collections.Generic.List<string>)translator.GetObject(L, 2, typeof(System.Collections.Generic.List<string>));
					string _defaultValue = LuaAPI.lua_tostring(L, 3);
					
					var gen_ret = new UnityEngine.UIElements.DropdownField(_choices, _defaultValue);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 6 && (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING) && translator.Assignable<System.Collections.Generic.List<string>>(L, 3) && (LuaAPI.lua_isnil(L, 4) || LuaAPI.lua_type(L, 4) == LuaTypes.LUA_TSTRING) && translator.Assignable<System.Func<string, string>>(L, 5) && translator.Assignable<System.Func<string, string>>(L, 6))
				{
					string _label = LuaAPI.lua_tostring(L, 2);
					System.Collections.Generic.List<string> _choices = (System.Collections.Generic.List<string>)translator.GetObject(L, 3, typeof(System.Collections.Generic.List<string>));
					string _defaultValue = LuaAPI.lua_tostring(L, 4);
					System.Func<string, string> _formatSelectedValueCallback = translator.GetDelegate<System.Func<string, string>>(L, 5);
					System.Func<string, string> _formatListItemCallback = translator.GetDelegate<System.Func<string, string>>(L, 6);
					
					var gen_ret = new UnityEngine.UIElements.DropdownField(_label, _choices, _defaultValue, _formatSelectedValueCallback, _formatListItemCallback);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 5 && (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING) && translator.Assignable<System.Collections.Generic.List<string>>(L, 3) && (LuaAPI.lua_isnil(L, 4) || LuaAPI.lua_type(L, 4) == LuaTypes.LUA_TSTRING) && translator.Assignable<System.Func<string, string>>(L, 5))
				{
					string _label = LuaAPI.lua_tostring(L, 2);
					System.Collections.Generic.List<string> _choices = (System.Collections.Generic.List<string>)translator.GetObject(L, 3, typeof(System.Collections.Generic.List<string>));
					string _defaultValue = LuaAPI.lua_tostring(L, 4);
					System.Func<string, string> _formatSelectedValueCallback = translator.GetDelegate<System.Func<string, string>>(L, 5);
					
					var gen_ret = new UnityEngine.UIElements.DropdownField(_label, _choices, _defaultValue, _formatSelectedValueCallback);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 4 && (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING) && translator.Assignable<System.Collections.Generic.List<string>>(L, 3) && (LuaAPI.lua_isnil(L, 4) || LuaAPI.lua_type(L, 4) == LuaTypes.LUA_TSTRING))
				{
					string _label = LuaAPI.lua_tostring(L, 2);
					System.Collections.Generic.List<string> _choices = (System.Collections.Generic.List<string>)translator.GetObject(L, 3, typeof(System.Collections.Generic.List<string>));
					string _defaultValue = LuaAPI.lua_tostring(L, 4);
					
					var gen_ret = new UnityEngine.UIElements.DropdownField(_label, _choices, _defaultValue);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 5 && translator.Assignable<System.Collections.Generic.List<string>>(L, 2) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 3) && translator.Assignable<System.Func<string, string>>(L, 4) && translator.Assignable<System.Func<string, string>>(L, 5))
				{
					System.Collections.Generic.List<string> _choices = (System.Collections.Generic.List<string>)translator.GetObject(L, 2, typeof(System.Collections.Generic.List<string>));
					int _defaultIndex = LuaAPI.xlua_tointeger(L, 3);
					System.Func<string, string> _formatSelectedValueCallback = translator.GetDelegate<System.Func<string, string>>(L, 4);
					System.Func<string, string> _formatListItemCallback = translator.GetDelegate<System.Func<string, string>>(L, 5);
					
					var gen_ret = new UnityEngine.UIElements.DropdownField(_choices, _defaultIndex, _formatSelectedValueCallback, _formatListItemCallback);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 4 && translator.Assignable<System.Collections.Generic.List<string>>(L, 2) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 3) && translator.Assignable<System.Func<string, string>>(L, 4))
				{
					System.Collections.Generic.List<string> _choices = (System.Collections.Generic.List<string>)translator.GetObject(L, 2, typeof(System.Collections.Generic.List<string>));
					int _defaultIndex = LuaAPI.xlua_tointeger(L, 3);
					System.Func<string, string> _formatSelectedValueCallback = translator.GetDelegate<System.Func<string, string>>(L, 4);
					
					var gen_ret = new UnityEngine.UIElements.DropdownField(_choices, _defaultIndex, _formatSelectedValueCallback);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 3 && translator.Assignable<System.Collections.Generic.List<string>>(L, 2) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 3))
				{
					System.Collections.Generic.List<string> _choices = (System.Collections.Generic.List<string>)translator.GetObject(L, 2, typeof(System.Collections.Generic.List<string>));
					int _defaultIndex = LuaAPI.xlua_tointeger(L, 3);
					
					var gen_ret = new UnityEngine.UIElements.DropdownField(_choices, _defaultIndex);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 6 && (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING) && translator.Assignable<System.Collections.Generic.List<string>>(L, 3) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 4) && translator.Assignable<System.Func<string, string>>(L, 5) && translator.Assignable<System.Func<string, string>>(L, 6))
				{
					string _label = LuaAPI.lua_tostring(L, 2);
					System.Collections.Generic.List<string> _choices = (System.Collections.Generic.List<string>)translator.GetObject(L, 3, typeof(System.Collections.Generic.List<string>));
					int _defaultIndex = LuaAPI.xlua_tointeger(L, 4);
					System.Func<string, string> _formatSelectedValueCallback = translator.GetDelegate<System.Func<string, string>>(L, 5);
					System.Func<string, string> _formatListItemCallback = translator.GetDelegate<System.Func<string, string>>(L, 6);
					
					var gen_ret = new UnityEngine.UIElements.DropdownField(_label, _choices, _defaultIndex, _formatSelectedValueCallback, _formatListItemCallback);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 5 && (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING) && translator.Assignable<System.Collections.Generic.List<string>>(L, 3) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 4) && translator.Assignable<System.Func<string, string>>(L, 5))
				{
					string _label = LuaAPI.lua_tostring(L, 2);
					System.Collections.Generic.List<string> _choices = (System.Collections.Generic.List<string>)translator.GetObject(L, 3, typeof(System.Collections.Generic.List<string>));
					int _defaultIndex = LuaAPI.xlua_tointeger(L, 4);
					System.Func<string, string> _formatSelectedValueCallback = translator.GetDelegate<System.Func<string, string>>(L, 5);
					
					var gen_ret = new UnityEngine.UIElements.DropdownField(_label, _choices, _defaultIndex, _formatSelectedValueCallback);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 4 && (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING) && translator.Assignable<System.Collections.Generic.List<string>>(L, 3) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 4))
				{
					string _label = LuaAPI.lua_tostring(L, 2);
					System.Collections.Generic.List<string> _choices = (System.Collections.Generic.List<string>)translator.GetObject(L, 3, typeof(System.Collections.Generic.List<string>));
					int _defaultIndex = LuaAPI.xlua_tointeger(L, 4);
					
					var gen_ret = new UnityEngine.UIElements.DropdownField(_label, _choices, _defaultIndex);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				
			}
			catch(System.Exception gen_e) {
				return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
			}
            return LuaAPI.luaL_error(L, "invalid arguments to UnityEngine.UIElements.DropdownField constructor!");
            
        }
        
		
        
		
        
        
        
        
        
        
        
        
        
		
		
		
		
    }
}
