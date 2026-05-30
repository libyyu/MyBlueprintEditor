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
    public class UGFrameworkRuntimeYooAssetsLuaBridgeWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(UGFramework.Runtime.YooAssetsLuaBridge);
			Utils.BeginObjectRegister(type, L, translator, 0, 0, 0, 0);
			
			
			
			
			
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 28, 1, 0);
			Utils.RegisterFunc(L, Utils.CLS_IDX, "HasLuaFile", _m_HasLuaFile_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "GetLuaAssetHandle", _m_GetLuaAssetHandle_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "ReleaseLuaAssetHandle", _m_ReleaseLuaAssetHandle_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "GetLuaAssetPath", _m_GetLuaAssetPath_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "GetAllLuaNames", _m_GetAllLuaNames_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "ClearLuaIndex", _m_ClearLuaIndex_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "SetupYooAssets", _m_SetupYooAssets_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "HasPackage", _m_HasPackage_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "RemovePackage", _m_RemovePackage_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "CreatePackage", _m_CreatePackage_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "SetDefaultPackage", _m_SetDefaultPackage_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "InitializeOffline", _m_InitializeOffline_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "InitializeHostPlay", _m_InitializeHostPlay_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "RequestVersion", _m_RequestVersion_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "GetPackageVersion", _m_GetPackageVersion_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "UpdateManifest", _m_UpdateManifest_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "GetDownloadCount", _m_GetDownloadCount_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "StartDownload", _m_StartDownload_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "LoadAsset", _m_LoadAsset_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "InstantiateAsync", _m_InstantiateAsync_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "LoadScene", _m_LoadScene_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "LoadRawFile", _m_LoadRawFile_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "LoadAllLuaFiles", _m_LoadAllLuaFiles_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "TryUnloadUnusedAsset", _m_TryUnloadUnusedAsset_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "UnloadUnusedAssets", _m_UnloadUnusedAssets_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "UnloadAllAssets", _m_UnloadAllAssets_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "ClearCache", _m_ClearCache_xlua_st_);
            
			
            
			Utils.RegisterFunc(L, Utils.CLS_GETTER_IDX, "LuaFileCount", _g_get_LuaFileCount);
            
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            return LuaAPI.luaL_error(L, "UGFramework.Runtime.YooAssetsLuaBridge does not have a constructor!");
        }
        
		
        
		
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_HasLuaFile_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    string _requireName = LuaAPI.lua_tostring(L, 1);
                    
                        var gen_ret = UGFramework.Runtime.YooAssetsLuaBridge.HasLuaFile( _requireName );
                        LuaAPI.lua_pushboolean(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_GetLuaAssetHandle_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    string _requireName = LuaAPI.lua_tostring(L, 1);
                    
                        var gen_ret = UGFramework.Runtime.YooAssetsLuaBridge.GetLuaAssetHandle( _requireName );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ReleaseLuaAssetHandle_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    string _requireName = LuaAPI.lua_tostring(L, 1);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.ReleaseLuaAssetHandle( _requireName );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_GetLuaAssetPath_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    string _requireName = LuaAPI.lua_tostring(L, 1);
                    
                        var gen_ret = UGFramework.Runtime.YooAssetsLuaBridge.GetLuaAssetPath( _requireName );
                        LuaAPI.lua_pushstring(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_GetAllLuaNames_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    
                        var gen_ret = UGFramework.Runtime.YooAssetsLuaBridge.GetAllLuaNames(  );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ClearLuaIndex_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.ClearLuaIndex(  );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_SetupYooAssets_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.SetupYooAssets(  );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_HasPackage_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    
                        var gen_ret = UGFramework.Runtime.YooAssetsLuaBridge.HasPackage( _packageName );
                        LuaAPI.lua_pushboolean(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_RemovePackage_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    
                        var gen_ret = UGFramework.Runtime.YooAssetsLuaBridge.RemovePackage( _packageName );
                        LuaAPI.lua_pushboolean(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_CreatePackage_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    
                        var gen_ret = UGFramework.Runtime.YooAssetsLuaBridge.CreatePackage( _packageName );
                        LuaAPI.lua_pushboolean(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_SetDefaultPackage_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.SetDefaultPackage( _packageName );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_InitializeOffline_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback _callback = translator.GetDelegate<UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback>(L, 2);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.InitializeOffline( _packageName, _callback );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_InitializeHostPlay_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    string _mainCdnUrl = LuaAPI.lua_tostring(L, 2);
                    string _fallbackCdnUrl = LuaAPI.lua_tostring(L, 3);
                    UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback _callback = translator.GetDelegate<UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback>(L, 4);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.InitializeHostPlay( _packageName, _mainCdnUrl, _fallbackCdnUrl, _callback );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_RequestVersion_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringStringCallback _callback = translator.GetDelegate<UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringStringCallback>(L, 2);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.RequestVersion( _packageName, _callback );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_GetPackageVersion_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    
                        var gen_ret = UGFramework.Runtime.YooAssetsLuaBridge.GetPackageVersion( _packageName );
                        LuaAPI.lua_pushstring(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_UpdateManifest_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    string _version = LuaAPI.lua_tostring(L, 2);
                    UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback _callback = translator.GetDelegate<UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback>(L, 3);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.UpdateManifest( _packageName, _version, _callback );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_GetDownloadCount_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 3&& (LuaAPI.lua_isnil(L, 1) || LuaAPI.lua_type(L, 1) == LuaTypes.LUA_TSTRING)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 3)) 
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    int _maxConcurrent = LuaAPI.xlua_tointeger(L, 2);
                    int _retryCount = LuaAPI.xlua_tointeger(L, 3);
                    
                        var gen_ret = UGFramework.Runtime.YooAssetsLuaBridge.GetDownloadCount( _packageName, _maxConcurrent, _retryCount );
                        LuaAPI.xlua_pushinteger(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 2&& (LuaAPI.lua_isnil(L, 1) || LuaAPI.lua_type(L, 1) == LuaTypes.LUA_TSTRING)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    int _maxConcurrent = LuaAPI.xlua_tointeger(L, 2);
                    
                        var gen_ret = UGFramework.Runtime.YooAssetsLuaBridge.GetDownloadCount( _packageName, _maxConcurrent );
                        LuaAPI.xlua_pushinteger(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& (LuaAPI.lua_isnil(L, 1) || LuaAPI.lua_type(L, 1) == LuaTypes.LUA_TSTRING)) 
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    
                        var gen_ret = UGFramework.Runtime.YooAssetsLuaBridge.GetDownloadCount( _packageName );
                        LuaAPI.xlua_pushinteger(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UGFramework.Runtime.YooAssetsLuaBridge.GetDownloadCount!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_StartDownload_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    int _maxConcurrent = LuaAPI.xlua_tointeger(L, 2);
                    int _failedRetryCount = LuaAPI.xlua_tointeger(L, 3);
                    UGFramework.Runtime.YooAssetsLuaBridge.LuaResourceDownloadProgressCallback _onProgress = translator.GetDelegate<UGFramework.Runtime.YooAssetsLuaBridge.LuaResourceDownloadProgressCallback>(L, 4);
                    UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback _onComplete = translator.GetDelegate<UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback>(L, 5);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.StartDownload( _packageName, _maxConcurrent, _failedRetryCount, _onProgress, _onComplete );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_LoadAsset_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    string _location = LuaAPI.lua_tostring(L, 2);
                    UGFramework.Runtime.YooAssetsLuaBridge.LuaLoadAssetCallback _callback = translator.GetDelegate<UGFramework.Runtime.YooAssetsLuaBridge.LuaLoadAssetCallback>(L, 3);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.LoadAsset( _packageName, _location, _callback );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_InstantiateAsync_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 4&& (LuaAPI.lua_isnil(L, 1) || LuaAPI.lua_type(L, 1) == LuaTypes.LUA_TSTRING)&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)&& translator.Assignable<UGFramework.Runtime.YooAssetsLuaBridge.LuaInstantiateCallback>(L, 3)&& translator.Assignable<UnityEngine.Transform>(L, 4)) 
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    string _location = LuaAPI.lua_tostring(L, 2);
                    UGFramework.Runtime.YooAssetsLuaBridge.LuaInstantiateCallback _callback = translator.GetDelegate<UGFramework.Runtime.YooAssetsLuaBridge.LuaInstantiateCallback>(L, 3);
                    UnityEngine.Transform _parent = (UnityEngine.Transform)translator.GetObject(L, 4, typeof(UnityEngine.Transform));
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.InstantiateAsync( _packageName, _location, _callback, _parent );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 3&& (LuaAPI.lua_isnil(L, 1) || LuaAPI.lua_type(L, 1) == LuaTypes.LUA_TSTRING)&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)&& translator.Assignable<UGFramework.Runtime.YooAssetsLuaBridge.LuaInstantiateCallback>(L, 3)) 
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    string _location = LuaAPI.lua_tostring(L, 2);
                    UGFramework.Runtime.YooAssetsLuaBridge.LuaInstantiateCallback _callback = translator.GetDelegate<UGFramework.Runtime.YooAssetsLuaBridge.LuaInstantiateCallback>(L, 3);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.InstantiateAsync( _packageName, _location, _callback );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UGFramework.Runtime.YooAssetsLuaBridge.InstantiateAsync!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_LoadScene_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    string _location = LuaAPI.lua_tostring(L, 2);
                    bool _additive = LuaAPI.lua_toboolean(L, 3);
                    UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback _callback = translator.GetDelegate<UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback>(L, 4);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.LoadScene( _packageName, _location, _additive, _callback );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_LoadRawFile_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    string _location = LuaAPI.lua_tostring(L, 2);
                    UGFramework.Runtime.YooAssetsLuaBridge.LuaLoadRawFileCallback _callback = translator.GetDelegate<UGFramework.Runtime.YooAssetsLuaBridge.LuaLoadRawFileCallback>(L, 3);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.LoadRawFile( _packageName, _location, _callback );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_LoadAllLuaFiles_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
                
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    string _assetPrefix = LuaAPI.lua_tostring(L, 2);
                    UGFramework.Runtime.YooAssetsLuaBridge.LuaLoadAllLuaFilesProgressCallback _onProgress = translator.GetDelegate<UGFramework.Runtime.YooAssetsLuaBridge.LuaLoadAllLuaFilesProgressCallback>(L, 3);
                    UGFramework.Runtime.YooAssetsLuaBridge.LuaLoadAllLuaFilesCompleteCallback _onComplete = translator.GetDelegate<UGFramework.Runtime.YooAssetsLuaBridge.LuaLoadAllLuaFilesCompleteCallback>(L, 4);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.LoadAllLuaFiles( _packageName, _assetPrefix, _onProgress, _onComplete );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_TryUnloadUnusedAsset_xlua_st_(RealStatePtr L)
        {
		    try {
            
            
            
                
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    string _location = LuaAPI.lua_tostring(L, 2);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.TryUnloadUnusedAsset( _packageName, _location );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_UnloadUnusedAssets_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& (LuaAPI.lua_isnil(L, 1) || LuaAPI.lua_type(L, 1) == LuaTypes.LUA_TSTRING)&& translator.Assignable<UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback>(L, 2)) 
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback _callback = translator.GetDelegate<UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback>(L, 2);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.UnloadUnusedAssets( _packageName, _callback );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 1&& (LuaAPI.lua_isnil(L, 1) || LuaAPI.lua_type(L, 1) == LuaTypes.LUA_TSTRING)) 
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.UnloadUnusedAssets( _packageName );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UGFramework.Runtime.YooAssetsLuaBridge.UnloadUnusedAssets!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_UnloadAllAssets_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& (LuaAPI.lua_isnil(L, 1) || LuaAPI.lua_type(L, 1) == LuaTypes.LUA_TSTRING)&& translator.Assignable<UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback>(L, 2)) 
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback _callback = translator.GetDelegate<UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback>(L, 2);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.UnloadAllAssets( _packageName, _callback );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 1&& (LuaAPI.lua_isnil(L, 1) || LuaAPI.lua_type(L, 1) == LuaTypes.LUA_TSTRING)) 
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.UnloadAllAssets( _packageName );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UGFramework.Runtime.YooAssetsLuaBridge.UnloadAllAssets!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ClearCache_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& (LuaAPI.lua_isnil(L, 1) || LuaAPI.lua_type(L, 1) == LuaTypes.LUA_TSTRING)&& translator.Assignable<UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback>(L, 2)) 
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback _callback = translator.GetDelegate<UGFramework.Runtime.YooAssetsLuaBridge.LuaBoolStringCallback>(L, 2);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.ClearCache( _packageName, _callback );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 1&& (LuaAPI.lua_isnil(L, 1) || LuaAPI.lua_type(L, 1) == LuaTypes.LUA_TSTRING)) 
                {
                    string _packageName = LuaAPI.lua_tostring(L, 1);
                    
                    UGFramework.Runtime.YooAssetsLuaBridge.ClearCache( _packageName );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UGFramework.Runtime.YooAssetsLuaBridge.ClearCache!");
            
        }
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_LuaFileCount(RealStatePtr L)
        {
		    try {
            
			    LuaAPI.xlua_pushinteger(L, UGFramework.Runtime.YooAssetsLuaBridge.LuaFileCount);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        
        
		
		
		
		
    }
}
