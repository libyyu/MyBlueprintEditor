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
    public class UnityEngineUIElementsIStyleWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(UnityEngine.UIElements.IStyle);
			Utils.BeginObjectRegister(type, L, translator, 0, 0, 81, 81);
			
			
			
			Utils.RegisterFunc(L, Utils.GETTER_IDX, "alignContent", _g_get_alignContent);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "alignItems", _g_get_alignItems);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "alignSelf", _g_get_alignSelf);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "backgroundColor", _g_get_backgroundColor);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "backgroundImage", _g_get_backgroundImage);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "backgroundPositionX", _g_get_backgroundPositionX);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "backgroundPositionY", _g_get_backgroundPositionY);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "backgroundRepeat", _g_get_backgroundRepeat);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "backgroundSize", _g_get_backgroundSize);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "borderBottomColor", _g_get_borderBottomColor);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "borderBottomLeftRadius", _g_get_borderBottomLeftRadius);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "borderBottomRightRadius", _g_get_borderBottomRightRadius);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "borderBottomWidth", _g_get_borderBottomWidth);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "borderLeftColor", _g_get_borderLeftColor);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "borderLeftWidth", _g_get_borderLeftWidth);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "borderRightColor", _g_get_borderRightColor);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "borderRightWidth", _g_get_borderRightWidth);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "borderTopColor", _g_get_borderTopColor);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "borderTopLeftRadius", _g_get_borderTopLeftRadius);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "borderTopRightRadius", _g_get_borderTopRightRadius);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "borderTopWidth", _g_get_borderTopWidth);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "bottom", _g_get_bottom);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "color", _g_get_color);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "cursor", _g_get_cursor);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "display", _g_get_display);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "flexBasis", _g_get_flexBasis);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "flexDirection", _g_get_flexDirection);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "flexGrow", _g_get_flexGrow);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "flexShrink", _g_get_flexShrink);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "flexWrap", _g_get_flexWrap);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "fontSize", _g_get_fontSize);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "height", _g_get_height);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "justifyContent", _g_get_justifyContent);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "left", _g_get_left);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "letterSpacing", _g_get_letterSpacing);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "marginBottom", _g_get_marginBottom);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "marginLeft", _g_get_marginLeft);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "marginRight", _g_get_marginRight);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "marginTop", _g_get_marginTop);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "maxHeight", _g_get_maxHeight);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "maxWidth", _g_get_maxWidth);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "minHeight", _g_get_minHeight);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "minWidth", _g_get_minWidth);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "opacity", _g_get_opacity);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "overflow", _g_get_overflow);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "paddingBottom", _g_get_paddingBottom);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "paddingLeft", _g_get_paddingLeft);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "paddingRight", _g_get_paddingRight);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "paddingTop", _g_get_paddingTop);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "position", _g_get_position);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "right", _g_get_right);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "rotate", _g_get_rotate);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "scale", _g_get_scale);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "textOverflow", _g_get_textOverflow);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "textShadow", _g_get_textShadow);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "top", _g_get_top);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "transformOrigin", _g_get_transformOrigin);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "transitionDelay", _g_get_transitionDelay);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "transitionDuration", _g_get_transitionDuration);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "transitionProperty", _g_get_transitionProperty);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "transitionTimingFunction", _g_get_transitionTimingFunction);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "translate", _g_get_translate);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "unityBackgroundImageTintColor", _g_get_unityBackgroundImageTintColor);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "unityFont", _g_get_unityFont);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "unityFontDefinition", _g_get_unityFontDefinition);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "unityFontStyleAndWeight", _g_get_unityFontStyleAndWeight);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "unityOverflowClipBox", _g_get_unityOverflowClipBox);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "unityParagraphSpacing", _g_get_unityParagraphSpacing);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "unitySliceBottom", _g_get_unitySliceBottom);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "unitySliceLeft", _g_get_unitySliceLeft);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "unitySliceRight", _g_get_unitySliceRight);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "unitySliceScale", _g_get_unitySliceScale);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "unitySliceTop", _g_get_unitySliceTop);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "unityTextAlign", _g_get_unityTextAlign);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "unityTextOutlineColor", _g_get_unityTextOutlineColor);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "unityTextOutlineWidth", _g_get_unityTextOutlineWidth);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "unityTextOverflowPosition", _g_get_unityTextOverflowPosition);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "visibility", _g_get_visibility);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "whiteSpace", _g_get_whiteSpace);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "width", _g_get_width);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "wordSpacing", _g_get_wordSpacing);
            
			Utils.RegisterFunc(L, Utils.SETTER_IDX, "alignContent", _s_set_alignContent);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "alignItems", _s_set_alignItems);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "alignSelf", _s_set_alignSelf);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "backgroundColor", _s_set_backgroundColor);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "backgroundImage", _s_set_backgroundImage);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "backgroundPositionX", _s_set_backgroundPositionX);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "backgroundPositionY", _s_set_backgroundPositionY);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "backgroundRepeat", _s_set_backgroundRepeat);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "backgroundSize", _s_set_backgroundSize);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "borderBottomColor", _s_set_borderBottomColor);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "borderBottomLeftRadius", _s_set_borderBottomLeftRadius);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "borderBottomRightRadius", _s_set_borderBottomRightRadius);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "borderBottomWidth", _s_set_borderBottomWidth);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "borderLeftColor", _s_set_borderLeftColor);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "borderLeftWidth", _s_set_borderLeftWidth);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "borderRightColor", _s_set_borderRightColor);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "borderRightWidth", _s_set_borderRightWidth);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "borderTopColor", _s_set_borderTopColor);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "borderTopLeftRadius", _s_set_borderTopLeftRadius);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "borderTopRightRadius", _s_set_borderTopRightRadius);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "borderTopWidth", _s_set_borderTopWidth);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "bottom", _s_set_bottom);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "color", _s_set_color);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "cursor", _s_set_cursor);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "display", _s_set_display);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "flexBasis", _s_set_flexBasis);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "flexDirection", _s_set_flexDirection);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "flexGrow", _s_set_flexGrow);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "flexShrink", _s_set_flexShrink);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "flexWrap", _s_set_flexWrap);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "fontSize", _s_set_fontSize);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "height", _s_set_height);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "justifyContent", _s_set_justifyContent);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "left", _s_set_left);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "letterSpacing", _s_set_letterSpacing);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "marginBottom", _s_set_marginBottom);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "marginLeft", _s_set_marginLeft);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "marginRight", _s_set_marginRight);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "marginTop", _s_set_marginTop);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "maxHeight", _s_set_maxHeight);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "maxWidth", _s_set_maxWidth);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "minHeight", _s_set_minHeight);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "minWidth", _s_set_minWidth);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "opacity", _s_set_opacity);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "overflow", _s_set_overflow);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "paddingBottom", _s_set_paddingBottom);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "paddingLeft", _s_set_paddingLeft);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "paddingRight", _s_set_paddingRight);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "paddingTop", _s_set_paddingTop);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "position", _s_set_position);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "right", _s_set_right);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "rotate", _s_set_rotate);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "scale", _s_set_scale);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "textOverflow", _s_set_textOverflow);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "textShadow", _s_set_textShadow);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "top", _s_set_top);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "transformOrigin", _s_set_transformOrigin);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "transitionDelay", _s_set_transitionDelay);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "transitionDuration", _s_set_transitionDuration);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "transitionProperty", _s_set_transitionProperty);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "transitionTimingFunction", _s_set_transitionTimingFunction);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "translate", _s_set_translate);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "unityBackgroundImageTintColor", _s_set_unityBackgroundImageTintColor);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "unityFont", _s_set_unityFont);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "unityFontDefinition", _s_set_unityFontDefinition);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "unityFontStyleAndWeight", _s_set_unityFontStyleAndWeight);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "unityOverflowClipBox", _s_set_unityOverflowClipBox);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "unityParagraphSpacing", _s_set_unityParagraphSpacing);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "unitySliceBottom", _s_set_unitySliceBottom);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "unitySliceLeft", _s_set_unitySliceLeft);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "unitySliceRight", _s_set_unitySliceRight);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "unitySliceScale", _s_set_unitySliceScale);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "unitySliceTop", _s_set_unitySliceTop);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "unityTextAlign", _s_set_unityTextAlign);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "unityTextOutlineColor", _s_set_unityTextOutlineColor);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "unityTextOutlineWidth", _s_set_unityTextOutlineWidth);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "unityTextOverflowPosition", _s_set_unityTextOverflowPosition);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "visibility", _s_set_visibility);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "whiteSpace", _s_set_whiteSpace);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "width", _s_set_width);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "wordSpacing", _s_set_wordSpacing);
            
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 1, 0, 0);
			
			
            
			
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            return LuaAPI.luaL_error(L, "UnityEngine.UIElements.IStyle does not have a constructor!");
        }
        
		
        
		
        
        
        
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_alignContent(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.alignContent);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_alignItems(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.alignItems);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_alignSelf(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.alignSelf);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_backgroundColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.backgroundColor);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_backgroundImage(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.backgroundImage);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_backgroundPositionX(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.backgroundPositionX);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_backgroundPositionY(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.backgroundPositionY);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_backgroundRepeat(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.backgroundRepeat);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_backgroundSize(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.backgroundSize);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_borderBottomColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.borderBottomColor);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_borderBottomLeftRadius(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.borderBottomLeftRadius);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_borderBottomRightRadius(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.borderBottomRightRadius);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_borderBottomWidth(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.borderBottomWidth);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_borderLeftColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.borderLeftColor);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_borderLeftWidth(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.borderLeftWidth);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_borderRightColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.borderRightColor);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_borderRightWidth(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.borderRightWidth);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_borderTopColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.borderTopColor);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_borderTopLeftRadius(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.borderTopLeftRadius);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_borderTopRightRadius(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.borderTopRightRadius);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_borderTopWidth(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.borderTopWidth);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_bottom(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.bottom);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_color(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.color);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_cursor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.cursor);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_display(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.display);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_flexBasis(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.flexBasis);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_flexDirection(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.flexDirection);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_flexGrow(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.flexGrow);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_flexShrink(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.flexShrink);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_flexWrap(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.flexWrap);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_fontSize(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.fontSize);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_height(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.height);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_justifyContent(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.justifyContent);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_left(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.left);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_letterSpacing(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.letterSpacing);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_marginBottom(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.marginBottom);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_marginLeft(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.marginLeft);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_marginRight(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.marginRight);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_marginTop(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.marginTop);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_maxHeight(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.maxHeight);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_maxWidth(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.maxWidth);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_minHeight(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.minHeight);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_minWidth(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.minWidth);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_opacity(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.opacity);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_overflow(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.overflow);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_paddingBottom(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.paddingBottom);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_paddingLeft(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.paddingLeft);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_paddingRight(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.paddingRight);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_paddingTop(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.paddingTop);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_position(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.position);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_right(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.right);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_rotate(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.rotate);
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
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.scale);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_textOverflow(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.textOverflow);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_textShadow(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.textShadow);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_top(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.top);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_transformOrigin(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.transformOrigin);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_transitionDelay(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.transitionDelay);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_transitionDuration(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.transitionDuration);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_transitionProperty(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.transitionProperty);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_transitionTimingFunction(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.transitionTimingFunction);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_translate(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.translate);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_unityBackgroundImageTintColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.unityBackgroundImageTintColor);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_unityFont(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.unityFont);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_unityFontDefinition(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.unityFontDefinition);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_unityFontStyleAndWeight(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.unityFontStyleAndWeight);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_unityOverflowClipBox(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.unityOverflowClipBox);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_unityParagraphSpacing(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.unityParagraphSpacing);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_unitySliceBottom(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.unitySliceBottom);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_unitySliceLeft(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.unitySliceLeft);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_unitySliceRight(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.unitySliceRight);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_unitySliceScale(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.unitySliceScale);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_unitySliceTop(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.unitySliceTop);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_unityTextAlign(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.unityTextAlign);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_unityTextOutlineColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.unityTextOutlineColor);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_unityTextOutlineWidth(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.unityTextOutlineWidth);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_unityTextOverflowPosition(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.unityTextOverflowPosition);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_visibility(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.visibility);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_whiteSpace(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.whiteSpace);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_width(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.width);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_wordSpacing(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.wordSpacing);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_alignContent(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Align> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.alignContent = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_alignItems(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Align> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.alignItems = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_alignSelf(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Align> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.alignSelf = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_backgroundColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleColor gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.backgroundColor = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_backgroundImage(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleBackground gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.backgroundImage = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_backgroundPositionX(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleBackgroundPosition gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.backgroundPositionX = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_backgroundPositionY(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleBackgroundPosition gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.backgroundPositionY = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_backgroundRepeat(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleBackgroundRepeat gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.backgroundRepeat = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_backgroundSize(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleBackgroundSize gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.backgroundSize = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_borderBottomColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleColor gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.borderBottomColor = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_borderBottomLeftRadius(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.borderBottomLeftRadius = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_borderBottomRightRadius(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.borderBottomRightRadius = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_borderBottomWidth(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleFloat gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.borderBottomWidth = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_borderLeftColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleColor gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.borderLeftColor = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_borderLeftWidth(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleFloat gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.borderLeftWidth = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_borderRightColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleColor gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.borderRightColor = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_borderRightWidth(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleFloat gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.borderRightWidth = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_borderTopColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleColor gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.borderTopColor = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_borderTopLeftRadius(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.borderTopLeftRadius = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_borderTopRightRadius(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.borderTopRightRadius = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_borderTopWidth(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleFloat gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.borderTopWidth = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_bottom(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.bottom = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_color(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleColor gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.color = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_cursor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleCursor gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.cursor = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_display(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.DisplayStyle> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.display = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_flexBasis(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.flexBasis = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_flexDirection(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.FlexDirection> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.flexDirection = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_flexGrow(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleFloat gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.flexGrow = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_flexShrink(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleFloat gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.flexShrink = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_flexWrap(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Wrap> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.flexWrap = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_fontSize(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.fontSize = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_height(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.height = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_justifyContent(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Justify> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.justifyContent = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_left(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.left = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_letterSpacing(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.letterSpacing = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_marginBottom(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.marginBottom = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_marginLeft(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.marginLeft = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_marginRight(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.marginRight = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_marginTop(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.marginTop = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_maxHeight(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.maxHeight = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_maxWidth(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.maxWidth = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_minHeight(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.minHeight = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_minWidth(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.minWidth = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_opacity(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleFloat gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.opacity = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_overflow(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Overflow> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.overflow = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_paddingBottom(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.paddingBottom = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_paddingLeft(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.paddingLeft = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_paddingRight(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.paddingRight = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_paddingTop(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.paddingTop = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_position(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Position> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.position = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_right(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.right = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_rotate(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleRotate gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.rotate = gen_value;
            
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
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleScale gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.scale = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_textOverflow(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.TextOverflow> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.textOverflow = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_textShadow(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleTextShadow gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.textShadow = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_top(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.top = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_transformOrigin(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleTransformOrigin gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.transformOrigin = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_transitionDelay(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleList<UnityEngine.UIElements.TimeValue> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.transitionDelay = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_transitionDuration(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleList<UnityEngine.UIElements.TimeValue> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.transitionDuration = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_transitionProperty(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleList<UnityEngine.UIElements.StylePropertyName> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.transitionProperty = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_transitionTimingFunction(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleList<UnityEngine.UIElements.EasingFunction> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.transitionTimingFunction = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_translate(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleTranslate gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.translate = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_unityBackgroundImageTintColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleColor gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.unityBackgroundImageTintColor = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_unityFont(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleFont gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.unityFont = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_unityFontDefinition(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleFontDefinition gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.unityFontDefinition = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_unityFontStyleAndWeight(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleEnum<UnityEngine.FontStyle> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.unityFontStyleAndWeight = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_unityOverflowClipBox(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.OverflowClipBox> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.unityOverflowClipBox = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_unityParagraphSpacing(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.unityParagraphSpacing = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_unitySliceBottom(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleInt gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.unitySliceBottom = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_unitySliceLeft(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleInt gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.unitySliceLeft = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_unitySliceRight(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleInt gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.unitySliceRight = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_unitySliceScale(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleFloat gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.unitySliceScale = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_unitySliceTop(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleInt gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.unitySliceTop = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_unityTextAlign(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleEnum<UnityEngine.TextAnchor> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.unityTextAlign = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_unityTextOutlineColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleColor gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.unityTextOutlineColor = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_unityTextOutlineWidth(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleFloat gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.unityTextOutlineWidth = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_unityTextOverflowPosition(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.TextOverflowPosition> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.unityTextOverflowPosition = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_visibility(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Visibility> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.visibility = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_whiteSpace(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.WhiteSpace> gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.whiteSpace = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_width(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.width = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_wordSpacing(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.IStyle gen_to_be_invoked = (UnityEngine.UIElements.IStyle)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.StyleLength gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.wordSpacing = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
		
		
		
		
    }
}
