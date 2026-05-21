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
    public class CutRopeGameRopeRendererWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(CutRope.Game.RopeRenderer);
			Utils.BeginObjectRegister(type, L, translator, 0, 2, 11, 11);
			
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "SetPoints", _m_SetPoints);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "Clear", _m_Clear);
			
			
			Utils.RegisterFunc(L, Utils.GETTER_IDX, "interpolationSteps", _g_get_interpolationSteps);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "tension", _g_get_tension);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "widthTop", _g_get_widthTop);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "widthBottom", _g_get_widthBottom);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "colorTop", _g_get_colorTop);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "colorMid", _g_get_colorMid);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "colorBottom", _g_get_colorBottom);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "showAnchorPin", _g_get_showAnchorPin);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "pinRadius", _g_get_pinRadius);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "pinColor", _g_get_pinColor);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "textureScrollSpeed", _g_get_textureScrollSpeed);
            
			Utils.RegisterFunc(L, Utils.SETTER_IDX, "interpolationSteps", _s_set_interpolationSteps);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "tension", _s_set_tension);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "widthTop", _s_set_widthTop);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "widthBottom", _s_set_widthBottom);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "colorTop", _s_set_colorTop);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "colorMid", _s_set_colorMid);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "colorBottom", _s_set_colorBottom);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "showAnchorPin", _s_set_showAnchorPin);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "pinRadius", _s_set_pinRadius);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "pinColor", _s_set_pinColor);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "textureScrollSpeed", _s_set_textureScrollSpeed);
            
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 1, 0, 0);
			
			
            
			
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            
			try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
				if(LuaAPI.lua_gettop(L) == 1)
				{
					
					var gen_ret = new CutRope.Game.RopeRenderer();
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				
			}
			catch(System.Exception gen_e) {
				return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
			}
            return LuaAPI.luaL_error(L, "invalid arguments to CutRope.Game.RopeRenderer constructor!");
            
        }
        
		
        
		
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_SetPoints(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    System.Collections.Generic.List<UnityEngine.Vector3> _points = (System.Collections.Generic.List<UnityEngine.Vector3>)translator.GetObject(L, 2, typeof(System.Collections.Generic.List<UnityEngine.Vector3>));
                    
                    gen_to_be_invoked.SetPoints( _points );
                    
                    
                    
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
            
            
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                    gen_to_be_invoked.Clear(  );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_interpolationSteps(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                LuaAPI.xlua_pushinteger(L, gen_to_be_invoked.interpolationSteps);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_tension(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.tension);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_widthTop(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.widthTop);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_widthBottom(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.widthBottom);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_colorTop(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                translator.PushUnityEngineColor(L, gen_to_be_invoked.colorTop);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_colorMid(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                translator.PushUnityEngineColor(L, gen_to_be_invoked.colorMid);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_colorBottom(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                translator.PushUnityEngineColor(L, gen_to_be_invoked.colorBottom);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_showAnchorPin(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushboolean(L, gen_to_be_invoked.showAnchorPin);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_pinRadius(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.pinRadius);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_pinColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                translator.PushUnityEngineColor(L, gen_to_be_invoked.pinColor);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_textureScrollSpeed(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.textureScrollSpeed);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_interpolationSteps(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.interpolationSteps = LuaAPI.xlua_tointeger(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_tension(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.tension = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_widthTop(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.widthTop = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_widthBottom(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.widthBottom = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_colorTop(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                UnityEngine.Color gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.colorTop = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_colorMid(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                UnityEngine.Color gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.colorMid = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_colorBottom(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                UnityEngine.Color gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.colorBottom = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_showAnchorPin(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.showAnchorPin = LuaAPI.lua_toboolean(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_pinRadius(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.pinRadius = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_pinColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                UnityEngine.Color gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.pinColor = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_textureScrollSpeed(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeRenderer gen_to_be_invoked = (CutRope.Game.RopeRenderer)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.textureScrollSpeed = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
		
		
		
		
    }
}
