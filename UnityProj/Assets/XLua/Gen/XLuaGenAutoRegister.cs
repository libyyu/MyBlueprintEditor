#if USE_UNI_LUA
using LuaAPI = UniLua.Lua;
using RealStatePtr = UniLua.ILuaState;
using LuaCSFunction = UniLua.CSharpFunctionDelegate;
#else
using LuaAPI = XLua.LuaDLL.Lua;
using RealStatePtr = System.IntPtr;
using LuaCSFunction = XLua.LuaDLL.lua_CSFunction;
#endif

using System;
using System.Collections.Generic;
using System.Reflection;


namespace XLua.CSObjectWrap
{
    public class XLua_Gen_Initer_Register__
	{
        
        
        static void wrapInit0(LuaEnv luaenv, ObjectTranslator translator)
        {
        
            translator.DelayWrapLoader(typeof(CutRope.Game.CameraShaker), CutRopeGameCameraShakerWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(CutRope.Game.Candy), CutRopeGameCandyWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(CutRope.Game.CutInput), CutRopeGameCutInputWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(CutRope.Game.GameVisualEnhancer), CutRopeGameGameVisualEnhancerWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(CutRope.Game.LevelController), CutRopeGameLevelControllerWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(CutRope.Game.Obstacle), CutRopeGameObstacleWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(CutRope.Game.RopeRenderer), CutRopeGameRopeRendererWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(CutRope.Game.RopeSpawner), CutRopeGameRopeSpawnerWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(CutRope.Game.Star), CutRopeGameStarWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(CutRope.Game.RopeVisualSetup), CutRopeGameRopeVisualSetupWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UGFramework.Runtime.IUIPanelBackend), UGFrameworkRuntimeIUIPanelBackendWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UGFramework.Runtime.UGUIPanelBackend), UGFrameworkRuntimeUGUIPanelBackendWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UGFramework.Runtime.UITKPanelBackend), UGFrameworkRuntimeUITKPanelBackendWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UGFramework.Runtime.IUIPanelBridge), UGFrameworkRuntimeIUIPanelBridgeWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UGFramework.Runtime.UITKLuaBridge), UGFrameworkRuntimeUITKLuaBridgeWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.IStyle), UnityEngineUIElementsIStyleWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.VisualElement), UnityEngineUIElementsVisualElementWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.VisualElementExtensions), UnityEngineUIElementsVisualElementExtensionsWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UIToolkitExtensions), UIToolkitExtensionsWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.UQueryExtensions), UnityEngineUIElementsUQueryExtensionsWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.UIDocument), UnityEngineUIElementsUIDocumentWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.PanelSettings), UnityEngineUIElementsPanelSettingsWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.Button), UnityEngineUIElementsButtonWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.Label), UnityEngineUIElementsLabelWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.TextField), UnityEngineUIElementsTextFieldWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.Toggle), UnityEngineUIElementsToggleWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.Slider), UnityEngineUIElementsSliderWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.ScrollView), UnityEngineUIElementsScrollViewWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.Scroller), UnityEngineUIElementsScrollerWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.ProgressBar), UnityEngineUIElementsProgressBarWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.DropdownField), UnityEngineUIElementsDropdownFieldWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.DisplayStyle), UnityEngineUIElementsDisplayStyleWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.Visibility), UnityEngineUIElementsVisibilityWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UnityEngine.UIElements.ScrollViewMode), UnityEngineUIElementsScrollViewModeWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UGFramework.Runtime.AudioManager), UGFrameworkRuntimeAudioManagerWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UGFramework.Runtime.FTimerListBehavior), UGFrameworkRuntimeFTimerListBehaviorWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UGFramework.Runtime.GameUtil), UGFrameworkRuntimeGameUtilWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UGFramework.Runtime.SceneLoader), UGFrameworkRuntimeSceneLoaderWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UGFramework.Runtime.UILuaBehaviour), UGFrameworkRuntimeUILuaBehaviourWrap.__Register);
        
        
            translator.DelayWrapLoader(typeof(UGFramework.Runtime.YooAssetsLuaBridge), UGFrameworkRuntimeYooAssetsLuaBridgeWrap.__Register);
        
        
        
        }
        
        public static void Init(LuaEnv luaenv, ObjectTranslator translator)
        {
            
            wrapInit0(luaenv, translator);
            
            
        }
	}
}

namespace XLua
{
	internal partial class InternalGlobals_Gen
    {
	    
        private delegate bool TryArrayGet(Type type, RealStatePtr L, ObjectTranslator translator, object obj, int index);
        private delegate bool TryArraySet(Type type, RealStatePtr L, ObjectTranslator translator, object obj, int array_idx, int obj_idx);
	    private static void Init(
            out Dictionary<Type, IEnumerable<MethodInfo>> extensionMethodMap,
            out TryArrayGet genTryArrayGetPtr,
            out TryArraySet genTryArraySetPtr)
		{
            XLua.LuaEnv.AddIniter(XLua.CSObjectWrap.XLua_Gen_Initer_Register__.Init);
            XLua.LuaEnv.AddIniter(XLua.ObjectTranslator_Gen.Init);
		    extensionMethodMap = new Dictionary<Type, IEnumerable<MethodInfo>>()
			{
			    
			};
			
            genTryArrayGetPtr = StaticLuaCallbacks_Wrap.__tryArrayGet;
            genTryArraySetPtr = StaticLuaCallbacks_Wrap.__tryArraySet;
		}
	}
}
