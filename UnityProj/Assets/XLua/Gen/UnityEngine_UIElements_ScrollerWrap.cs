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
    public class UnityEngineUIElementsScrollerWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(UnityEngine.UIElements.Scroller);
			Utils.BeginObjectRegister(type, L, translator, 0, 4, 7, 4);
			
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "Adjust", _m_Adjust);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "ScrollPageUp", _m_ScrollPageUp);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "ScrollPageDown", _m_ScrollPageDown);
			
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "valueChanged", _e_valueChanged);
			
			Utils.RegisterFunc(L, Utils.GETTER_IDX, "slider", _g_get_slider);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "lowButton", _g_get_lowButton);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "highButton", _g_get_highButton);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "value", _g_get_value);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "lowValue", _g_get_lowValue);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "highValue", _g_get_highValue);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "direction", _g_get_direction);
            
			Utils.RegisterFunc(L, Utils.SETTER_IDX, "value", _s_set_value);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "lowValue", _s_set_lowValue);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "highValue", _s_set_highValue);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "direction", _s_set_direction);
            
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 7, 0, 0);
			
			
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "ussClassName", UnityEngine.UIElements.Scroller.ussClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "horizontalVariantUssClassName", UnityEngine.UIElements.Scroller.horizontalVariantUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "verticalVariantUssClassName", UnityEngine.UIElements.Scroller.verticalVariantUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "sliderUssClassName", UnityEngine.UIElements.Scroller.sliderUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "lowButtonUssClassName", UnityEngine.UIElements.Scroller.lowButtonUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "highButtonUssClassName", UnityEngine.UIElements.Scroller.highButtonUssClassName);
            
			
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            
			try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
				if(LuaAPI.lua_gettop(L) == 1)
				{
					
					var gen_ret = new UnityEngine.UIElements.Scroller();
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 5 && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 3) && translator.Assignable<System.Action<float>>(L, 4) && translator.Assignable<UnityEngine.UIElements.SliderDirection>(L, 5))
				{
					float _lowValue = (float)LuaAPI.lua_tonumber(L, 2);
					float _highValue = (float)LuaAPI.lua_tonumber(L, 3);
					System.Action<float> _valueChanged = translator.GetDelegate<System.Action<float>>(L, 4);
					UnityEngine.UIElements.SliderDirection _direction;translator.Get(L, 5, out _direction);
					
					var gen_ret = new UnityEngine.UIElements.Scroller(_lowValue, _highValue, _valueChanged, _direction);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 4 && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 3) && translator.Assignable<System.Action<float>>(L, 4))
				{
					float _lowValue = (float)LuaAPI.lua_tonumber(L, 2);
					float _highValue = (float)LuaAPI.lua_tonumber(L, 3);
					System.Action<float> _valueChanged = translator.GetDelegate<System.Action<float>>(L, 4);
					
					var gen_ret = new UnityEngine.UIElements.Scroller(_lowValue, _highValue, _valueChanged);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				
			}
			catch(System.Exception gen_e) {
				return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
			}
            return LuaAPI.luaL_error(L, "invalid arguments to UnityEngine.UIElements.Scroller constructor!");
            
        }
        
		
        
		
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_Adjust(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.Scroller gen_to_be_invoked = (UnityEngine.UIElements.Scroller)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    float _factor = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    gen_to_be_invoked.Adjust( _factor );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ScrollPageUp(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.Scroller gen_to_be_invoked = (UnityEngine.UIElements.Scroller)translator.FastGetCSObj(L, 1);
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1) 
                {
                    
                    gen_to_be_invoked.ScrollPageUp(  );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    float _factor = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    gen_to_be_invoked.ScrollPageUp( _factor );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UnityEngine.UIElements.Scroller.ScrollPageUp!");
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ScrollPageDown(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.Scroller gen_to_be_invoked = (UnityEngine.UIElements.Scroller)translator.FastGetCSObj(L, 1);
            
            
			    int gen_param_count = LuaAPI.lua_gettop(L);
            
                if(gen_param_count == 1) 
                {
                    
                    gen_to_be_invoked.ScrollPageDown(  );
                    
                    
                    
                    return 0;
                }
                if(gen_param_count == 2&& LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2)) 
                {
                    float _factor = (float)LuaAPI.lua_tonumber(L, 2);
                    
                    gen_to_be_invoked.ScrollPageDown( _factor );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
            return LuaAPI.luaL_error(L, "invalid arguments to UnityEngine.UIElements.Scroller.ScrollPageDown!");
            
        }
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_slider(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.Scroller gen_to_be_invoked = (UnityEngine.UIElements.Scroller)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.slider);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_lowButton(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.Scroller gen_to_be_invoked = (UnityEngine.UIElements.Scroller)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.lowButton);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_highButton(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.Scroller gen_to_be_invoked = (UnityEngine.UIElements.Scroller)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.highButton);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_value(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.Scroller gen_to_be_invoked = (UnityEngine.UIElements.Scroller)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.value);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_lowValue(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.Scroller gen_to_be_invoked = (UnityEngine.UIElements.Scroller)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.lowValue);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_highValue(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.Scroller gen_to_be_invoked = (UnityEngine.UIElements.Scroller)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.highValue);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_direction(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.Scroller gen_to_be_invoked = (UnityEngine.UIElements.Scroller)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.direction);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_value(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.Scroller gen_to_be_invoked = (UnityEngine.UIElements.Scroller)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.value = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_lowValue(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.Scroller gen_to_be_invoked = (UnityEngine.UIElements.Scroller)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.lowValue = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_highValue(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.Scroller gen_to_be_invoked = (UnityEngine.UIElements.Scroller)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.highValue = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_direction(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                UnityEngine.UIElements.Scroller gen_to_be_invoked = (UnityEngine.UIElements.Scroller)translator.FastGetCSObj(L, 1);
                UnityEngine.UIElements.SliderDirection gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.direction = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
		
		
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _e_valueChanged(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			    int gen_param_count = LuaAPI.lua_gettop(L);
			UnityEngine.UIElements.Scroller gen_to_be_invoked = (UnityEngine.UIElements.Scroller)translator.FastGetCSObj(L, 1);
                System.Action<float> gen_delegate = translator.GetDelegate<System.Action<float>>(L, 3);
                if (gen_delegate == null) {
                    return LuaAPI.luaL_error(L, "#3 need System.Action<float>!");
                }
				
				if (gen_param_count == 3)
				{
					
					if (LuaAPI.xlua_is_eq_str(L, 2, "+")) {
						gen_to_be_invoked.valueChanged += gen_delegate;
						return 0;
					} 
					
					
					if (LuaAPI.xlua_is_eq_str(L, 2, "-")) {
						gen_to_be_invoked.valueChanged -= gen_delegate;
						return 0;
					} 
					
				}
			} catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
			LuaAPI.luaL_error(L, "invalid arguments to UnityEngine.UIElements.Scroller.valueChanged!");
            return 0;
        }
        
		
		
    }
}
