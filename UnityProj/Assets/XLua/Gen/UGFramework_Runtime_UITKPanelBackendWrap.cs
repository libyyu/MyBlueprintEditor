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
    public class UGFrameworkRuntimeUITKPanelBackendWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(UGFramework.Runtime.UITKPanelBackend);
			Utils.BeginObjectRegister(type, L, translator, 0, 5, 5, 0);
			
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "SetVisible", _m_SetVisible);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "GetVisible", _m_GetVisible);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "SetSortingOrder", _m_SetSortingOrder);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "GetEventBridge", _m_GetEventBridge);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "Destroy", _m_Destroy);
			
			
			Utils.RegisterFunc(L, Utils.GETTER_IDX, "IsUIToolkit", _g_get_IsUIToolkit);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "RootGameObject", _g_get_RootGameObject);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "IsValid", _g_get_IsValid);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "Document", _g_get_Document);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "RootVisualElement", _g_get_RootVisualElement);
            
			
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 2, 0, 0);
			Utils.RegisterFunc(L, Utils.CLS_IDX, "Create", _m_Create_xlua_st_);
            
			
            
			
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            
			try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
				if(LuaAPI.lua_gettop(L) == 4 && translator.Assignable<UnityEngine.GameObject>(L, 2) && translator.Assignable<UnityEngine.UIElements.UIDocument>(L, 3) && translator.Assignable<UGFramework.Runtime.UITKLuaBridge>(L, 4))
				{
					UnityEngine.GameObject _hostGO = (UnityEngine.GameObject)translator.GetObject(L, 2, typeof(UnityEngine.GameObject));
					UnityEngine.UIElements.UIDocument _doc = (UnityEngine.UIElements.UIDocument)translator.GetObject(L, 3, typeof(UnityEngine.UIElements.UIDocument));
					UGFramework.Runtime.UITKLuaBridge _bridge = (UGFramework.Runtime.UITKLuaBridge)translator.GetObject(L, 4, typeof(UGFramework.Runtime.UITKLuaBridge));
					
					var gen_ret = new UGFramework.Runtime.UITKPanelBackend(_hostGO, _doc, _bridge);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				
			}
			catch(System.Exception gen_e) {
				return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
			}
            return LuaAPI.luaL_error(L, "invalid arguments to UGFramework.Runtime.UITKPanelBackend constructor!");
            
        }
        
		
        
		
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_SetVisible(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UGFramework.Runtime.UITKPanelBackend gen_to_be_invoked = (UGFramework.Runtime.UITKPanelBackend)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    bool _visible = LuaAPI.lua_toboolean(L, 2);
                    
                    gen_to_be_invoked.SetVisible( _visible );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_GetVisible(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UGFramework.Runtime.UITKPanelBackend gen_to_be_invoked = (UGFramework.Runtime.UITKPanelBackend)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                        var gen_ret = gen_to_be_invoked.GetVisible(  );
                        LuaAPI.lua_pushboolean(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_SetSortingOrder(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UGFramework.Runtime.UITKPanelBackend gen_to_be_invoked = (UGFramework.Runtime.UITKPanelBackend)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    int _order = LuaAPI.xlua_tointeger(L, 2);
                    
                    gen_to_be_invoked.SetSortingOrder( _order );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_GetEventBridge(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UGFramework.Runtime.UITKPanelBackend gen_to_be_invoked = (UGFramework.Runtime.UITKPanelBackend)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                        var gen_ret = gen_to_be_invoked.GetEventBridge(  );
                        translator.PushAny(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_Destroy(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UGFramework.Runtime.UITKPanelBackend gen_to_be_invoked = (UGFramework.Runtime.UITKPanelBackend)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                    gen_to_be_invoked.Destroy(  );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_Create_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 5&& translator.Assignable<UnityEngine.UIElements.VisualTreeAsset>(L, 1)&& translator.Assignable<UnityEngine.UIElements.PanelSettings>(L, 2)&& translator.Assignable<UnityEngine.Transform>(L, 3)&& (LuaAPI.lua_isnil(L, 4) || LuaAPI.lua_type(L, 4) == LuaTypes.LUA_TSTRING)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 5)) 
                {
                    UnityEngine.UIElements.VisualTreeAsset _vta = (UnityEngine.UIElements.VisualTreeAsset)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualTreeAsset));
                    UnityEngine.UIElements.PanelSettings _panelSettings = (UnityEngine.UIElements.PanelSettings)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.PanelSettings));
                    UnityEngine.Transform _parentTransform = (UnityEngine.Transform)translator.GetObject(L, 3, typeof(UnityEngine.Transform));
                    string _panelName = LuaAPI.lua_tostring(L, 4);
                    int _sortingOrder = LuaAPI.xlua_tointeger(L, 5);
                    
                        var gen_ret = UGFramework.Runtime.UITKPanelBackend.Create( _vta, _panelSettings, _parentTransform, _panelName, _sortingOrder );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 4&& translator.Assignable<UnityEngine.UIElements.VisualTreeAsset>(L, 1)&& translator.Assignable<UnityEngine.UIElements.PanelSettings>(L, 2)&& translator.Assignable<UnityEngine.Transform>(L, 3)&& (LuaAPI.lua_isnil(L, 4) || LuaAPI.lua_type(L, 4) == LuaTypes.LUA_TSTRING)) 
                {
                    UnityEngine.UIElements.VisualTreeAsset _vta = (UnityEngine.UIElements.VisualTreeAsset)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualTreeAsset));
                    UnityEngine.UIElements.PanelSettings _panelSettings = (UnityEngine.UIElements.PanelSettings)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.PanelSettings));
                    UnityEngine.Transform _parentTransform = (UnityEngine.Transform)translator.GetObject(L, 3, typeof(UnityEngine.Transform));
                    string _panelName = LuaAPI.lua_tostring(L, 4);
                    
                        var gen_ret = UGFramework.Runtime.UITKPanelBackend.Create( _vta, _panelSettings, _parentTransform, _panelName );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UGFramework.Runtime.UITKPanelBackend.Create!");
            
        }
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_IsUIToolkit(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UGFramework.Runtime.UITKPanelBackend gen_to_be_invoked = (UGFramework.Runtime.UITKPanelBackend)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushboolean(L, gen_to_be_invoked.IsUIToolkit);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_RootGameObject(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UGFramework.Runtime.UITKPanelBackend gen_to_be_invoked = (UGFramework.Runtime.UITKPanelBackend)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.RootGameObject);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_IsValid(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UGFramework.Runtime.UITKPanelBackend gen_to_be_invoked = (UGFramework.Runtime.UITKPanelBackend)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushboolean(L, gen_to_be_invoked.IsValid);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_Document(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UGFramework.Runtime.UITKPanelBackend gen_to_be_invoked = (UGFramework.Runtime.UITKPanelBackend)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.Document);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_RootVisualElement(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UGFramework.Runtime.UITKPanelBackend gen_to_be_invoked = (UGFramework.Runtime.UITKPanelBackend)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.RootVisualElement);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        
        
		
		
		
		
    }
}
