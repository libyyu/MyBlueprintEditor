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
    public class UIToolkitExtensionsWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(UIToolkitExtensions);
			Utils.BeginObjectRegister(type, L, translator, 0, 0, 0, 0);
			
			
			
			
			
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 163, 0, 0);
			Utils.RegisterFunc(L, Utils.CLS_IDX, "get_alignContent", _m_get_alignContent_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_alignContent", _m_set_alignContent_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_alignItems", _m_get_alignItems_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_alignItems", _m_set_alignItems_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_alignSelf", _m_get_alignSelf_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_alignSelf", _m_set_alignSelf_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_backgroundColor", _m_get_backgroundColor_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_backgroundColor", _m_set_backgroundColor_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_backgroundImage", _m_get_backgroundImage_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_backgroundImage", _m_set_backgroundImage_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_backgroundPositionX", _m_get_backgroundPositionX_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_backgroundPositionX", _m_set_backgroundPositionX_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_backgroundPositionY", _m_get_backgroundPositionY_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_backgroundPositionY", _m_set_backgroundPositionY_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_backgroundRepeat", _m_get_backgroundRepeat_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_backgroundRepeat", _m_set_backgroundRepeat_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_backgroundSize", _m_get_backgroundSize_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_backgroundSize", _m_set_backgroundSize_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_borderBottomColor", _m_get_borderBottomColor_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_borderBottomColor", _m_set_borderBottomColor_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_borderBottomLeftRadius", _m_get_borderBottomLeftRadius_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_borderBottomLeftRadius", _m_set_borderBottomLeftRadius_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_borderBottomRightRadius", _m_get_borderBottomRightRadius_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_borderBottomRightRadius", _m_set_borderBottomRightRadius_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_borderBottomWidth", _m_get_borderBottomWidth_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_borderBottomWidth", _m_set_borderBottomWidth_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_borderLeftColor", _m_get_borderLeftColor_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_borderLeftColor", _m_set_borderLeftColor_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_borderLeftWidth", _m_get_borderLeftWidth_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_borderLeftWidth", _m_set_borderLeftWidth_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_borderRightColor", _m_get_borderRightColor_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_borderRightColor", _m_set_borderRightColor_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_borderRightWidth", _m_get_borderRightWidth_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_borderRightWidth", _m_set_borderRightWidth_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_borderTopColor", _m_get_borderTopColor_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_borderTopColor", _m_set_borderTopColor_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_borderTopLeftRadius", _m_get_borderTopLeftRadius_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_borderTopLeftRadius", _m_set_borderTopLeftRadius_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_borderTopRightRadius", _m_get_borderTopRightRadius_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_borderTopRightRadius", _m_set_borderTopRightRadius_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_borderTopWidth", _m_get_borderTopWidth_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_borderTopWidth", _m_set_borderTopWidth_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_bottom", _m_get_bottom_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_bottom", _m_set_bottom_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_color", _m_get_color_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_color", _m_set_color_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_cursor", _m_get_cursor_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_cursor", _m_set_cursor_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_display", _m_get_display_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_display", _m_set_display_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_flexBasis", _m_get_flexBasis_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_flexBasis", _m_set_flexBasis_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_flexDirection", _m_get_flexDirection_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_flexDirection", _m_set_flexDirection_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_flexGrow", _m_get_flexGrow_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_flexGrow", _m_set_flexGrow_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_flexShrink", _m_get_flexShrink_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_flexShrink", _m_set_flexShrink_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_flexWrap", _m_get_flexWrap_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_flexWrap", _m_set_flexWrap_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_fontSize", _m_get_fontSize_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_fontSize", _m_set_fontSize_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_height", _m_get_height_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_height", _m_set_height_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_justifyContent", _m_get_justifyContent_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_justifyContent", _m_set_justifyContent_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_left", _m_get_left_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_left", _m_set_left_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_letterSpacing", _m_get_letterSpacing_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_letterSpacing", _m_set_letterSpacing_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_marginBottom", _m_get_marginBottom_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_marginBottom", _m_set_marginBottom_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_marginLeft", _m_get_marginLeft_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_marginLeft", _m_set_marginLeft_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_marginRight", _m_get_marginRight_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_marginRight", _m_set_marginRight_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_marginTop", _m_get_marginTop_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_marginTop", _m_set_marginTop_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_maxHeight", _m_get_maxHeight_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_maxHeight", _m_set_maxHeight_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_maxWidth", _m_get_maxWidth_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_maxWidth", _m_set_maxWidth_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_minHeight", _m_get_minHeight_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_minHeight", _m_set_minHeight_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_minWidth", _m_get_minWidth_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_minWidth", _m_set_minWidth_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_opacity", _m_get_opacity_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_opacity", _m_set_opacity_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_overflow", _m_get_overflow_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_overflow", _m_set_overflow_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_paddingBottom", _m_get_paddingBottom_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_paddingBottom", _m_set_paddingBottom_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_paddingLeft", _m_get_paddingLeft_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_paddingLeft", _m_set_paddingLeft_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_paddingRight", _m_get_paddingRight_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_paddingRight", _m_set_paddingRight_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_paddingTop", _m_get_paddingTop_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_paddingTop", _m_set_paddingTop_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_position", _m_get_position_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_position", _m_set_position_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_right", _m_get_right_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_right", _m_set_right_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_rotate", _m_get_rotate_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_rotate", _m_set_rotate_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_scale", _m_get_scale_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_scale", _m_set_scale_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_textOverflow", _m_get_textOverflow_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_textOverflow", _m_set_textOverflow_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_textShadow", _m_get_textShadow_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_textShadow", _m_set_textShadow_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_top", _m_get_top_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_top", _m_set_top_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_transformOrigin", _m_get_transformOrigin_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_transformOrigin", _m_set_transformOrigin_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_transitionDelay", _m_get_transitionDelay_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_transitionDelay", _m_set_transitionDelay_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_transitionDuration", _m_get_transitionDuration_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_transitionDuration", _m_set_transitionDuration_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_transitionProperty", _m_get_transitionProperty_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_transitionProperty", _m_set_transitionProperty_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_transitionTimingFunction", _m_get_transitionTimingFunction_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_transitionTimingFunction", _m_set_transitionTimingFunction_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_translate", _m_get_translate_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_translate", _m_set_translate_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_unityBackgroundImageTintColor", _m_get_unityBackgroundImageTintColor_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_unityBackgroundImageTintColor", _m_set_unityBackgroundImageTintColor_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_unityFont", _m_get_unityFont_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_unityFont", _m_set_unityFont_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_unityFontDefinition", _m_get_unityFontDefinition_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_unityFontDefinition", _m_set_unityFontDefinition_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_unityFontStyleAndWeight", _m_get_unityFontStyleAndWeight_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_unityFontStyleAndWeight", _m_set_unityFontStyleAndWeight_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_unityOverflowClipBox", _m_get_unityOverflowClipBox_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_unityOverflowClipBox", _m_set_unityOverflowClipBox_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_unityParagraphSpacing", _m_get_unityParagraphSpacing_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_unityParagraphSpacing", _m_set_unityParagraphSpacing_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_unitySliceBottom", _m_get_unitySliceBottom_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_unitySliceBottom", _m_set_unitySliceBottom_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_unitySliceLeft", _m_get_unitySliceLeft_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_unitySliceLeft", _m_set_unitySliceLeft_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_unitySliceRight", _m_get_unitySliceRight_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_unitySliceRight", _m_set_unitySliceRight_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_unitySliceScale", _m_get_unitySliceScale_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_unitySliceScale", _m_set_unitySliceScale_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_unitySliceTop", _m_get_unitySliceTop_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_unitySliceTop", _m_set_unitySliceTop_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_unityTextAlign", _m_get_unityTextAlign_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_unityTextAlign", _m_set_unityTextAlign_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_unityTextOutlineColor", _m_get_unityTextOutlineColor_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_unityTextOutlineColor", _m_set_unityTextOutlineColor_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_unityTextOutlineWidth", _m_get_unityTextOutlineWidth_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_unityTextOutlineWidth", _m_set_unityTextOutlineWidth_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_unityTextOverflowPosition", _m_get_unityTextOverflowPosition_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_unityTextOverflowPosition", _m_set_unityTextOverflowPosition_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_visibility", _m_get_visibility_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_visibility", _m_set_visibility_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_whiteSpace", _m_get_whiteSpace_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_whiteSpace", _m_set_whiteSpace_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_width", _m_get_width_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_width", _m_set_width_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "get_wordSpacing", _m_get_wordSpacing_xlua_st_);
            Utils.RegisterFunc(L, Utils.CLS_IDX, "set_wordSpacing", _m_set_wordSpacing_xlua_st_);
            
			
            
			
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            return LuaAPI.luaL_error(L, "UIToolkitExtensions does not have a constructor!");
        }
        
		
        
		
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_alignContent_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_alignContent( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_alignContent( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_alignContent!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_alignContent_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Align>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Align> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_alignContent( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Align>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Align> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_alignContent( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Align>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Align _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_alignContent( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Align>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Align _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_alignContent( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_alignContent!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_alignItems_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_alignItems( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_alignItems( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_alignItems!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_alignItems_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Align>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Align> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_alignItems( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Align>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Align> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_alignItems( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Align>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Align _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_alignItems( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Align>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Align _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_alignItems( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_alignItems!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_alignSelf_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_alignSelf( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_alignSelf( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_alignSelf!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_alignSelf_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Align>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Align> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_alignSelf( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Align>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Align> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_alignSelf( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Align>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Align _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_alignSelf( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Align>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Align _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_alignSelf( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_alignSelf!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_backgroundColor_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_backgroundColor( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_backgroundColor( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_backgroundColor!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_backgroundColor_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleColor>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleColor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_backgroundColor( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleColor>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleColor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_backgroundColor( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_backgroundColor!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_backgroundImage_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_backgroundImage( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_backgroundImage( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_backgroundImage!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_backgroundImage_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleBackground>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleBackground _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_backgroundImage( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleBackground>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleBackground _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_backgroundImage( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_backgroundImage!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_backgroundPositionX_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_backgroundPositionX( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_backgroundPositionX( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_backgroundPositionX!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_backgroundPositionX_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleBackgroundPosition>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleBackgroundPosition _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_backgroundPositionX( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleBackgroundPosition>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleBackgroundPosition _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_backgroundPositionX( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_backgroundPositionX!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_backgroundPositionY_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_backgroundPositionY( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_backgroundPositionY( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_backgroundPositionY!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_backgroundPositionY_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleBackgroundPosition>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleBackgroundPosition _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_backgroundPositionY( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleBackgroundPosition>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleBackgroundPosition _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_backgroundPositionY( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_backgroundPositionY!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_backgroundRepeat_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_backgroundRepeat( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_backgroundRepeat( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_backgroundRepeat!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_backgroundRepeat_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleBackgroundRepeat>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleBackgroundRepeat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_backgroundRepeat( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleBackgroundRepeat>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleBackgroundRepeat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_backgroundRepeat( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_backgroundRepeat!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_backgroundSize_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_backgroundSize( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_backgroundSize( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_backgroundSize!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_backgroundSize_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleBackgroundSize>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleBackgroundSize _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_backgroundSize( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleBackgroundSize>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleBackgroundSize _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_backgroundSize( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_backgroundSize!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_borderBottomColor_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_borderBottomColor( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_borderBottomColor( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_borderBottomColor!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_borderBottomColor_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleColor>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleColor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderBottomColor( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleColor>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleColor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderBottomColor( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_borderBottomColor!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_borderBottomLeftRadius_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_borderBottomLeftRadius( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_borderBottomLeftRadius( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_borderBottomLeftRadius!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_borderBottomLeftRadius_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_borderBottomLeftRadius( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_borderBottomLeftRadius( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderBottomLeftRadius( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderBottomLeftRadius( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderBottomLeftRadius( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderBottomLeftRadius( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_borderBottomLeftRadius!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_borderBottomRightRadius_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_borderBottomRightRadius( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_borderBottomRightRadius( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_borderBottomRightRadius!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_borderBottomRightRadius_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_borderBottomRightRadius( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_borderBottomRightRadius( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderBottomRightRadius( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderBottomRightRadius( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderBottomRightRadius( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderBottomRightRadius( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_borderBottomRightRadius!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_borderBottomWidth_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_borderBottomWidth( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_borderBottomWidth( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_borderBottomWidth!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_borderBottomWidth_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderBottomWidth( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderBottomWidth( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_borderBottomWidth!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_borderLeftColor_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_borderLeftColor( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_borderLeftColor( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_borderLeftColor!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_borderLeftColor_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleColor>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleColor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderLeftColor( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleColor>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleColor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderLeftColor( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_borderLeftColor!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_borderLeftWidth_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_borderLeftWidth( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_borderLeftWidth( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_borderLeftWidth!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_borderLeftWidth_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderLeftWidth( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderLeftWidth( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_borderLeftWidth!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_borderRightColor_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_borderRightColor( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_borderRightColor( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_borderRightColor!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_borderRightColor_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleColor>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleColor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderRightColor( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleColor>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleColor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderRightColor( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_borderRightColor!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_borderRightWidth_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_borderRightWidth( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_borderRightWidth( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_borderRightWidth!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_borderRightWidth_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderRightWidth( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderRightWidth( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_borderRightWidth!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_borderTopColor_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_borderTopColor( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_borderTopColor( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_borderTopColor!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_borderTopColor_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleColor>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleColor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderTopColor( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleColor>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleColor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderTopColor( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_borderTopColor!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_borderTopLeftRadius_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_borderTopLeftRadius( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_borderTopLeftRadius( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_borderTopLeftRadius!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_borderTopLeftRadius_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_borderTopLeftRadius( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_borderTopLeftRadius( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderTopLeftRadius( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderTopLeftRadius( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderTopLeftRadius( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderTopLeftRadius( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_borderTopLeftRadius!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_borderTopRightRadius_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_borderTopRightRadius( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_borderTopRightRadius( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_borderTopRightRadius!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_borderTopRightRadius_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_borderTopRightRadius( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_borderTopRightRadius( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderTopRightRadius( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderTopRightRadius( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderTopRightRadius( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderTopRightRadius( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_borderTopRightRadius!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_borderTopWidth_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_borderTopWidth( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_borderTopWidth( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_borderTopWidth!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_borderTopWidth_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderTopWidth( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_borderTopWidth( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_borderTopWidth!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_bottom_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_bottom( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_bottom( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_bottom!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_bottom_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_bottom( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_bottom( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_bottom( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_bottom( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_bottom( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_bottom( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_bottom!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_color_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_color( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_color( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_color!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_color_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleColor>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleColor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_color( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleColor>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleColor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_color( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.Color>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.Color _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_color( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.Color>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.Color _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_color( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_color!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_cursor_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_cursor( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_cursor( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_cursor!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_cursor_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleCursor>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleCursor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_cursor( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleCursor>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleCursor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_cursor( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_cursor!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_display_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_display( _e );
                        translator.PushUnityEngineUIElementsDisplayStyle(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_display( _s );
                        translator.PushUnityEngineUIElementsDisplayStyle(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_display!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_display_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.DisplayStyle>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.DisplayStyle> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_display( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.DisplayStyle>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.DisplayStyle> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_display( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.DisplayStyle>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.DisplayStyle _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_display( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.DisplayStyle>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.DisplayStyle _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_display( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_display!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_flexBasis_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_flexBasis( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_flexBasis( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_flexBasis!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_flexBasis_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_flexBasis( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_flexBasis( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_flexBasis( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_flexBasis( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_flexBasis( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_flexBasis( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_flexBasis!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_flexDirection_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_flexDirection( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_flexDirection( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_flexDirection!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_flexDirection_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.FlexDirection>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.FlexDirection> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_flexDirection( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.FlexDirection>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.FlexDirection> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_flexDirection( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.FlexDirection>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.FlexDirection _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_flexDirection( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.FlexDirection>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.FlexDirection _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_flexDirection( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_flexDirection!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_flexGrow_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_flexGrow( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_flexGrow( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_flexGrow!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_flexGrow_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_flexGrow( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_flexGrow( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_flexGrow!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_flexShrink_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_flexShrink( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_flexShrink( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_flexShrink!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_flexShrink_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_flexShrink( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_flexShrink( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_flexShrink!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_flexWrap_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_flexWrap( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_flexWrap( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_flexWrap!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_flexWrap_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Wrap>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Wrap> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_flexWrap( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Wrap>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Wrap> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_flexWrap( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Wrap>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Wrap _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_flexWrap( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Wrap>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Wrap _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_flexWrap( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_flexWrap!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_fontSize_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_fontSize( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_fontSize( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_fontSize!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_fontSize_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_fontSize( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_fontSize( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_fontSize( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_fontSize( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_fontSize( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_fontSize( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_fontSize!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_height_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_height( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_height( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_height!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_height_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_height( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_height( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_height( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_height( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_height( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_height( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_height!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_justifyContent_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_justifyContent( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_justifyContent( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_justifyContent!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_justifyContent_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Justify>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Justify> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_justifyContent( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Justify>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Justify> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_justifyContent( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Justify>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Justify _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_justifyContent( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Justify>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Justify _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_justifyContent( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_justifyContent!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_left_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_left( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_left( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_left!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_left_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_left( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_left( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_left( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_left( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_left( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_left( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_left!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_letterSpacing_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_letterSpacing( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_letterSpacing( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_letterSpacing!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_letterSpacing_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_letterSpacing( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_letterSpacing( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_letterSpacing( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_letterSpacing( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_letterSpacing( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_letterSpacing( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_letterSpacing!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_marginBottom_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_marginBottom( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_marginBottom( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_marginBottom!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_marginBottom_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_marginBottom( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_marginBottom( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_marginBottom( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_marginBottom( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_marginBottom( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_marginBottom( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_marginBottom!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_marginLeft_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_marginLeft( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_marginLeft( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_marginLeft!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_marginLeft_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_marginLeft( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_marginLeft( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_marginLeft( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_marginLeft( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_marginLeft( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_marginLeft( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_marginLeft!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_marginRight_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_marginRight( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_marginRight( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_marginRight!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_marginRight_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_marginRight( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_marginRight( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_marginRight( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_marginRight( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_marginRight( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_marginRight( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_marginRight!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_marginTop_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_marginTop( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_marginTop( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_marginTop!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_marginTop_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_marginTop( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_marginTop( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_marginTop( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_marginTop( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_marginTop( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_marginTop( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_marginTop!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_maxHeight_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_maxHeight( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_maxHeight( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_maxHeight!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_maxHeight_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_maxHeight( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_maxHeight( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_maxHeight( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_maxHeight( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_maxHeight( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_maxHeight( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_maxHeight!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_maxWidth_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_maxWidth( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_maxWidth( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_maxWidth!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_maxWidth_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_maxWidth( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_maxWidth( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_maxWidth( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_maxWidth( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_maxWidth( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_maxWidth( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_maxWidth!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_minHeight_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_minHeight( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_minHeight( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_minHeight!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_minHeight_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_minHeight( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_minHeight( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_minHeight( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_minHeight( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_minHeight( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_minHeight( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_minHeight!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_minWidth_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_minWidth( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_minWidth( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_minWidth!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_minWidth_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_minWidth( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_minWidth( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_minWidth( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_minWidth( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_minWidth( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_minWidth( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_minWidth!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_opacity_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_opacity( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_opacity( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_opacity!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_opacity_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_opacity( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_opacity( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_opacity!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_overflow_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_overflow( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_overflow( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_overflow!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_overflow_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Overflow>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Overflow> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_overflow( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Overflow>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Overflow> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_overflow( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Overflow>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Overflow _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_overflow( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Overflow>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Overflow _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_overflow( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_overflow!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_paddingBottom_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_paddingBottom( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_paddingBottom( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_paddingBottom!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_paddingBottom_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_paddingBottom( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_paddingBottom( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_paddingBottom( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_paddingBottom( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_paddingBottom( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_paddingBottom( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_paddingBottom!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_paddingLeft_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_paddingLeft( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_paddingLeft( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_paddingLeft!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_paddingLeft_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_paddingLeft( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_paddingLeft( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_paddingLeft( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_paddingLeft( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_paddingLeft( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_paddingLeft( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_paddingLeft!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_paddingRight_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_paddingRight( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_paddingRight( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_paddingRight!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_paddingRight_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_paddingRight( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_paddingRight( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_paddingRight( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_paddingRight( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_paddingRight( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_paddingRight( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_paddingRight!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_paddingTop_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_paddingTop( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_paddingTop( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_paddingTop!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_paddingTop_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_paddingTop( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_paddingTop( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_paddingTop( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_paddingTop( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_paddingTop( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_paddingTop( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_paddingTop!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_position_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_position( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_position( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_position!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_position_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Position>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Position> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_position( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Position>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Position> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_position( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Position>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Position _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_position( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Position>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Position _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_position( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_position!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_right_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_right( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_right( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_right!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_right_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_right( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_right( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_right( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_right( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_right( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_right( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_right!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_rotate_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_rotate( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_rotate( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_rotate!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_rotate_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleRotate>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleRotate _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_rotate( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleRotate>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleRotate _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_rotate( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_rotate!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_scale_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_scale( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_scale( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_scale!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_scale_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleScale>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleScale _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_scale( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleScale>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleScale _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_scale( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_scale!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_textOverflow_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_textOverflow( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_textOverflow( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_textOverflow!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_textOverflow_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.TextOverflow>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.TextOverflow> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_textOverflow( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.TextOverflow>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.TextOverflow> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_textOverflow( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.TextOverflow>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.TextOverflow _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_textOverflow( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.TextOverflow>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.TextOverflow _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_textOverflow( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_textOverflow!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_textShadow_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_textShadow( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_textShadow( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_textShadow!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_textShadow_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleTextShadow>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleTextShadow _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_textShadow( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleTextShadow>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleTextShadow _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_textShadow( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_textShadow!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_top_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_top( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_top( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_top!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_top_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_top( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_top( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_top( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_top( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_top( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_top( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_top!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_transformOrigin_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_transformOrigin( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_transformOrigin( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_transformOrigin!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_transformOrigin_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleTransformOrigin>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleTransformOrigin _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_transformOrigin( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleTransformOrigin>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleTransformOrigin _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_transformOrigin( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_transformOrigin!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_transitionDelay_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_transitionDelay( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_transitionDelay( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_transitionDelay!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_transitionDelay_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleList<UnityEngine.UIElements.TimeValue>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleList<UnityEngine.UIElements.TimeValue> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_transitionDelay( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleList<UnityEngine.UIElements.TimeValue>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleList<UnityEngine.UIElements.TimeValue> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_transitionDelay( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_transitionDelay!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_transitionDuration_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_transitionDuration( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_transitionDuration( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_transitionDuration!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_transitionDuration_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleList<UnityEngine.UIElements.TimeValue>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleList<UnityEngine.UIElements.TimeValue> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_transitionDuration( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleList<UnityEngine.UIElements.TimeValue>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleList<UnityEngine.UIElements.TimeValue> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_transitionDuration( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_transitionDuration!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_transitionProperty_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_transitionProperty( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_transitionProperty( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_transitionProperty!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_transitionProperty_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleList<UnityEngine.UIElements.StylePropertyName>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleList<UnityEngine.UIElements.StylePropertyName> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_transitionProperty( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleList<UnityEngine.UIElements.StylePropertyName>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleList<UnityEngine.UIElements.StylePropertyName> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_transitionProperty( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_transitionProperty!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_transitionTimingFunction_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_transitionTimingFunction( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_transitionTimingFunction( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_transitionTimingFunction!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_transitionTimingFunction_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleList<UnityEngine.UIElements.EasingFunction>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleList<UnityEngine.UIElements.EasingFunction> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_transitionTimingFunction( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleList<UnityEngine.UIElements.EasingFunction>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleList<UnityEngine.UIElements.EasingFunction> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_transitionTimingFunction( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_transitionTimingFunction!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_translate_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_translate( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_translate( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_translate!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_translate_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleTranslate>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleTranslate _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_translate( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleTranslate>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleTranslate _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_translate( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_translate!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_unityBackgroundImageTintColor_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_unityBackgroundImageTintColor( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_unityBackgroundImageTintColor( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_unityBackgroundImageTintColor!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_unityBackgroundImageTintColor_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleColor>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleColor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityBackgroundImageTintColor( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleColor>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleColor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityBackgroundImageTintColor( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_unityBackgroundImageTintColor!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_unityFont_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_unityFont( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_unityFont( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_unityFont!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_unityFont_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFont>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleFont _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityFont( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFont>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleFont _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityFont( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_unityFont!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_unityFontDefinition_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_unityFontDefinition( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_unityFontDefinition( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_unityFontDefinition!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_unityFontDefinition_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFontDefinition>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleFontDefinition _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityFontDefinition( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFontDefinition>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleFontDefinition _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityFontDefinition( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_unityFontDefinition!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_unityFontStyleAndWeight_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_unityFontStyleAndWeight( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_unityFontStyleAndWeight( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_unityFontStyleAndWeight!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_unityFontStyleAndWeight_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.FontStyle>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.FontStyle> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityFontStyleAndWeight( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.FontStyle>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.FontStyle> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityFontStyleAndWeight( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.FontStyle>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.FontStyle _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityFontStyleAndWeight( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.FontStyle>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.FontStyle _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityFontStyleAndWeight( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_unityFontStyleAndWeight!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_unityOverflowClipBox_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_unityOverflowClipBox( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_unityOverflowClipBox( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_unityOverflowClipBox!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_unityOverflowClipBox_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.OverflowClipBox>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.OverflowClipBox> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityOverflowClipBox( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.OverflowClipBox>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.OverflowClipBox> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityOverflowClipBox( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.OverflowClipBox>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.OverflowClipBox _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityOverflowClipBox( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.OverflowClipBox>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.OverflowClipBox _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityOverflowClipBox( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_unityOverflowClipBox!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_unityParagraphSpacing_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_unityParagraphSpacing( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_unityParagraphSpacing( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_unityParagraphSpacing!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_unityParagraphSpacing_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_unityParagraphSpacing( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_unityParagraphSpacing( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityParagraphSpacing( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityParagraphSpacing( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityParagraphSpacing( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityParagraphSpacing( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_unityParagraphSpacing!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_unitySliceBottom_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_unitySliceBottom( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_unitySliceBottom( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_unitySliceBottom!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_unitySliceBottom_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleInt>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleInt _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unitySliceBottom( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleInt>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleInt _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unitySliceBottom( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_unitySliceBottom!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_unitySliceLeft_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_unitySliceLeft( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_unitySliceLeft( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_unitySliceLeft!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_unitySliceLeft_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleInt>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleInt _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unitySliceLeft( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleInt>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleInt _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unitySliceLeft( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_unitySliceLeft!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_unitySliceRight_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_unitySliceRight( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_unitySliceRight( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_unitySliceRight!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_unitySliceRight_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleInt>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleInt _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unitySliceRight( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleInt>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleInt _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unitySliceRight( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_unitySliceRight!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_unitySliceScale_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_unitySliceScale( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_unitySliceScale( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_unitySliceScale!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_unitySliceScale_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unitySliceScale( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unitySliceScale( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_unitySliceScale!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_unitySliceTop_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_unitySliceTop( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_unitySliceTop( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_unitySliceTop!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_unitySliceTop_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleInt>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleInt _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unitySliceTop( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleInt>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleInt _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unitySliceTop( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_unitySliceTop!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_unityTextAlign_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_unityTextAlign( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_unityTextAlign( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_unityTextAlign!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_unityTextAlign_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.TextAnchor>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.TextAnchor> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityTextAlign( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.TextAnchor>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.TextAnchor> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityTextAlign( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.TextAnchor>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.TextAnchor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityTextAlign( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.TextAnchor>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.TextAnchor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityTextAlign( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_unityTextAlign!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_unityTextOutlineColor_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_unityTextOutlineColor( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_unityTextOutlineColor( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_unityTextOutlineColor!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_unityTextOutlineColor_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleColor>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleColor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityTextOutlineColor( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleColor>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleColor _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityTextOutlineColor( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_unityTextOutlineColor!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_unityTextOutlineWidth_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_unityTextOutlineWidth( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_unityTextOutlineWidth( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_unityTextOutlineWidth!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_unityTextOutlineWidth_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityTextOutlineWidth( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleFloat>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleFloat _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityTextOutlineWidth( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_unityTextOutlineWidth!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_unityTextOverflowPosition_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_unityTextOverflowPosition( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_unityTextOverflowPosition( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_unityTextOverflowPosition!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_unityTextOverflowPosition_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.TextOverflowPosition>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.TextOverflowPosition> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityTextOverflowPosition( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.TextOverflowPosition>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.TextOverflowPosition> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityTextOverflowPosition( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.TextOverflowPosition>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.TextOverflowPosition _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityTextOverflowPosition( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.TextOverflowPosition>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.TextOverflowPosition _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_unityTextOverflowPosition( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_unityTextOverflowPosition!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_visibility_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_visibility( _e );
                        translator.PushUnityEngineUIElementsVisibility(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_visibility( _s );
                        translator.PushUnityEngineUIElementsVisibility(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_visibility!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_visibility_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Visibility>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Visibility> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_visibility( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Visibility>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.Visibility> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_visibility( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Visibility>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Visibility _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_visibility( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Visibility>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Visibility _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_visibility( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_visibility!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_whiteSpace_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_whiteSpace( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_whiteSpace( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_whiteSpace!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_whiteSpace_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.WhiteSpace>>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.WhiteSpace> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_whiteSpace( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.WhiteSpace>>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleEnum<UnityEngine.UIElements.WhiteSpace> _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_whiteSpace( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.WhiteSpace>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.WhiteSpace _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_whiteSpace( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.WhiteSpace>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.WhiteSpace _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_whiteSpace( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_whiteSpace!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_width_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_width( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_width( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_width!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_width_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_width( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_width( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_width( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_width( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_width( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_width( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_width!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_get_wordSpacing_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    
                        var gen_ret = UIToolkitExtensions.get_wordSpacing( _e );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                if(gen_param_count == 1&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    
                        var gen_ret = UIToolkitExtensions.get_wordSpacing( _s );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.get_wordSpacing!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_set_wordSpacing_xlua_st_(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_wordSpacing( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    float _v = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    UIToolkitExtensions.set_wordSpacing( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_wordSpacing( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.StyleLength>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.StyleLength _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_wordSpacing( _s, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.VisualElement>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.VisualElement _e = (UnityEngine.UIElements.VisualElement)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.VisualElement));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_wordSpacing( _e, _v );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& translator.Assignable<UnityEngine.UIElements.IStyle>(L, 1)&& translator.Assignable<UnityEngine.UIElements.Length>(L, 2)) 
                {
                    UnityEngine.UIElements.IStyle _s = (UnityEngine.UIElements.IStyle)translator.GetObject(L, 1, typeof(UnityEngine.UIElements.IStyle));
                    UnityEngine.UIElements.Length _v;translator.Get(L, 2, out _v);
                    
                    UIToolkitExtensions.set_wordSpacing( _s, _v );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UIToolkitExtensions.set_wordSpacing!");
            
        }
        
        
        
        
        
        
		
		
		
		
    }
}
