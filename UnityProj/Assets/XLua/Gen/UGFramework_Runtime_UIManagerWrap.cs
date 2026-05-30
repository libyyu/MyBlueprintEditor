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
    public class UGFrameworkRuntimeUIManagerWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(UGFramework.Runtime.UIManager);
			Utils.BeginObjectRegister(type, L, translator, 0, 4, 1, 1);
			
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "OpenPanel", _m_OpenPanel);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "ClosePanel", _m_ClosePanel);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "DisposePanel", _m_DisposePanel);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "CloseAll", _m_CloseAll);
			
			
			Utils.RegisterFunc(L, Utils.GETTER_IDX, "uiRoot", _g_get_uiRoot);
            
			Utils.RegisterFunc(L, Utils.SETTER_IDX, "uiRoot", _s_set_uiRoot);
            
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 6, 1, 0);
			Utils.RegisterFunc(L, Utils.CLS_IDX, "RegisterViewFactory", _m_RegisterViewFactory_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "LuaOpen", _m_LuaOpen_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "LuaClose", _m_LuaClose_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "LuaDispose", _m_LuaDispose_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "LuaCloseAll", _m_LuaCloseAll_xlua_st_);
            
			
            
			Utils.RegisterFunc(L, Utils.CLS_GETTER_IDX, "Instance", _g_get_Instance);
            
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            
			try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
				if(LuaAPI.lua_gettop(L) == 1)
				{
					
					var gen_ret = new UGFramework.Runtime.UIManager();
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				
			}
			catch(System.Exception gen_e) {
				return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
			}
            return LuaAPI.luaL_error(L, "invalid arguments to UGFramework.Runtime.UIManager constructor!");
            
        }
        
		
        
		
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_RegisterViewFactory_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    System.Func<UnityEngine.GameObject, UGFramework.Runtime.IView> _factory = translator.GetDelegate<System.Func<UnityEngine.GameObject, UGFramework.Runtime.IView>>(L, 1);
                    
                    UGFramework.Runtime.UIManager.RegisterViewFactory( _factory );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_OpenPanel(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UGFramework.Runtime.UIManager gen_to_be_invoked = (UGFramework.Runtime.UIManager)translator.FastGetCSObj(L, 1);
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 4&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)&& (LuaAPI.lua_isnil(L, 3) || LuaAPI.lua_type(L, 3) == LuaTypes.LUA_TSTRING)&& translator.Assignable<System.Action<UGFramework.Runtime.IView>>(L, 4)) 
                {
                    string _address = LuaAPI.lua_tostring(L, 2);
                    string _param = LuaAPI.lua_tostring(L, 3);
                    System.Action<UGFramework.Runtime.IView> _onComplete = translator.GetDelegate<System.Action<UGFramework.Runtime.IView>>(L, 4);
                    
                    gen_to_be_invoked.OpenPanel( _address, _param, _onComplete );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 3&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)&& (LuaAPI.lua_isnil(L, 3) || LuaAPI.lua_type(L, 3) == LuaTypes.LUA_TSTRING)) 
                {
                    string _address = LuaAPI.lua_tostring(L, 2);
                    string _param = LuaAPI.lua_tostring(L, 3);
                    
                    gen_to_be_invoked.OpenPanel( _address, _param );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)) 
                {
                    string _address = LuaAPI.lua_tostring(L, 2);
                    
                    gen_to_be_invoked.OpenPanel( _address );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UGFramework.Runtime.UIManager.OpenPanel!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ClosePanel(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UGFramework.Runtime.UIManager gen_to_be_invoked = (UGFramework.Runtime.UIManager)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    string _address = LuaAPI.lua_tostring(L, 2);
                    
                    gen_to_be_invoked.ClosePanel( _address );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_DisposePanel(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UGFramework.Runtime.UIManager gen_to_be_invoked = (UGFramework.Runtime.UIManager)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    string _address = LuaAPI.lua_tostring(L, 2);
                    
                    gen_to_be_invoked.DisposePanel( _address );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_CloseAll(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UGFramework.Runtime.UIManager gen_to_be_invoked = (UGFramework.Runtime.UIManager)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                    gen_to_be_invoked.CloseAll(  );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_LuaOpen_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    string _address = LuaAPI.lua_tostring(L, 1);
                    string _param = LuaAPI.lua_tostring(L, 2);
                    UGFramework.Runtime.UIManager.LuaViewCallback _onComplete = translator.GetDelegate<UGFramework.Runtime.UIManager.LuaViewCallback>(L, 3);
                    
                    UGFramework.Runtime.UIManager.LuaOpen( _address, _param, _onComplete );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_LuaClose_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    string _address = LuaAPI.lua_tostring(L, 1);
                    
                    UGFramework.Runtime.UIManager.LuaClose( _address );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_LuaDispose_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    string _address = LuaAPI.lua_tostring(L, 1);
                    
                    UGFramework.Runtime.UIManager.LuaDispose( _address );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_LuaCloseAll_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    
                    UGFramework.Runtime.UIManager.LuaCloseAll(  );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_Instance(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			    translator.Push(L, UGFramework.Runtime.UIManager.Instance);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_uiRoot(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UGFramework.Runtime.UIManager gen_to_be_invoked = (UGFramework.Runtime.UIManager)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.uiRoot);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_uiRoot(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UGFramework.Runtime.UIManager gen_to_be_invoked = (UGFramework.Runtime.UIManager)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.uiRoot = (UnityEngine.Transform)translator.GetObject(L, 2, typeof(UnityEngine.Transform));
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
		
		
		
		
    }
}
