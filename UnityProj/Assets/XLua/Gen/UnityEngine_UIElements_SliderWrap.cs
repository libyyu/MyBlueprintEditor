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
    public class UnityEngineUIElementsSliderWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(UnityEngine.UIElements.Slider);
			Utils.BeginObjectRegister(type, L, translator, 0, 1, 0, 0);
			
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "ApplyInputDeviceDelta", _m_ApplyInputDeviceDelta);
			
			
			
			
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 4, 0, 0);
			
			
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "ussClassName", UnityEngine.UIElements.Slider.ussClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "labelUssClassName", UnityEngine.UIElements.Slider.labelUssClassName);
            Utils.RegisterObject(L, translator, Utils.CLS_IDX, "inputUssClassName", UnityEngine.UIElements.Slider.inputUssClassName);
            
			
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            
			try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
				if(LuaAPI.lua_gettop(L) == 1)
				{
					
					var gen_ret = new UnityEngine.UIElements.Slider();
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 5 && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 3) && translator.Assignable<UnityEngine.UIElements.SliderDirection>(L, 4) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 5))
				{
					float _start = (float)LuaAPI.lua_tonumber(L, 2);
					float _end = (float)LuaAPI.lua_tonumber(L, 3);
					UnityEngine.UIElements.SliderDirection _direction;translator.Get(L, 4, out _direction);
					float _pageSize = (float)LuaAPI.lua_tonumber(L, 5);
					
					var gen_ret = new UnityEngine.UIElements.Slider(_start, _end, _direction, _pageSize);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 4 && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 3) && translator.Assignable<UnityEngine.UIElements.SliderDirection>(L, 4))
				{
					float _start = (float)LuaAPI.lua_tonumber(L, 2);
					float _end = (float)LuaAPI.lua_tonumber(L, 3);
					UnityEngine.UIElements.SliderDirection _direction;translator.Get(L, 4, out _direction);
					
					var gen_ret = new UnityEngine.UIElements.Slider(_start, _end, _direction);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 3 && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 2) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 3))
				{
					float _start = (float)LuaAPI.lua_tonumber(L, 2);
					float _end = (float)LuaAPI.lua_tonumber(L, 3);
					
					var gen_ret = new UnityEngine.UIElements.Slider(_start, _end);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 6 && (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 3) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 4) && translator.Assignable<UnityEngine.UIElements.SliderDirection>(L, 5) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 6))
				{
					string _label = LuaAPI.lua_tostring(L, 2);
					float _start = (float)LuaAPI.lua_tonumber(L, 3);
					float _end = (float)LuaAPI.lua_tonumber(L, 4);
					UnityEngine.UIElements.SliderDirection _direction;translator.Get(L, 5, out _direction);
					float _pageSize = (float)LuaAPI.lua_tonumber(L, 6);
					
					var gen_ret = new UnityEngine.UIElements.Slider(_label, _start, _end, _direction, _pageSize);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 5 && (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 3) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 4) && translator.Assignable<UnityEngine.UIElements.SliderDirection>(L, 5))
				{
					string _label = LuaAPI.lua_tostring(L, 2);
					float _start = (float)LuaAPI.lua_tonumber(L, 3);
					float _end = (float)LuaAPI.lua_tonumber(L, 4);
					UnityEngine.UIElements.SliderDirection _direction;translator.Get(L, 5, out _direction);
					
					var gen_ret = new UnityEngine.UIElements.Slider(_label, _start, _end, _direction);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 4 && (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 3) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 4))
				{
					string _label = LuaAPI.lua_tostring(L, 2);
					float _start = (float)LuaAPI.lua_tonumber(L, 3);
					float _end = (float)LuaAPI.lua_tonumber(L, 4);
					
					var gen_ret = new UnityEngine.UIElements.Slider(_label, _start, _end);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 3 && (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING) && LuaTypes.LUA_TNUMBER == LuaAPI.lua_type(L, 3))
				{
					string _label = LuaAPI.lua_tostring(L, 2);
					float _start = (float)LuaAPI.lua_tonumber(L, 3);
					
					var gen_ret = new UnityEngine.UIElements.Slider(_label, _start);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				if(LuaAPI.lua_gettop(L) == 2 && (LuaAPI.lua_isnil(L, 2) || LuaAPI.lua_type(L, 2) == LuaTypes.LUA_TSTRING))
				{
					string _label = LuaAPI.lua_tostring(L, 2);
					
					var gen_ret = new UnityEngine.UIElements.Slider(_label);
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				
			}
			catch(System.Exception gen_e) {
				return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
			}
            return LuaAPI.luaL_error(L, "invalid arguments to UnityEngine.UIElements.Slider constructor!");
            
        }
        
		
        
		
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_ApplyInputDeviceDelta(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                UnityEngine.UIElements.Slider gen_to_be_invoked = (UnityEngine.UIElements.Slider)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    UnityEngine.Vector3 _delta;translator.Get(L, 2, out _delta);
                    UnityEngine.UIElements.DeltaSpeed _speed;translator.Get(L, 3, out _speed);
                    float _startValue = (float)LuaAPI.lua_tonumber(L, 4);
                    
                    gen_to_be_invoked.ApplyInputDeviceDelta( _delta, _speed, _startValue );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        
        
        
        
        
		
		
		
		
    }
}
