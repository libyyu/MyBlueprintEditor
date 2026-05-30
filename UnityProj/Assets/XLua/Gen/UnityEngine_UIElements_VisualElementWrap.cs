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
using UnityEngine.UIElements;

namespace XLua.CSObjectWrap
{
    using Utils = XLua.Utils;
    public class UnityEngineUIElementsVisualElementWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(UnityEngine.UIElements.VisualElement);
			Utils.BeginObjectRegister(type, L, translator, 0, 40, 31, 9);
			
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "Focus", _m_Focus);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "SendEvent", _m_SendEvent);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "SetEnabled", _m_SetEnabled);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "MarkDirtyRepaint", _m_MarkDirtyRepaint);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "ContainsPoint", _m_ContainsPoint);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "Overlaps", _m_Overlaps);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "ToString", _m_ToString);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "GetClasses", _m_GetClasses);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "ClearClassList", _m_ClearClassList);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "AddToClassList", _m_AddToClassList);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "RemoveFromClassList", _m_RemoveFromClassList);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "ToggleInClassList", _m_ToggleInClassList);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "EnableInClassList", _m_EnableInClassList);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "ClassListContains", _m_ClassListContains);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "FindAncestorUserData", _m_FindAncestorUserData);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "Add", _m_Add);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "Insert", _m_Insert);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "Remove", _m_Remove);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "RemoveAt", _m_RemoveAt);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "Clear", _m_Clear);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "ElementAt", _m_ElementAt);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "IndexOf", _m_IndexOf);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "Children", _m_Children);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "Sort", _m_Sort);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "BringToFront", _m_BringToFront);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "SendToBack", _m_SendToBack);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "PlaceBehind", _m_PlaceBehind);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "PlaceInFront", _m_PlaceInFront);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "RemoveFromHierarchy", _m_RemoveFromHierarchy);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "Contains", _m_Contains);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "FindCommonAncestor", _m_FindCommonAncestor);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "StretchToParentSize", _m_StretchToParentSize);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "StretchToParentWidth", _m_StretchToParentWidth);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "AddManipulator", _m_AddManipulator);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "RemoveManipulator", _m_RemoveManipulator);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "WorldToLocal", _m_WorldToLocal);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "LocalToWorld", _m_LocalToWorld);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "ChangeCoordinatesTo", _m_ChangeCoordinatesTo);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "Q", _m_Q);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "Query", _m_Query);
			
			
			Utils.RegisterFunc(L, Utils.GETTER_IDX, "resolvedStyle", _g_get_resolvedStyle);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "viewDataKey", _g_get_viewDataKey);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "userData", _g_get_userData);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "canGrabFocus", _g_get_canGrabFocus);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "focusController", _g_get_focusController);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "usageHints", _g_get_usageHints);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "transform", _g_get_transform);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "layout", _g_get_layout);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "contentRect", _g_get_contentRect);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "worldBound", _g_get_worldBound);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "localBound", _g_get_localBound);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "worldTransform", _g_get_worldTransform);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "pickingMode", _g_get_pickingMode);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "name", _g_get_name);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "enabledInHierarchy", _g_get_enabledInHierarchy);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "enabledSelf", _g_get_enabledSelf);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "languageDirection", _g_get_languageDirection);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "visible", _g_get_visible);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "generateVisualContent", _g_get_generateVisualContent);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "experimental", _g_get_experimental);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "hierarchy", _g_get_hierarchy);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "parent", _g_get_parent);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "panel", _g_get_panel);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "contentContainer", _g_get_contentContainer);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "visualTreeAssetSource", _g_get_visualTreeAssetSource);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "childCount", _g_get_childCount);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "schedule", _g_get_schedule);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "style", _g_get_style);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "customStyle", _g_get_customStyle);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "styleSheets", _g_get_styleSheets);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "tooltip", _g_get_tooltip);
            
			Utils.RegisterFunc(L, Utils.SETTER_IDX, "viewDataKey", _s_set_viewDataKey);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "userData", _s_set_userData);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "usageHints", _s_set_usageHints);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "pickingMode", _s_set_pickingMode);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "name", _s_set_name);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "languageDirection", _s_set_languageDirection);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "visible", _s_set_visible);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "generateVisualContent", _s_set_generateVisualContent);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "tooltip", _s_set_tooltip);
            
			
			Utils.EndObjectRegister(type, L, translator, __CSIndexer, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 2, 0, 0);
			
			
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "disabledUssClassName", UnityEngine.UIElements.VisualElement.disabledUssClassName);
            
			
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            
			try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
				if(LuaAPI.lua_gettop(L) == 1)
				{
					
					var gen_ret = new UnityEngine.UIElements.VisualElement();
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				
			}
			catch(System.Exception gen_e) {
				return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
			}
            return LuaAPI.luaL_error(L, "invalid arguments to UnityEngine.UIElements.VisualElement constructor!");
            
        }
        
		
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        public static int __CSIndexer(RealStatePtr L)
        {
			try {
			    ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
				
				if (translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2))
				{
					
					UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
					int index = LuaAPI.xlua_tointeger(L, 2);
					LuaAPI.lua_pushboolean(L, true);
					translator.Push(L, gen_to_be_invoked[index]);
					return 2;
				}
				
			}
			catch(System.Exception gen_e) {
				return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
			}
			
            LuaAPI.lua_pushboolean(L, false);
			return 1;
        }
		
        
		
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_Focus(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                    gen_to_be_invoked.Focus(  );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_SendEvent(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    UnityEngine.UIElements.EventBase _e = (UnityEngine.UIElements.EventBase)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.EventBase));
                    
                    gen_to_be_invoked.SendEvent( _e );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_SetEnabled(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    bool _value = LuaAPI.lua_toboolean(L, 2);
                    
                    gen_to_be_invoked.SetEnabled( _value );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_MarkDirtyRepaint(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                    gen_to_be_invoked.MarkDirtyRepaint(  );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ContainsPoint(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    UnityEngine.Vector2 _localPoint;translator.Get(L, 2, out _localPoint);
                    
                        var gen_ret = gen_to_be_invoked.ContainsPoint( _localPoint );
                        LuaAPI.lua_pushboolean(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_Overlaps(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    UnityEngine.Rect _rectangle;translator.Get(L, 2, out _rectangle);
                    
                        var gen_ret = gen_to_be_invoked.Overlaps( _rectangle );
                        LuaAPI.lua_pushboolean(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ToString(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                        var gen_ret = gen_to_be_invoked.ToString(  );
                        LuaAPI.lua_pushstring(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_GetClasses(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                        var gen_ret = gen_to_be_invoked.GetClasses(  );
                        translator.PushAny(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ClearClassList(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                    gen_to_be_invoked.ClearClassList(  );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_AddToClassList(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    string _className = LuaAPI.lua_tostring(L, 2);
                    
                    gen_to_be_invoked.AddToClassList( _className );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_RemoveFromClassList(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    string _className = LuaAPI.lua_tostring(L, 2);
                    
                    gen_to_be_invoked.RemoveFromClassList( _className );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ToggleInClassList(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    string _className = LuaAPI.lua_tostring(L, 2);
                    
                    gen_to_be_invoked.ToggleInClassList( _className );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_EnableInClassList(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    string _className = LuaAPI.lua_tostring(L, 2);
                    bool _enable = LuaAPI.lua_toboolean(L, 3);
                    
                    gen_to_be_invoked.EnableInClassList( _className, _enable );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ClassListContains(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    string _cls = LuaAPI.lua_tostring(L, 2);
                    
                        var gen_ret = gen_to_be_invoked.ClassListContains( _cls );
                        LuaAPI.lua_pushboolean(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_FindAncestorUserData(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                        var gen_ret = gen_to_be_invoked.FindAncestorUserData(  );
                        translator.PushAny(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_Add(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    UnityEngine.UIElements.VisualElement _child = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.VisualElement));
                    
                    gen_to_be_invoked.Add( _child );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_Insert(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    int _index = LuaAPI.xlua_tointeger(L, 2);
                    UnityEngine.UIElements.VisualElement _element = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 3, typeof(UnityEngine.UIElements.VisualElement));
                    
                    gen_to_be_invoked.Insert( _index, _element );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_Remove(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    UnityEngine.UIElements.VisualElement _element = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.VisualElement));
                    
                    gen_to_be_invoked.Remove( _element );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_RemoveAt(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    int _index = LuaAPI.xlua_tointeger(L, 2);
                    
                    gen_to_be_invoked.RemoveAt( _index );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_Clear(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                    gen_to_be_invoked.Clear(  );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ElementAt(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    int _index = LuaAPI.xlua_tointeger(L, 2);
                    
                        var gen_ret = gen_to_be_invoked.ElementAt( _index );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_IndexOf(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    UnityEngine.UIElements.VisualElement _element = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = gen_to_be_invoked.IndexOf( _element );
                        LuaAPI.xlua_pushinteger(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_Children(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                        var gen_ret = gen_to_be_invoked.Children(  );
                        translator.PushAny(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_Sort(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    System.Comparison<UnityEngine.UIElements.VisualElement> _comp = translator.GetDelegate<System.Comparison<UnityEngine.UIElements.VisualElement>>(L, 2);
                    
                    gen_to_be_invoked.Sort( _comp );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_BringToFront(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                    gen_to_be_invoked.BringToFront(  );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_SendToBack(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                    gen_to_be_invoked.SendToBack(  );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_PlaceBehind(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    UnityEngine.UIElements.VisualElement _sibling = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.VisualElement));
                    
                    gen_to_be_invoked.PlaceBehind( _sibling );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_PlaceInFront(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    UnityEngine.UIElements.VisualElement _sibling = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.VisualElement));
                    
                    gen_to_be_invoked.PlaceInFront( _sibling );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_RemoveFromHierarchy(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                    gen_to_be_invoked.RemoveFromHierarchy(  );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_Contains(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    UnityEngine.UIElements.VisualElement _child = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = gen_to_be_invoked.Contains( _child );
                        LuaAPI.lua_pushboolean(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_FindCommonAncestor(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    UnityEngine.UIElements.VisualElement _other = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = gen_to_be_invoked.FindCommonAncestor( _other );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_StretchToParentSize(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                    gen_to_be_invoked.StretchToParentSize(  );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_StretchToParentWidth(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                    gen_to_be_invoked.StretchToParentWidth(  );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_AddManipulator(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    UnityEngine.UIElements.IManipulator _manipulator = (UnityEngine.UIElements.IManipulator)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.IManipulator));
                    
                    gen_to_be_invoked.AddManipulator( _manipulator );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_RemoveManipulator(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    UnityEngine.UIElements.IManipulator _manipulator = (UnityEngine.UIElements.IManipulator)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.IManipulator));
                    
                    gen_to_be_invoked.RemoveManipulator( _manipulator );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_WorldToLocal(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.Vector2>(L, 2)) 
                {
                    UnityEngine.Vector2 _p;translator.Get(L, 2, out _p);
                    
                        var gen_ret = gen_to_be_invoked.WorldToLocal( _p );
                        translator.PushUnityEngineVector2(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.Rect>(L, 2)) 
                {
                    UnityEngine.Rect _r;translator.Get(L, 2, out _r);
                    
                        var gen_ret = gen_to_be_invoked.WorldToLocal( _r );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UnityEngine.UIElements.VisualElement.WorldToLocal!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_LocalToWorld(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.Vector2>(L, 2)) 
                {
                    UnityEngine.Vector2 _p;translator.Get(L, 2, out _p);
                    
                        var gen_ret = gen_to_be_invoked.LocalToWorld( _p );
                        translator.PushUnityEngineVector2(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.Rect>(L, 2)) 
                {
                    UnityEngine.Rect _r;translator.Get(L, 2, out _r);
                    
                        var gen_ret = gen_to_be_invoked.LocalToWorld( _r );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UnityEngine.UIElements.VisualElement.LocalToWorld!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ChangeCoordinatesTo(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 3&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 2)&& translator.Assignable<UnityEngine.Vector2>(L, 3)) 
                {
                    UnityEngine.UIElements.VisualElement _dest = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.Vector2 _point;translator.Get(L, 3, out _point);
                    
                        var gen_ret = gen_to_be_invoked.ChangeCoordinatesTo( _dest, _point );
                        translator.PushUnityEngineVector2(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 3&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 2)&& translator.Assignable<UnityEngine.Rect>(L, 3)) 
                {
                    UnityEngine.UIElements.VisualElement _dest = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 2, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.Rect _rect;translator.Get(L, 3, out _rect);
                    
                        var gen_ret = gen_to_be_invoked.ChangeCoordinatesTo( _dest, _rect );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UnityEngine.UIElements.VisualElement.ChangeCoordinatesTo!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_Q(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count >= 2&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)&& (LuaTypes.LUA_TNONE == LuaAPI.lua_type(L, 3) || (LuaAPI.lua_isnil(L, 3) || LuaAPI.lua_type(L, 3) == LuaTypes.LUA_TSTRING))) 
                {
                    string _name = LuaAPI.lua_tostring(L, 2);
                    string[] _classes = translator.GetParams<string>(L, 3);
                    
                        var gen_ret = gen_to_be_invoked.Q( _name, _classes );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count >= 1&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)) 
                {
                    string _name = LuaAPI.lua_tostring(L, 2);
                    
                        var gen_ret = gen_to_be_invoked.Q( _name );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count >= 0) 
                {
                    
                        var gen_ret = gen_to_be_invoked.Q(  );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 3&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)&& (LuaAPI.lua_isnil(L, 3) || LuaAPI.lua_type(L, 3) == LuaTypes.LUA_TSTRING)) 
                {
                    string _name = LuaAPI.lua_tostring(L, 2);
                    string _className = LuaAPI.lua_tostring(L, 3);
                    
                        var gen_ret = gen_to_be_invoked.Q( _name, _className );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 2&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)) 
                {
                    string _name = LuaAPI.lua_tostring(L, 2);
                    
                        var gen_ret = gen_to_be_invoked.Q( _name );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1) 
                {
                    
                        var gen_ret = gen_to_be_invoked.Q(  );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UnityEngine.UIElements.VisualElement.Q!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_Query(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1) 
                {
                    
                        var gen_ret = gen_to_be_invoked.Query(  );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count >= 2&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)&& (LuaTypes.LUA_TNONE == LuaAPI.lua_type(L, 3) || (LuaAPI.lua_isnil(L, 3) || LuaAPI.lua_type(L, 3) == LuaTypes.LUA_TSTRING))) 
                {
                    string _name = LuaAPI.lua_tostring(L, 2);
                    string[] _classes = translator.GetParams<string>(L, 3);
                    
                        var gen_ret = gen_to_be_invoked.Query( _name, _classes );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count >= 1&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)) 
                {
                    string _name = LuaAPI.lua_tostring(L, 2);
                    
                        var gen_ret = gen_to_be_invoked.Query( _name );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count >= 0) 
                {
                    
                        var gen_ret = gen_to_be_invoked.Query(  );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 3&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)&& (LuaAPI.lua_isnil(L, 3) || LuaAPI.lua_type(L, 3) == LuaTypes.LUA_TSTRING)) 
                {
                    string _name = LuaAPI.lua_tostring(L, 2);
                    string _className = LuaAPI.lua_tostring(L, 3);
                    
                        var gen_ret = gen_to_be_invoked.Query( _name, _className );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 2&& (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING)) 
                {
                    string _name = LuaAPI.lua_tostring(L, 2);
                    
                        var gen_ret = gen_to_be_invoked.Query( _name );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1) 
                {
                    
                        var gen_ret = gen_to_be_invoked.Query(  );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UnityEngine.UIElements.VisualElement.Query!");
            
        }
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_resolvedStyle(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.PushAny(L, gen_to_be_invoked.resolvedStyle);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_viewDataKey(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushstring(L, gen_to_be_invoked.viewDataKey);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_userData(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.PushAny(L, gen_to_be_invoked.userData);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_canGrabFocus(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushboolean(L, gen_to_be_invoked.canGrabFocus);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_focusController(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.focusController);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_usageHints(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.usageHints);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_transform(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.PushAny(L, gen_to_be_invoked.transform);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_layout(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.layout);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_contentRect(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.contentRect);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_worldBound(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.worldBound);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_localBound(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.localBound);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_worldTransform(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.worldTransform);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_pickingMode(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.pickingMode);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_name(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushstring(L, gen_to_be_invoked.name);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_enabledInHierarchy(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushboolean(L, gen_to_be_invoked.enabledInHierarchy);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_enabledSelf(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushboolean(L, gen_to_be_invoked.enabledSelf);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_languageDirection(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.languageDirection);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_visible(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushboolean(L, gen_to_be_invoked.visible);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_generateVisualContent(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.generateVisualContent);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_experimental(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.PushAny(L, gen_to_be_invoked.experimental);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_hierarchy(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.hierarchy);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_parent(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.parent);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_panel(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.PushAny(L, gen_to_be_invoked.panel);
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
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.contentContainer);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_visualTreeAssetSource(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.visualTreeAssetSource);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_childCount(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                LuaAPI.xlua_pushinteger(L, gen_to_be_invoked.childCount);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_schedule(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.PushAny(L, gen_to_be_invoked.schedule);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_style(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.PushAny(L, gen_to_be_invoked.style);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_customStyle(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.PushAny(L, gen_to_be_invoked.customStyle);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_styleSheets(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.styleSheets);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_tooltip(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushstring(L, gen_to_be_invoked.tooltip);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_viewDataKey(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.viewDataKey = LuaAPI.lua_tostring(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_userData(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.userData = translator.GetObject(L, 2, typeof(object));
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_usageHints(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.UsageHints gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.usageHints = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_pickingMode(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.PickingMode gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.pickingMode = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_name(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.name = LuaAPI.lua_tostring(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_languageDirection(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.LanguageDirection gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.languageDirection = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_visible(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.visible = LuaAPI.lua_toboolean(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_generateVisualContent(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.generateVisualContent = translator.GetDelegate<System.Action<UnityEngine.UIElements.MeshGenerationContext>>(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_tooltip(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.VisualElement gen_to_be_invoked = (UnityEngine.UIElements.VisualElement)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.tooltip = LuaAPI.lua_tostring(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
		
		
		
		
    }
}
