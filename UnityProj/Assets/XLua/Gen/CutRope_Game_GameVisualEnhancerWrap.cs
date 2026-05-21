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
    public class CutRopeGameGameVisualEnhancerWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(CutRope.Game.GameVisualEnhancer);
			Utils.BeginObjectRegister(type, L, translator, 0, 0, 8, 8);
			
			
			
			Utils.RegisterFunc(L, Utils.GETTER_IDX, "enhanceBackground", _g_get_enhanceBackground);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "bgColorTop", _g_get_bgColorTop);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "bgColorBottom", _g_get_bgColorBottom);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "enhanceCandy", _g_get_enhanceCandy);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "showCutTrail", _g_get_showCutTrail);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "trailDuration", _g_get_trailDuration);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "trailColor", _g_get_trailColor);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "showCutParticles", _g_get_showCutParticles);
            
			Utils.RegisterFunc(L, Utils.SETTER_IDX, "enhanceBackground", _s_set_enhanceBackground);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "bgColorTop", _s_set_bgColorTop);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "bgColorBottom", _s_set_bgColorBottom);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "enhanceCandy", _s_set_enhanceCandy);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "showCutTrail", _s_set_showCutTrail);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "trailDuration", _s_set_trailDuration);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "trailColor", _s_set_trailColor);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "showCutParticles", _s_set_showCutParticles);
            
			
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
					
					var gen_ret = new CutRope.Game.GameVisualEnhancer();
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				
			}
			catch(System.Exception gen_e) {
				return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
			}
            return LuaAPI.luaL_error(L, "invalid arguments to CutRope.Game.GameVisualEnhancer constructor!");
            
        }
        
		
        
		
        
        
        
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_enhanceBackground(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.GameVisualEnhancer gen_to_be_invoked = (CutRope.Game.GameVisualEnhancer)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushboolean(L, gen_to_be_invoked.enhanceBackground);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_bgColorTop(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.GameVisualEnhancer gen_to_be_invoked = (CutRope.Game.GameVisualEnhancer)translator.FastGetCSObj(L, 1);
                translator.PushUnityEngineColor(L, gen_to_be_invoked.bgColorTop);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_bgColorBottom(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.GameVisualEnhancer gen_to_be_invoked = (CutRope.Game.GameVisualEnhancer)translator.FastGetCSObj(L, 1);
                translator.PushUnityEngineColor(L, gen_to_be_invoked.bgColorBottom);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_enhanceCandy(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.GameVisualEnhancer gen_to_be_invoked = (CutRope.Game.GameVisualEnhancer)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushboolean(L, gen_to_be_invoked.enhanceCandy);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_showCutTrail(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.GameVisualEnhancer gen_to_be_invoked = (CutRope.Game.GameVisualEnhancer)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushboolean(L, gen_to_be_invoked.showCutTrail);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_trailDuration(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.GameVisualEnhancer gen_to_be_invoked = (CutRope.Game.GameVisualEnhancer)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.trailDuration);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_trailColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.GameVisualEnhancer gen_to_be_invoked = (CutRope.Game.GameVisualEnhancer)translator.FastGetCSObj(L, 1);
                translator.PushUnityEngineColor(L, gen_to_be_invoked.trailColor);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_showCutParticles(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.GameVisualEnhancer gen_to_be_invoked = (CutRope.Game.GameVisualEnhancer)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushboolean(L, gen_to_be_invoked.showCutParticles);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_enhanceBackground(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.GameVisualEnhancer gen_to_be_invoked = (CutRope.Game.GameVisualEnhancer)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.enhanceBackground = LuaAPI.lua_toboolean(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_bgColorTop(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.GameVisualEnhancer gen_to_be_invoked = (CutRope.Game.GameVisualEnhancer)translator.FastGetCSObj(L, 1);
                UnityEngine.Color gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.bgColorTop = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_bgColorBottom(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.GameVisualEnhancer gen_to_be_invoked = (CutRope.Game.GameVisualEnhancer)translator.FastGetCSObj(L, 1);
                UnityEngine.Color gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.bgColorBottom = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_enhanceCandy(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.GameVisualEnhancer gen_to_be_invoked = (CutRope.Game.GameVisualEnhancer)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.enhanceCandy = LuaAPI.lua_toboolean(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_showCutTrail(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.GameVisualEnhancer gen_to_be_invoked = (CutRope.Game.GameVisualEnhancer)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.showCutTrail = LuaAPI.lua_toboolean(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_trailDuration(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.GameVisualEnhancer gen_to_be_invoked = (CutRope.Game.GameVisualEnhancer)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.trailDuration = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_trailColor(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.GameVisualEnhancer gen_to_be_invoked = (CutRope.Game.GameVisualEnhancer)translator.FastGetCSObj(L, 1);
                UnityEngine.Color gen_value;translator.Get(L, 2, out gen_value);
				gen_to_be_invoked.trailColor = gen_value;
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_showCutParticles(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.GameVisualEnhancer gen_to_be_invoked = (CutRope.Game.GameVisualEnhancer)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.showCutParticles = LuaAPI.lua_toboolean(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
		
		
		
		
    }
}
