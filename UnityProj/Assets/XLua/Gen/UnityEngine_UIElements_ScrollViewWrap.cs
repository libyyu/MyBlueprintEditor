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
    public class UnityEngineUIElementsScrollViewWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(UnityEngine.UIElements.ScrollView);
			Utils.BeginObjectRegister(type, L, translator, 0, 1, 16, 12);
			
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "ScrollTo", _m_ScrollTo);
			
			
			Utils.RegisterFunc(L, Utils.GETTER_IDX, "horizontalScrollerVisibility", _g_get_horizontalScrollerVisibility);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "verticalScrollerVisibility", _g_get_verticalScrollerVisibility);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "scrollOffset", _g_get_scrollOffset);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "horizontalPageSize", _g_get_horizontalPageSize);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "verticalPageSize", _g_get_verticalPageSize);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "mouseWheelScrollSize", _g_get_mouseWheelScrollSize);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "scrollDecelerationRate", _g_get_scrollDecelerationRate);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "elasticity", _g_get_elasticity);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "touchScrollBehavior", _g_get_touchScrollBehavior);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "nestedInteractionKind", _g_get_nestedInteractionKind);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "elasticAnimationIntervalMs", _g_get_elasticAnimationIntervalMs);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "contentViewport", _g_get_contentViewport);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "horizontalScroller", _g_get_horizontalScroller);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "verticalScroller", _g_get_verticalScroller);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "contentContainer", _g_get_contentContainer);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "mode", _g_get_mode);
            
			Utils.RegisterFunc(L, Utils.SETTER_IDX, "horizontalScrollerVisibility", _s_set_horizontalScrollerVisibility);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "verticalScrollerVisibility", _s_set_verticalScrollerVisibility);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "scrollOffset", _s_set_scrollOffset);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "horizontalPageSize", _s_set_horizontalPageSize);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "verticalPageSize", _s_set_verticalPageSize);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "mouseWheelScrollSize", _s_set_mouseWheelScrollSize);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "scrollDecelerationRate", _s_set_scrollDecelerationRate);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "elasticity", _s_set_elasticity);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "touchScrollBehavior", _s_set_touchScrollBehavior);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "nestedInteractionKind", _s_set_nestedInteractionKind);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "elasticAnimationIntervalMs", _s_set_elasticAnimationIntervalMs);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "mode", _s_set_mode);
            
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 17, 0, 0);
			
			
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "ussClassName", UnityEngine.UIElements.ScrollView.ussClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "viewportUssClassName", UnityEngine.UIElements.ScrollView.viewportUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "horizontalVariantViewportUssClassName", UnityEngine.UIElements.ScrollView.horizontalVariantViewportUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "verticalVariantViewportUssClassName", UnityEngine.UIElements.ScrollView.verticalVariantViewportUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "verticalHorizontalVariantViewportUssClassName", UnityEngine.UIElements.ScrollView.verticalHorizontalVariantViewportUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "contentAndVerticalScrollUssClassName", UnityEngine.UIElements.ScrollView.contentAndVerticalScrollUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "contentUssClassName", UnityEngine.UIElements.ScrollView.contentUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "horizontalVariantContentUssClassName", UnityEngine.UIElements.ScrollView.horizontalVariantContentUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "verticalVariantContentUssClassName", UnityEngine.UIElements.ScrollView.verticalVariantContentUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "verticalHorizontalVariantContentUssClassName", UnityEngine.UIElements.ScrollView.verticalHorizontalVariantContentUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "hScrollerUssClassName", UnityEngine.UIElements.ScrollView.hScrollerUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "vScrollerUssClassName", UnityEngine.UIElements.ScrollView.vScrollerUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "horizontalVariantUssClassName", UnityEngine.UIElements.ScrollView.horizontalVariantUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "verticalVariantUssClassName", UnityEngine.UIElements.ScrollView.verticalVariantUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "verticalHorizontalVariantUssClassName", UnityEngine.UIElements.ScrollView.verticalHorizontalVariantUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "scrollVariantUssClassName", UnityEngine.UIElements.ScrollView.scrollVariantUssClassName);
            
			
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            
			try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
				if(LuaAPI.lua_gettop(L) == 1)
				{
					
					var gen_ret = new UnityEngine.UIElements.ScrollView();
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 2 && translator.Assignable<UnityEngine.UIElements.ScrollViewMode>(L, 2))
				{
					UnityEngine.UIElements.ScrollViewMode _scrollViewMode;translator.Get(L, 2, out _scrollViewMode);
					
					var gen_ret = new UnityEngine.UIElements.ScrollView(_scrollViewMode);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				
			}
			catch(System.Exception gen_e) {
				return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
			}
            return LuaAPI.luaL_error(L, "invalid arguments to UnityEngine.UIElements.ScrollView constructor!");
            
        }
        
		
        
		
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ScrollTo(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    UnityEngine.UIElements.VisualElement _child = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.VisualElement));
                    
                    gen_to_be_invoked.ScrollTo( _child );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_horizontalScrollerVisibility(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.horizontalScrollerVisibility);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_verticalScrollerVisibility(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.verticalScrollerVisibility);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_scrollOffset(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                translator.PushUnityEngineVector2(L, gen_to_be_invoked.scrollOffset);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_horizontalPageSize(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.horizontalPageSize);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_verticalPageSize(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.verticalPageSize);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_mouseWheelScrollSize(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.mouseWheelScrollSize);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_scrollDecelerationRate(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.scrollDecelerationRate);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_elasticity(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.elasticity);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_touchScrollBehavior(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.touchScrollBehavior);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_nestedInteractionKind(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.nestedInteractionKind);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_elasticAnimationIntervalMs(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushint64(L, gen_to_be_invoked.elasticAnimationIntervalMs);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_contentViewport(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.contentViewport);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_horizontalScroller(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.horizontalScroller);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_verticalScroller(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.verticalScroller);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_contentContainer(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.contentContainer);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_mode(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                translator.PushUnityEngineUIElementsScrollViewMode(L, gen_to_be_invoked.mode);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_horizontalScrollerVisibility(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.ScrollerVisibility gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.horizontalScrollerVisibility = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_verticalScrollerVisibility(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.ScrollerVisibility gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.verticalScrollerVisibility = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_scrollOffset(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                UnityEngine.Vector2 gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.scrollOffset = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_horizontalPageSize(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.horizontalPageSize = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_verticalPageSize(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.verticalPageSize = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_mouseWheelScrollSize(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.mouseWheelScrollSize = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_scrollDecelerationRate(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.scrollDecelerationRate = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_elasticity(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.elasticity = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_touchScrollBehavior(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.ScrollView.TouchScrollBehavior gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.touchScrollBehavior = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_nestedInteractionKind(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.ScrollView.NestedInteractionKind gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.nestedInteractionKind = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_elasticAnimationIntervalMs(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.elasticAnimationIntervalMs = LuaAPI.lua_toint64(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_mode(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.ScrollView gen_to_be_invoked = (UnityEngine.UIElements.ScrollView)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.ScrollViewMode gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.mode = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
		
		
		
		
    }
}
