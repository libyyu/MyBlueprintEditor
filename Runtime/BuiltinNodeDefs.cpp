// Runtime/BuiltinNodeDefs.cpp -- 内置节点定义注册入口
//
// 该文件仅包含 RegisterBuiltinNodeDefinitions() 入口函数，
// 负责注册全部分类（NodeCategory）并依次调用各 RegisterNodeDefs_XX。
//
// 各分类的节点定义实现已拆分到 Runtime/nodedefs/ 子目录：
//
//   BuiltinNodeDefs_Flow.cpp        — Flow Control (Branch/ForLoop/Delay/Gate/...)
//   BuiltinNodeDefs_Action.cpp      — Actions (OutputAction/InputActionFire/...)
//   BuiltinNodeDefs_Math.cpp        — Math (Add/Sub/Sin/Lerp/Random/...)
//   BuiltinNodeDefs_Debug.cpp       — Debug (PrintString/Log/Assert/...)
//   BuiltinNodeDefs_String.cpp      — String (AppendString/StringSplit/Format/...)
//   BuiltinNodeDefs_Array.cpp       — Array (ArrayAdd/ArrayGet/ForEach/...)
//   BuiltinNodeDefs_Map.cpp         — Map (MakeMap/MapGet/MapSet/...)
//   BuiltinNodeDefs_Time.cpp        — Time (GetTime/DeltaTime/TimeSince/...)
//   BuiltinNodeDefs_Data.cpp        — Data (ToJSON/FromJSON/HasKey/...)
//   BuiltinNodeDefs_Tree.cpp        — Behavior Tree (Sequence/MoveTo/...)
//   BuiltinNodeDefs_Houdini.cpp     — Houdini (HoudiniTransform/HoudiniGroup/...)
//   BuiltinNodeDefs_Misc.cpp        — Misc (GetVariable/SetVariable/IsValid/...)
//   BuiltinNodeDefs_Conversion.cpp  — Type conversion nodes
//   BuiltinNodeDefs_Event.cpp       — Events (OnBeginPlay/DispatchEvent/...)
//   BuiltinNodeDefs_Function.cpp    — Function nodes
//   BuiltinNodeDefs_EventBus.cpp    — EventBus subscribe/publish
//   BuiltinNodeDefs_Retry.cpp       — Retry / Backoff nodes
//   BuiltinNodeDefs_File.cpp        — File I/O
//   BuiltinNodeDefs_Network.cpp     — Network (HTTP/WS/MQTT/...)
//   BuiltinNodeDefs_AI.cpp          — AI / LLM / Tool / Agent / MCP
//   BuiltinNodeDefs_Game.cpp        — Game (BehaviorTree/StateMachine/Physics/...)
//   BuiltinNodeDefs_Save.cpp        — Save system
//   BuiltinNodeDefs_GameMath.cpp    — Game-specific math
//   BuiltinNodeDefs_Socket.cpp      — TCP/UDP sockets
//   BuiltinNodeDefs_GameExtra.cpp   — Inventory/Easing/Random/Entity & Tag

#include "BuiltinNodeDefs.h"
#include "nodedefs/BuiltinNodeDefs_Internal.h"

namespace NodeEditor {
namespace Runtime {

void RegisterBuiltinNodeDefinitions(INodeRegistry& registry)
{
    // --- 注册分类 ---
    auto addCat = [&registry](const char* id, const char* name) {
        NodeCategory cat;
        cat.id = id;
        cat.name = name;
        registry.registerCategory(cat);
    };
    addCat("Flow",       "Flow Control");
    addCat("Action",     "Actions");
    addCat("Math",       "Math");
    addCat("Debug",      "Debug");
    addCat("Time",       "Time");
    addCat("Data",       "Data");
    addCat("Tree",       "Behavior Tree");
    addCat("Houdini",    "Houdini");
    addCat("Misc",       "Misc");
    addCat("Conversion", "Conversion");
    addCat("Event",      "Events");
    addCat("Function",   "Functions");
    addCat("Network",    "Network");
    addCat("AI",         "AI / LLM");
    addCat("AI/JSON",    "AI / JSON");
    addCat("AI/LLM",     "AI / LLM");
    addCat("AI/String",  "AI / String");
    addCat("AI/Memory",  "AI / Memory");
    addCat("AI/Tool",    "AI / Tool");
    addCat("AI/Agent",   "AI / Agent");
    addCat("AI/MCP",     "AI / MCP");
    addCat("File",             "File I/O");
    addCat("Game/BehaviorTree","Behavior Tree");
    addCat("Game/StateMachine","State Machine");
    addCat("Game/Save",        "Save System");
    addCat("Game/Math",        "Game Math");
    addCat("Game/Physics",     "Physics Helpers");
    addCat("Game/Stat",        "Stat System");
    addCat("Game/Cooldown",    "Cooldown");
    addCat("Network/TCP",      "TCP");
    addCat("Network/UDP",      "UDP");
    addCat("Game/Random",      "Random");
    addCat("Game/Easing",      "Easing");
    addCat("Game/Timer",       "Game Timer");
    addCat("Game/Entity",      "Entity & Tag");
    addCat("Game/Inventory",   "Inventory");

    // --- 注册各分类的节点定义（实现在 Runtime/nodedefs/ 子目录） ---
    RegisterNodeDefs_Flow(registry);
    RegisterNodeDefs_Action(registry);
    RegisterNodeDefs_Math(registry);
    RegisterNodeDefs_Debug(registry);
    RegisterNodeDefs_String(registry);
    RegisterNodeDefs_Array(registry);
    RegisterNodeDefs_Map(registry);
    RegisterNodeDefs_Time(registry);
    RegisterNodeDefs_Data(registry);
    RegisterNodeDefs_Tree(registry);
    RegisterNodeDefs_Houdini(registry);
    RegisterNodeDefs_Misc(registry);
    RegisterNodeDefs_Conversion(registry);
    RegisterNodeDefs_Event(registry);
    RegisterNodeDefs_Function(registry);
    RegisterNodeDefs_EventBus(registry);
    RegisterNodeDefs_Network(registry);
    RegisterNodeDefs_AI(registry);
    RegisterNodeDefs_File(registry);
    RegisterNodeDefs_Retry(registry);
    RegisterNodeDefs_Game(registry);
    RegisterNodeDefs_Save(registry);
    RegisterNodeDefs_GameMath(registry);
    RegisterNodeDefs_Socket(registry);
    RegisterNodeDefs_GameExtra(registry);
}

} // namespace Runtime
} // namespace NodeEditor
