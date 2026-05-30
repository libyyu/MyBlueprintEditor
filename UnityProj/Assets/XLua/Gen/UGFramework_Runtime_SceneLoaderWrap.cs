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
    public class UGFrameworkRuntimeSceneLoaderWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(UGFramework.Runtime.SceneLoader);
			Utils.BeginObjectRegister(type, L, translator, 0, 3, 0, 0);
			
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "LoadScene", _m_LoadScene);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "LoadSceneAdditive", _m_LoadSceneAdditive);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "UnloadScene", _m_UnloadScene);
			
			
			
			
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 4, 1, 0);
			Utils.RegisterFunc(L, Utils.CLS_IDX, "LuaLoadScene", _m_LuaLoadScene_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "LuaLoadSceneAdditive", _m_LuaLoadSceneAdditive_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "LuaUnloadScene", _m_LuaUnloadScene_xlua_st_);
            
			
            
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
					
					var gen_ret = new UGFramework.Runtime.SceneLoader();
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				
			}
			catch(System.Exception gen_e) {
				return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
			}
            return LuaAPI.luaL_error(L, "invalid arguments to UGFramework.Runtime.SceneLoader constructor!");
            
        }
        
		
        
		
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_LoadScene(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UGFramework.Runtime.SceneLoader gen_to_be_invoked = (UGFramework.Runtime.SceneLoader)translator.FastGetCSObj(L, 1);
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 4&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)&& translator.Assignable<System.Action<float>>(L, 3)&& translator.Assignable<System.Action>(L, 4)) 
                {
                    string _sceneName = LuaAPI.lua_tostring(L, 2);
                    System.Action<float> _onProgress = translator.GetDelegate<System.Action<float>>(L, 3);
                    System.Action _onComplete = translator.GetDelegate<System.Action>(L, 4);
                    
                    gen_to_be_invoked.LoadScene( _sceneName, _onProgress, _onComplete );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 3&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)&& translator.Assignable<System.Action<float>>(L, 3)) 
                {
                    string _sceneName = LuaAPI.lua_tostring(L, 2);
                    System.Action<float> _onProgress = translator.GetDelegate<System.Action<float>>(L, 3);
                    
                    gen_to_be_invoked.LoadScene( _sceneName, _onProgress );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)) 
                {
                    string _sceneName = LuaAPI.lua_tostring(L, 2);
                    
                    gen_to_be_invoked.LoadScene( _sceneName );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UGFramework.Runtime.SceneLoader.LoadScene!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_LoadSceneAdditive(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UGFramework.Runtime.SceneLoader gen_to_be_invoked = (UGFramework.Runtime.SceneLoader)translator.FastGetCSObj(L, 1);
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 4&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)&& translator.Assignable<System.Action<float>>(L, 3)&& translator.Assignable<System.Action>(L, 4)) 
                {
                    string _sceneName = LuaAPI.lua_tostring(L, 2);
                    System.Action<float> _onProgress = translator.GetDelegate<System.Action<float>>(L, 3);
                    System.Action _onComplete = translator.GetDelegate<System.Action>(L, 4);
                    
                    gen_to_be_invoked.LoadSceneAdditive( _sceneName, _onProgress, _onComplete );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 3&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)&& translator.Assignable<System.Action<float>>(L, 3)) 
                {
                    string _sceneName = LuaAPI.lua_tostring(L, 2);
                    System.Action<float> _onProgress = translator.GetDelegate<System.Action<float>>(L, 3);
                    
                    gen_to_be_invoked.LoadSceneAdditive( _sceneName, _onProgress );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)) 
                {
                    string _sceneName = LuaAPI.lua_tostring(L, 2);
                    
                    gen_to_be_invoked.LoadSceneAdditive( _sceneName );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UGFramework.Runtime.SceneLoader.LoadSceneAdditive!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_UnloadScene(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UGFramework.Runtime.SceneLoader gen_to_be_invoked = (UGFramework.Runtime.SceneLoader)translator.FastGetCSObj(L, 1);
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 3&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)&& translator.Assignable<System.Action>(L, 3)) 
                {
                    string _sceneName = LuaAPI.lua_tostring(L, 2);
                    System.Action _onComplete = translator.GetDelegate<System.Action>(L, 3);
                    
                    gen_to_be_invoked.UnloadScene( _sceneName, _onComplete );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)) 
                {
                    string _sceneName = LuaAPI.lua_tostring(L, 2);
                    
                    gen_to_be_invoked.UnloadScene( _sceneName );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UGFramework.Runtime.SceneLoader.UnloadScene!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_LuaLoadScene_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    string _sceneName = LuaAPI.lua_tostring(L, 1);
                    UGFramework.Runtime.SceneLoader.LuaProgressCallback _onProgress = translator.GetDelegate<UGFramework.Runtime.SceneLoader.LuaProgressCallback>(L, 2);
                    UGFramework.Runtime.SceneLoader.LuaCompleteCallback _onComplete = translator.GetDelegate<UGFramework.Runtime.SceneLoader.LuaCompleteCallback>(L, 3);
                    
                    UGFramework.Runtime.SceneLoader.LuaLoadScene( _sceneName, _onProgress, _onComplete );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_LuaLoadSceneAdditive_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    string _sceneName = LuaAPI.lua_tostring(L, 1);
                    UGFramework.Runtime.SceneLoader.LuaProgressCallback _onProgress = translator.GetDelegate<UGFramework.Runtime.SceneLoader.LuaProgressCallback>(L, 2);
                    UGFramework.Runtime.SceneLoader.LuaCompleteCallback _onComplete = translator.GetDelegate<UGFramework.Runtime.SceneLoader.LuaCompleteCallback>(L, 3);
                    
                    UGFramework.Runtime.SceneLoader.LuaLoadSceneAdditive( _sceneName, _onProgress, _onComplete );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_LuaUnloadScene_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    string _sceneName = LuaAPI.lua_tostring(L, 1);
                    UGFramework.Runtime.SceneLoader.LuaCompleteCallback _onComplete = translator.GetDelegate<UGFramework.Runtime.SceneLoader.LuaCompleteCallback>(L, 2);
                    
                    UGFramework.Runtime.SceneLoader.LuaUnloadScene( _sceneName, _onComplete );
                    
                    
                    
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
			    translator.Push(L, UGFramework.Runtime.SceneLoader.Instance);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        
        
		
		
		
		
    }
}
