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
    public class CutRopeGameRopeSpawnerWrap 
    {
        public static void __Register(RealStatePtr L)
        {
			ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			System.Type type = typeof(CutRope.Game.RopeSpawner);
			Utils.BeginObjectRegister(type, L, translator, 0, 4, 7, 6);
			
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "Spawn", _m_Spawn);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "Cut", _m_Cut);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "CutAtWorld", _m_CutAtWorld);
			Utils.RegisterFunc(L, Utils.METHOD_IDX, "OnSegmentTrigger", _m_OnSegmentTrigger);
			
			
			Utils.RegisterFunc(L, Utils.GETTER_IDX, "IsAlive", _g_get_IsAlive);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "segmentCount", _g_get_segmentCount);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "segmentLength", _g_get_segmentLength);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "segmentMass", _g_get_segmentMass);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "segmentPrefab", _g_get_segmentPrefab);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "lineRenderer", _g_get_lineRenderer);
            Utils.RegisterFunc(L, Utils.GETTER_IDX, "OnRopeCut", _g_get_OnRopeCut);
            
			Utils.RegisterFunc(L, Utils.SETTER_IDX, "segmentCount", _s_set_segmentCount);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "segmentLength", _s_set_segmentLength);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "segmentMass", _s_set_segmentMass);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "segmentPrefab", _s_set_segmentPrefab);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "lineRenderer", _s_set_lineRenderer);
            Utils.RegisterFunc(L, Utils.SETTER_IDX, "OnRopeCut", _s_set_OnRopeCut);
            
			
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
					
					var gen_ret = new CutRope.Game.RopeSpawner();
					translator.Push(L, gen_ret);
                    
					return 1;
				}
				
			}
			catch(System.Exception gen_e) {
				return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
			}
            return LuaAPI.luaL_error(L, "invalid arguments to CutRope.Game.RopeSpawner constructor!");
            
        }
        
		
        
		
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_Spawn(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    CutRope.Game.Candy _candy = (CutRope.Game.Candy)translator.GetObject(L, 2, typeof(CutRope.Game.Candy));
                    
                    gen_to_be_invoked.Spawn( _candy );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_Cut(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    int _index = LuaAPI.xlua_tointeger(L, 2);
                    
                    gen_to_be_invoked.Cut( _index );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_CutAtWorld(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    UnityEngine.Vector2 _worldPos;translator.Get(L, 2, out _worldPos);
                    
                    gen_to_be_invoked.CutAtWorld( _worldPos );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _m_OnSegmentTrigger(RealStatePtr L)
        {
		    try {
            
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
            
            
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
            
            
                
                {
                    CutRope.Game.RopeSegment _seg = (CutRope.Game.RopeSegment)translator.GetObject(L, 2, typeof(CutRope.Game.RopeSegment));
                    UnityEngine.Collider2D _other = (UnityEngine.Collider2D)translator.GetObject(L, 3, typeof(UnityEngine.Collider2D));
                    
                    gen_to_be_invoked.OnSegmentTrigger( _seg, _other );
                    
                    
                    
                    return 0;
                }
                
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            
        }
        
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_IsAlive(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushboolean(L, gen_to_be_invoked.IsAlive);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_segmentCount(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
                LuaAPI.xlua_pushinteger(L, gen_to_be_invoked.segmentCount);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_segmentLength(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.segmentLength);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_segmentMass(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
                LuaAPI.lua_pushnumber(L, gen_to_be_invoked.segmentMass);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_segmentPrefab(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.segmentPrefab);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_lineRenderer(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.lineRenderer);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _g_get_OnRopeCut(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
                translator.Push(L, gen_to_be_invoked.OnRopeCut);
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 1;
        }
        
        
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_segmentCount(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.segmentCount = LuaAPI.xlua_tointeger(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_segmentLength(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.segmentLength = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_segmentMass(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.segmentMass = (float)LuaAPI.lua_tonumber(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_segmentPrefab(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.segmentPrefab = (UnityEngine.GameObject)translator.GetObject(L, 2, typeof(UnityEngine.GameObject));
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_lineRenderer(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.lineRenderer = (UnityEngine.LineRenderer)translator.GetObject(L, 2, typeof(UnityEngine.LineRenderer));
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
        [MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
        static int _s_set_OnRopeCut(RealStatePtr L)
        {
		    try {
                ObjectTranslator translator = ObjectTranslatorPool.Instance.Find(L);
			
                CutRope.Game.RopeSpawner gen_to_be_invoked = (CutRope.Game.RopeSpawner)translator.FastGetCSObj(L, 1);
                gen_to_be_invoked.OnRopeCut = translator.GetDelegate<System.Action<CutRope.Game.RopeSpawner>>(L, 2);
            
            } catch(System.Exception gen_e) {
                return LuaAPI.luaL_error(L, "c# exception:" + gen_e);
            }
            return 0;
        }
        
		
		
		
		
    }
}
