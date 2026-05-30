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
    public class UnityEngineUIElementsPanelSettingsWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(UnityEngine.UIElements.PanelSettings);
			Utils.BeginObjectRegister(type, L, translator, 0, 1, 18, 17);
			
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "SetScreenToPanelSpaceFunction", _m_SetScreenToPanelSpaceFunction);
			
			
			Utils.RegisterFunc(L, Utils.GETTER_IDX, "themeStyleSheet", _g_get_themeStyleSheet);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "targetTexture", _g_get_targetTexture);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "scaleMode", _g_get_scaleMode);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "referenceSpritePixelsPerUnit", _g_get_referenceSpritePixelsPerUnit);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "scale", _g_get_scale);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "referenceDpi", _g_get_referenceDpi);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "fallbackDpi", _g_get_fallbackDpi);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "referenceResolution", _g_get_referenceResolution);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "screenMatchMode", _g_get_screenMatchMode);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "match", _g_get_match);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "sortingOrder", _g_get_sortingOrder);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "targetDisplay", _g_get_targetDisplay);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "clearDepthStencil", _g_get_clearDepthStencil);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "depthClearValue", _g_get_depthClearValue);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "clearColor", _g_get_clearColor);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "colorClearValue", _g_get_colorClearValue);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "dynamicAtlasSettings", _g_get_dynamicAtlasSettings);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "textSettings", _g_get_textSettings);
            
			Utils.RegisterFunc(L, Utils.SETTER_IDX, "themeStyleSheet", _s_set_themeStyleSheet);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "targetTexture", _s_set_targetTexture);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "scaleMode", _s_set_scaleMode);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "referenceSpritePixelsPerUnit", _s_set_referenceSpritePixelsPerUnit);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "scale", _s_set_scale);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "referenceDpi", _s_set_referenceDpi);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "fallbackDpi", _s_set_fallbackDpi);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "referenceResolution", _s_set_referenceResolution);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "screenMatchMode", _s_set_screenMatchMode);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "match", _s_set_match);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "sortingOrder", _s_set_sortingOrder);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "targetDisplay", _s_set_targetDisplay);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "clearDepthStencil", _s_set_clearDepthStencil);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "clearColor", _s_set_clearColor);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "colorClearValue", _s_set_colorClearValue);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "dynamicAtlasSettings", _s_set_dynamicAtlasSettings);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "textSettings", _s_set_textSettings);
            
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 1, 0, 0);
			
			
            
			
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            return LuaAPI.luaL_error(L, "UnityEngine.UIElements.PanelSettings does not have a constructor!");
        }
        
		
        
		
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_SetScreenToPanelSpaceFunction(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    System.Func<UnityEngine.Vector2, UnityEngine.Vector2> _screentoPanelSpaceFunction = translator.GetDelegate<System.Func<UnityEngine.Vector2, UnityEngine.Vector2>>(L, 2);
                    
                    gen_to_be_invoked.SetScreenToPanelSpaceFunction( _screentoPanelSpaceFunction );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_themeStyleSheet(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.themeStyleSheet);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_targetTexture(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.targetTexture);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_scaleMode(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.scaleMode);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_referenceSpritePixelsPerUnit(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.referenceSpritePixelsPerUnit);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_scale(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.scale);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_referenceDpi(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.referenceDpi);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_fallbackDpi(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.fallbackDpi);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_referenceResolution(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.referenceResolution);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_screenMatchMode(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.screenMatchMode);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_match(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.match);
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
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.sortingOrder);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_targetDisplay(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                LuaAPI.xlua_pushinteger(L, gen_to_be_invoked.targetDisplay);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_clearDepthStencil(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushboolean(L, gen_to_be_invoked.clearDepthStencil);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_depthClearValue(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.depthClearValue);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_clearColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushboolean(L, gen_to_be_invoked.clearColor);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_colorClearValue(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                translator.PushUnityEngineColor(L, gen_to_be_invoked.colorClearValue);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_dynamicAtlasSettings(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.dynamicAtlasSettings);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_textSettings(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.textSettings);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_themeStyleSheet(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.themeStyleSheet = (UnityEngine.UIElements.ThemeStyleSheet)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.ThemeStyleSheet));
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_targetTexture(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.targetTexture = (UnityEngine.RenderTexture)translator.GetObject(L, 2, typeof(UnityEngine.RenderTexture));
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_scaleMode(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.PanelScaleMode gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.scaleMode = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_referenceSpritePixelsPerUnit(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.referenceSpritePixelsPerUnit = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_scale(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.scale = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_referenceDpi(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.referenceDpi = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_fallbackDpi(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.fallbackDpi = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_referenceResolution(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                UnityEngine.Vector2Int gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.referenceResolution = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_screenMatchMode(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.PanelScreenMatchMode gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.screenMatchMode = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_match(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.match = (float)LuaAPI.lua_tonumber(L, 2);
            
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
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.sortingOrder = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_targetDisplay(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.targetDisplay = LuaAPI.xlua_tointeger(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_clearDepthStencil(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.clearDepthStencil = LuaAPI.lua_toboolean(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_clearColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.clearColor = LuaAPI.lua_toboolean(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_colorClearValue(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                UnityEngine.Color gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.colorClearValue = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_dynamicAtlasSettings(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.dynamicAtlasSettings = (UnityEngine.UIElements.DynamicAtlasSettings)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.DynamicAtlasSettings));
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_textSettings(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.PanelSettings gen_to_be_invoked = (UnityEngine.UIElements.PanelSettings)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.textSettings = (UnityEngine.UIElements.PanelTextSettings)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.PanelTextSettings));
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
		
		
		
		
    }
}
