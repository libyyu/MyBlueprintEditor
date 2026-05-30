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
    public class UnityEngineUIElementsUIDocumentWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(UnityEngine.UIElements.UIDocument);
			Utils.BeginObjectRegister(type, L, translator, 0, 0, 5, 3);
			
			
			
			Utils.RegisterFunc(L, Utils.GETTER_IDX, "panelSettings", _g_get_panelSettings);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "parentUI", _g_get_parentUI);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "visualTreeAsset", _g_get_visualTreeAsset);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "rootVisualElement", _g_get_rootVisualElement);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "sortingOrder", _g_get_sortingOrder);
            
			Utils.RegisterFunc(L, Utils.SETTER_IDX, "panelSettings", _s_set_panelSettings);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "visualTreeAsset", _s_set_visualTreeAsset);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "sortingOrder", _s_set_sortingOrder);
            
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 1, 0, 0);
			
			
            
			
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            return LuaAPI.luaL_error(L, "UnityEngine.UIElements.UIDocument does not have a constructor!");
        }
        
		
        
		
        
        
        
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_panelSettings(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.UIDocument gen_to_be_invoked = (UnityEngine.UIElements.UIDocument)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.panelSettings);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_parentUI(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.UIDocument gen_to_be_invoked = (UnityEngine.UIElements.UIDocument)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.parentUI);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_visualTreeAsset(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.UIDocument gen_to_be_invoked = (UnityEngine.UIElements.UIDocument)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.visualTreeAsset);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_rootVisualElement(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.UIDocument gen_to_be_invoked = (UnityEngine.UIElements.UIDocument)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.rootVisualElement);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_sortingOrder(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.UIDocument gen_to_be_invoked = (UnityEngine.UIElements.UIDocument)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.sortingOrder);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_panelSettings(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.UIDocument gen_to_be_invoked = (UnityEngine.UIElements.UIDocument)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.panelSettings = (UnityEngine.UIElements.PanelSettings)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.PanelSettings));
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_visualTreeAsset(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.UIDocument gen_to_be_invoked = (UnityEngine.UIElements.UIDocument)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.visualTreeAsset = (UnityEngine.UIElements.VisualTreeAsset)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.VisualTreeAsset));
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_sortingOrder(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.UIDocument gen_to_be_invoked = (UnityEngine.UIElements.UIDocument)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.sortingOrder = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
		
		
		
		
    }
}
