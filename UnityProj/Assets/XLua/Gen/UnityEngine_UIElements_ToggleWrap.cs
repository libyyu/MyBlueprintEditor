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
    public class UnityEngineUIElementsToggleWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(UnityEngine.UIElements.Toggle);
			Utils.BeginObjectRegister(type, L, translator, 0, 0, 0, 0);
			
			
			
			
			
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 7, 0, 0);
			
			
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "ussClassName", UnityEngine.UIElements.Toggle.ussClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "labelUssClassName", UnityEngine.UIElements.Toggle.labelUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "inputUssClassName", UnityEngine.UIElements.Toggle.inputUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "checkmarkUssClassName", UnityEngine.UIElements.Toggle.checkmarkUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "textUssClassName", UnityEngine.UIElements.Toggle.textUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "mixedValuesUssClassName", UnityEngine.UIElements.Toggle.mixedValuesUssClassName);
            
			
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            
			try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
				if(LuaAPI.lua_gettop(L) == 1)
				{
					
					var gen_ret = new UnityEngine.UIElements.Toggle();
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 2 && (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING))
				{
					string _label = LuaAPI.lua_tostring(L, 2);
					
					var gen_ret = new UnityEngine.UIElements.Toggle(_label);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				
			}
			catch(System.Exception gen_e) {
				return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
			}
            return LuaAPI.luaL_error(L, "invalid arguments to UnityEngine.UIElements.Toggle constructor!");
            
        }
        
		
        
		
        
        
        
        
        
        
        
        
        
		
		
		
		
    }
}
