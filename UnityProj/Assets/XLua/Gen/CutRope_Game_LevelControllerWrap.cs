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
    public class CutRopeGameLevelControllerWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(CutRope.Game.LevelController);
			Utils.BeginObjectRegister(type, L, translator, 0, 3, 11, 8);
			
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "StopLevel", _m_StopLevel);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "GetRope", _m_GetRope);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "GetRopeCount", _m_GetRopeCount);
			
			
			Utils.RegisterFunc(L, Utils.GETTER_IDX, "ElapsedTime", _g_get_ElapsedTime);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "CutCount", _g_get_CutCount);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "IsRunning", _g_get_IsRunning);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "OnTick", _g_get_OnTick);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "OnCut", _g_get_OnCut);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "OnCandyEaten", _g_get_OnCandyEaten);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "OnCandyFailed", _g_get_OnCandyFailed);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "OnLevelReady", _g_get_OnLevelReady);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "ropes", _g_get_ropes);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "candy", _g_get_candy);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "cutInput", _g_get_cutInput);
            
			Utils.RegisterFunc(L, Utils.SETTER_IDX, "OnTick", _s_set_OnTick);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "OnCut", _s_set_OnCut);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "OnCandyEaten", _s_set_OnCandyEaten);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "OnCandyFailed", _s_set_OnCandyFailed);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "OnLevelReady", _s_set_OnLevelReady);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "ropes", _s_set_ropes);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "candy", _s_set_candy);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "cutInput", _s_set_cutInput);
            
			
			Utils.EndObjectRegister(type, L, translator, null, null,
			    null, null, null);

		    Utils.BeginClassRegister(type, L, __CreateInstance, 1, 1, 0);
			
			
            
			Utils.RegisterFunc(L, Utils.CLS_GETTER_IDX, "Current", _g_get_Current);
            
			
			
			Utils.EndClassRegister(type, L, translator);
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int __CreateInstance(RealStatePtr L)
        {
            
			try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
				if(LuaAPI.lua_gettop(L) == 1)
				{
					
					var gen_ret = new CutRope.Game.LevelController();
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				
			}
			catch(System.Exception gen_e) {
				return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
			}
            return LuaAPI.luaL_error(L, "invalid arguments to CutRope.Game.LevelController constructor!");
            
        }
        
		
        
		
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_StopLevel(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                    gen_to_be_invoked.StopLevel(  );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_GetRope(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    int _index = LuaAPI.xlua_tointeger(L, 2);
                    
                        var gen_ret = gen_to_be_invoked.GetRope( _index );
                        translator.Push(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_GetRopeCount(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    
                        var gen_ret = gen_to_be_invoked.GetRopeCount(  );
                        LuaAPI.xlua_pushinteger(L, gen_ret);
                    
                    
                    
                    return 1;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_Current(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			    translator.Push(L, CutRope.Game.LevelController.Current);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_ElapsedTime(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.ElapsedTime);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_CutCount(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                LuaAPI.xlua_pushinteger(L, gen_to_be_invoked.CutCount);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_IsRunning(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushboolean(L, gen_to_be_invoked.IsRunning);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_OnTick(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.OnTick);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_OnCut(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.OnCut);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_OnCandyEaten(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.OnCandyEaten);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_OnCandyFailed(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.OnCandyFailed);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_OnLevelReady(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.OnLevelReady);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_ropes(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.ropes);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_candy(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.candy);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_cutInput(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.cutInput);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_OnTick(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.OnTick = translator.GetDelegate<CutRope.Game.LevelController.LuaTickDelegate>(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_OnCut(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.OnCut = translator.GetDelegate<CutRope.Game.LevelController.LuaCutDelegate>(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_OnCandyEaten(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.OnCandyEaten = translator.GetDelegate<CutRope.Game.LevelController.LuaVoidDelegate>(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_OnCandyFailed(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.OnCandyFailed = translator.GetDelegate<CutRope.Game.LevelController.LuaVoidDelegate>(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_OnLevelReady(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.OnLevelReady = translator.GetDelegate<CutRope.Game.LevelController.LuaVoidDelegate>(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_ropes(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.ropes = (System.Collections.Generic.List<CutRope.Game.RopeSpawner>)translator.GetObject(L, 2, typeof(System.Collections.Generic.List<CutRope.Game.RopeSpawner>));
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_candy(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.candy = (CutRope.Game.Candy)translator.GetObject(L, 2, typeof(CutRope.Game.Candy));
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_cutInput(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.LevelController gen_to_be_invoked = (CutRope.Game.LevelController)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.cutInput = (CutRope.Game.CutInput)translator.GetObject(L, 2, typeof(CutRope.Game.CutInput));
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
		
		
		
		
    }
}
