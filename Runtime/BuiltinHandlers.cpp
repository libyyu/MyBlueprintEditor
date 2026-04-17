// Runtime/BuiltinHandlers.cpp -- 内置节点处理器注册入口
//
// 该文件仅包含 RegisterBuiltinHandlers() 入口函数。
// 各分类处理器的实现已拆分为独立文件：
//
//   BuiltinHandlers_Flow.cpp    — 控制流 (Branch/ForLoop/Delay/Gate/ExecuteBlueprint …)
//   BuiltinHandlers_Action.cpp  — 动作   (OutputAction/InputActionFire/CustomEvent …)
//   BuiltinHandlers_Math.cpp    — 数学   (Add/Sub/Mul/Div/Sin/Cos/Lerp/Random …)
//   BuiltinHandlers_Debug.cpp   — 调试   (PrintString/Log/Assert/FormatLog …)
//   BuiltinHandlers_String.cpp  — 字符串 (AppendString/StringSplit/FormatString …)
//   BuiltinHandlers_Map.cpp     — Map    (MakeMap/MapGet/MapSet/ForEachMapLoop …)
//   BuiltinHandlers_Array.cpp   — 数组   (ArrayAdd/ArrayGet/ForEachLoop …)
//   BuiltinHandlers_Tree.cpp    — 树结构 (Sequence/MoveTo/RandomWait …)
//   BuiltinHandlers_Houdini.cpp — Houdini(HoudiniTransform/HoudiniGroup …)
//   BuiltinHandlers_Time.cpp    — 时间   (GetTime/DeltaTime/TimeSince/TimerInfo …)
//   BuiltinHandlers_Data.cpp    — 数据   (ToJSON/FromJSON/HasKey/GetField/ArrayToString …)
//   BuiltinHandlers_Misc.cpp    — 杂项   (GetVariable/SetVariable/IsValid/MakeLiteral* …)
//   BuiltinHandlers_Agent.cpp   — Agent扩展 (Memory.*/Tool.Register/Trigger.*)

#include "BuiltinHandlers.h"

#include "handlers/BuiltinHandlers_Flow.h"
#include "handlers/BuiltinHandlers_Action.h"
#include "handlers/BuiltinHandlers_Math.h"
#include "handlers/BuiltinHandlers_Debug.h"
#include "handlers/BuiltinHandlers_String.h"
#include "handlers/BuiltinHandlers_Map.h"
#include "handlers/BuiltinHandlers_Array.h"
#include "handlers/BuiltinHandlers_Set.h"
#include "handlers/BuiltinHandlers_Tree.h"
#include "handlers/BuiltinHandlers_Houdini.h"
#include "handlers/BuiltinHandlers_Time.h"
#include "handlers/BuiltinHandlers_Data.h"
#include "handlers/BuiltinHandlers_Misc.h"
#include "handlers/BuiltinHandlers_Network.h"
#include "handlers/BuiltinHandlers_AI.h"
#include "handlers/BuiltinHandlers_File.h"
#include "handlers/BuiltinHandlers_Server.h"
#include "handlers/BuiltinHandlers_Crypto.h"
#include "handlers/BuiltinHandlers_Proto.h"
#include "handlers/BuiltinHandlers_Agent.h"
#include "handlers/BuiltinHandlers_Game.h"
#include "handlers/BuiltinHandlers_Save.h"
#include "handlers/BuiltinHandlers_GameMath.h"

namespace NodeEditor {
namespace Runtime {

void RegisterBuiltinHandlers(
    BlueprintRunner& runner,
    const std::string& basePath,
    std::unordered_map<std::string, NodeHandler>* outHandlers)
{
    std::unordered_map<std::string, NodeHandler> allHandlers;

    RegisterHandlers_Flow(allHandlers, runner, basePath);
    RegisterHandlers_Action(allHandlers, runner);
    RegisterHandlers_Math(allHandlers);
    RegisterHandlers_Debug(allHandlers);
    RegisterHandlers_String(allHandlers);
    RegisterHandlers_Map(allHandlers);
    RegisterHandlers_Array(allHandlers);
    RegisterHandlers_Set(allHandlers);
    RegisterHandlers_Time(allHandlers, runner);
    RegisterHandlers_Data(allHandlers);
    RegisterHandlers_Tree(allHandlers);
    RegisterHandlers_Houdini(allHandlers);
    RegisterHandlers_Misc(allHandlers);
    RegisterHandlers_Network(allHandlers, runner);
    RegisterHandlers_AI(allHandlers, runner);
    RegisterHandlers_File(allHandlers);
    RegisterHandlers_Server(allHandlers, runner);
    RegisterHandlers_Crypto(allHandlers);
    RegisterHandlers_Proto(allHandlers);
    RegisterHandlers_Agent(allHandlers, runner);
    RegisterHandlers_Game(allHandlers, runner);
    RegisterHandlers_Save(allHandlers);
    RegisterHandlers_GameMath(allHandlers, runner);

    // 设置默认处理器
    runner.SetDefaultHandler([](ExecutionContext& ctx) {
        ctx.Log("  [Default Handler] pass-through");
        return true;
    });

    // 批量注册到 runner
    runner.RegisterHandlers(allHandlers);

    // 如果调用者需要 handlers 映射表（供 Editor 层保存或传递给子蓝图）
    if (outHandlers)
        *outHandlers = std::move(allHandlers);
}

} // namespace Runtime
} // namespace NodeEditor
