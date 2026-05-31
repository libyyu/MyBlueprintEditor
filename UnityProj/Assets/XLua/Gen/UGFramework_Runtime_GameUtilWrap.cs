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
    public class UGFrameworkRuntimeGameUtilWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(UGFramework.Runtime.GameUtil);
			Utils.BeginObjectRegister(type, L, translator, 0, 0, 0, 0);
			
			
			
			
			
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 16, 0, 0);
			Utils.RegisterFunc(L, Utils.CLS_IDX, "IsEditorEnv", _m_IsEditorEnv_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "IsWXEnv", _m_IsWXEnv_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "IsWebGLEnv", _m_IsWebGLEnv_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "CreateDirectory", _m_CreateDirectory_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "CreateDirectoryForFile", _m_CreateDirectoryForFile_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "IsDirectoryExist", _m_IsDirectoryExist_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "ToHexString", _m_ToHexString_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "ToBytesString", _m_ToBytesString_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "IsPointerOverUIObject", _m_IsPointerOverUIObject_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "GetMetaTable", _m_GetMetaTable_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "LoadTexture2DFromFile", _m_LoadTexture2DFromFile_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "AddGlobalTimer", _m_AddGlobalTimer_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "RemoveGlobalTimer", _m_RemoveGlobalTimer_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "AddObjectTimer", _m_AddObjectTimer_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "RemoveObjectTimer", _m_RemoveObjectTimer_xlua_st_);
            
			
            
			
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            return LuaAPI.luaL_error(L, "UGFramework.Runtime.GameUtil does not have a constructor!");
        }
        
		
        
		
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_IsEditorEnv_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    
                        var gen_ret = UGFramework.Runtime.GameUtil.IsEditorEnv(  );
                        LuaAPI.lua_pushboolean(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_IsWXEnv_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    
                        var gen_ret = UGFramework.Runtime.GameUtil.IsWXEnv(  );
                        LuaAPI.lua_pushboolean(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_IsWebGLEnv_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    
                        var gen_ret = UGFramework.Runtime.GameUtil.IsWebGLEnv(  );
                        LuaAPI.lua_pushboolean(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_CreateDirectory_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    string _dir = LuaAPI.lua_tostring(L, 1);
                    
                    UGFramework.Runtime.GameUtil.CreateDirectory( _dir );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_CreateDirectoryForFile_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    string _filepath = LuaAPI.lua_tostring(L, 1);
                    
                    UGFramework.Runtime.GameUtil.CreateDirectoryForFile( _filepath );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_IsDirectoryExist_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    string _dir = LuaAPI.lua_tostring(L, 1);
                    
                        var gen_ret = UGFramework.Runtime.GameUtil.IsDirectoryExist( _dir );
                        LuaAPI.lua_pushboolean(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ToHexString_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& (LuaAPI.lua_isnil(L, 1) || LuaAPI.lua_type(L, 1) == LuaTypes.LUA_TSTRING)&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)) 
                {
                    byte[] _bytes = LuaAPI.lua_tobytes(L, 1);
                    string _sep = LuaAPI.lua_tostring(L, 2);
                    
                        var gen_ret = UGFramework.Runtime.GameUtil.ToHexString( _bytes, _sep );
                        LuaAPI.lua_pushstring(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& (LuaAPI.lua_isnil(L, 1) || LuaAPI.lua_type(L, 1) == LuaTypes.LUA_TSTRING)) 
                {
                    byte[] _bytes = LuaAPI.lua_tobytes(L, 1);
                    
                        var gen_ret = UGFramework.Runtime.GameUtil.ToHexString( _bytes );
                        LuaAPI.lua_pushstring(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UGFramework.Runtime.GameUtil.ToHexString!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ToBytesString_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& (LuaAPI.lua_isnil(L, 1) || LuaAPI.lua_type(L, 1) == LuaTypes.LUA_TSTRING)&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)) 
                {
                    byte[] _bytes = LuaAPI.lua_tobytes(L, 1);
                    string _sep = LuaAPI.lua_tostring(L, 2);
                    
                        var gen_ret = UGFramework.Runtime.GameUtil.ToBytesString( _bytes, _sep );
                        LuaAPI.lua_pushstring(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& (LuaAPI.lua_isnil(L, 1) || LuaAPI.lua_type(L, 1) == LuaTypes.LUA_TSTRING)) 
                {
                    byte[] _bytes = LuaAPI.lua_tobytes(L, 1);
                    
                        var gen_ret = UGFramework.Runtime.GameUtil.ToBytesString( _bytes );
                        LuaAPI.lua_pushstring(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UGFramework.Runtime.GameUtil.ToBytesString!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_IsPointerOverUIObject_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 0) 
                {
                    
                        var gen_ret = UGFramework.Runtime.GameUtil.IsPointerOverUIObject(  );
                        LuaAPI.lua_pushboolean(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.Canvas>(L, 1)&& translator.Assignable<UnityEngine.Vector2>(L, 2)) 
                {
                    UnityEngine.Canvas _canvas = (UnityEngine.Canvas)translator.GetObject(L, 1, typeof(UnityEngine.Canvas));
                    UnityEngine.Vector2 _screenPosition;translator.Get(L, 2, out _screenPosition);
                    
                        var gen_ret = UGFramework.Runtime.GameUtil.IsPointerOverUIObject( _canvas, _screenPosition );
                        LuaAPI.lua_pushboolean(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UGFramework.Runtime.GameUtil.IsPointerOverUIObject!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_GetMetaTable_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    string _typeName = LuaAPI.lua_tostring(L, 1);
                    
                        var gen_ret = UGFramework.Runtime.GameUtil.GetMetaTable( _typeName );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_LoadTexture2DFromFile_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    string _path = LuaAPI.lua_tostring(L, 1);
                    
                        var gen_ret = UGFramework.Runtime.GameUtil.LoadTexture2DFromFile( _path );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_AddGlobalTimer_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    float _ttl = (float)LuaAPI.lua_tonumber(L, 1);
                    bool _bOnce = LuaAPI.lua_toboolean(L, 2);
                    UGFramework.Runtime.FTimerList.TimerCallback _callback = translator.GetDelegate<UGFramework.Runtime.FTimerList.TimerCallback>(L, 3);
                    bool _bLateUpdate = LuaAPI.lua_toboolean(L, 4);
                    
                        var gen_ret = UGFramework.Runtime.GameUtil.AddGlobalTimer( _ttl, _bOnce, _callback, _bLateUpdate );
                        LuaAPI.xlua_pushinteger(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_RemoveGlobalTimer_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    int _timerId = LuaAPI.xlua_tointeger(L, 1);
                    
                    UGFramework.Runtime.GameUtil.RemoveGlobalTimer( _timerId );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_AddObjectTimer_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    UnityEngine.GameObject _go = (UnityEngine.GameObject)translator.GetObject(L, 1, typeof(UnityEngine.GameObject));
                    float _ttl = (float)LuaAPI.lua_tonumber(L, 2);
                    bool _bOnce = LuaAPI.lua_toboolean(L, 3);
                    UGFramework.Runtime.FTimerList.TimerCallback _callback = translator.GetDelegate<UGFramework.Runtime.FTimerList.TimerCallback>(L, 4);
                    bool _bLateUpdate = LuaAPI.lua_toboolean(L, 5);
                    
                        var gen_ret = UGFramework.Runtime.GameUtil.AddObjectTimer( _go, _ttl, _bOnce, _callback, _bLateUpdate );
                        LuaAPI.xlua_pushinteger(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_RemoveObjectTimer_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    UnityEngine.GameObject _go = (UnityEngine.GameObject)translator.GetObject(L, 1, typeof(UnityEngine.GameObject));
                    int _timerId = LuaAPI.xlua_tointeger(L, 2);
                    
                    UGFramework.Runtime.GameUtil.RemoveObjectTimer( _go, _timerId );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        
        
        
        
        
		
		
		
		
    }
}
