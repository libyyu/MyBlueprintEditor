// test_new_features.cpp
// 新功能专项单元测试：函数系统 / 事件系统 / 字符串池
// 编译方式：与 runtime-example 共用 CMake target，通过 --test-new 参数触发
//
// 用法：runtime-example --test-new

#include <iostream>
#include <string>
#include <vector>
#include <functional>

#include "BlueprintRunner.h"
#include "BlueprintExporter.h"
#include "BuiltinHandlers.h"
#include "EventBus.h"
#include "StringPool.h"
#include "FunctionLibrary.h"

using namespace NodeEditor::Runtime;

// ============================================================================
// 轻量测试框架（与 runtime-example.cpp 一致）
// ============================================================================

static int g_pass_new = 0;
static int g_fail_new = 0;

#define CHECK_NEW(cond, msg) \
    do { \
        if (cond) { std::cout << "  [PASS] " << (msg) << "\n"; ++g_pass_new; } \
        else      { std::cout << "  [FAIL] " << (msg) << "\n"; ++g_fail_new; } \
    } while(0)

// ============================================================================
// Test A: StringPool — 驻留池基本功能
// ============================================================================
static void test_string_pool()
{
    std::cout << "\n[Test A] StringPool — 字符串驻留池\n";

    StringPool::Get().Clear();

    const char* p1 = StringPool::Get().Intern("hello");
    const char* p2 = StringPool::Get().Intern("hello");
    const char* p3 = StringPool::Get().Intern(std::string("hello"));
    CHECK_NEW(p1 == p2, "相同字符串返回相同指针（p1 == p2）");
    CHECK_NEW(p1 == p3, "std::string 驻留与 const char* 结果一致（p1 == p3）");
    CHECK_NEW(std::string(p1) == "hello", "驻留指针内容正确");

    const char* p4 = StringPool::Get().Intern("world");
    CHECK_NEW(p4 != p1, "不同字符串返回不同指针");

    CHECK_NEW(StringPool::Get().Size() == 2, "池大小 == 2");

    // Variant::internedString()
    Variant v(std::string("hello"));
    const char* pv = v.internedString();
    CHECK_NEW(pv == p1, "Variant::internedString() 返回与池一致的指针");
    CHECK_NEW(std::string(pv) == "hello", "Variant::internedString() 内容正确");

    // 空字符串
    const char* pe = StringPool::Get().Intern("");
    CHECK_NEW(pe != nullptr, "空字符串驻留不崩溃");
    CHECK_NEW(std::string(pe) == "", "空字符串驻留内容正确");

    std::cout << "  池大小: " << StringPool::Get().Size() << "\n";
}

// ============================================================================
// Test B: EventBus — 基本发布/订阅
// ============================================================================
static void test_event_bus_basic()
{
    std::cout << "\n[Test B] EventBus — 基本发布/订阅\n";

    EventBus::Get().ClearAll();

    std::vector<std::string> received;

    int id1 = EventBus::Get().Subscribe("player.jump", [&](const std::string& name, const Variant& payload) {
        received.push_back("jump:" + payload.asString());
    });
    CHECK_NEW(id1 > 0, "Subscribe 返回正数 ID");

    EventBus::Get().Fire("player.jump", Variant(std::string("high")));
    CHECK_NEW(received.size() == 1, "Fire 后收到 1 条消息");
    CHECK_NEW(!received.empty() && received[0] == "jump:high", "消息内容正确");

    // 第二个订阅者
    std::vector<std::string> received2;
    int id2 = EventBus::Get().Subscribe("player.jump", [&](const std::string& name, const Variant& payload) {
        received2.push_back("jump2:" + payload.asString());
    });

    EventBus::Get().Fire("player.jump", Variant(std::string("low")));
    CHECK_NEW(received.size() == 2, "两条消息后 received.size() == 2");
    CHECK_NEW(received2.size() == 1, "第二个订阅者收到 1 条消息");

    // Unsubscribe
    EventBus::Get().Unsubscribe(id1);
    EventBus::Get().Fire("player.jump", Variant(std::string("after_unsub")));
    CHECK_NEW(received.size() == 2, "Unsubscribe 后 id1 不再收到消息");
    CHECK_NEW(received2.size() == 2, "id2 仍然收到消息");

    // ClearEvent
    EventBus::Get().ClearEvent("player.jump");
    EventBus::Get().Fire("player.jump", Variant(std::string("cleared")));
    CHECK_NEW(received2.size() == 2, "ClearEvent 后 id2 也不再收到消息");
}

// ============================================================================
// Test C: EventBus — GetRegisteredEvents + 多事件
// ============================================================================
static void test_event_bus_multi()
{
    std::cout << "\n[Test C] EventBus — 多事件 + GetRegisteredEvents\n";

    EventBus::Get().ClearAll();

    int hitCount = 0;
    EventBus::Get().Subscribe("game.start", [&](const std::string&, const Variant&) { hitCount++; });
    EventBus::Get().Subscribe("game.over",  [&](const std::string&, const Variant&) { hitCount += 10; });

    EventBus::Get().Fire("game.start");
    CHECK_NEW(hitCount == 1, "game.start 触发 hitCount == 1");

    EventBus::Get().Fire("game.over");
    CHECK_NEW(hitCount == 11, "game.over 触发 hitCount == 11");

    // 未订阅的事件不崩溃
    EventBus::Get().Fire("game.pause");
    CHECK_NEW(hitCount == 11, "未订阅事件 Fire 不崩溃，hitCount 不变");

    auto events = EventBus::Get().GetRegisteredEvents();
    bool hasStart = false, hasOver = false;
    for (const auto& e : events) {
        if (e == "game.start") hasStart = true;
        if (e == "game.over")  hasOver  = true;
    }
    CHECK_NEW(hasStart, "GetRegisteredEvents 包含 game.start");
    CHECK_NEW(hasOver,  "GetRegisteredEvents 包含 game.over");
}

// ============================================================================
// Test D: EventBus + BlueprintRunner — Event.Fire 节点（直接 ExecuteNode）
// ============================================================================
static void test_event_nodes()
{
    std::cout << "\n[Test D] Event.Fire 节点执行（直接 ExecuteNode）\n";

    EventBus::Get().ClearAll();

    // 先用纯 API 验证订阅
    std::vector<std::string> apiReceived;
    EventBus::Get().Subscribe("blueprint.test", [&](const std::string&, const Variant& p) {
        apiReceived.push_back(p.asString());
    });

    // 构造一个只含 Event.Fire 的蓝图（无 OnBeginPlay，直接 ExecuteNode 触发）
    BlueprintData bp;
    bp.metadata.name = "EventFireTest";

    // Node: Event.Fire
    {
        NodeInstance n;
        n.id = 2; n.definitionId = "Event.Fire";
        PinInfo exec_in;  exec_in.id=21;  exec_in.kind=PinKind::Input;  exec_in.isExec=true;
        PinInfo exec_out; exec_out.id=22; exec_out.kind=PinKind::Output; exec_out.isExec=true;
        PinInfo evname;   evname.id=23;   evname.kind=PinKind::Input;  evname.dataType=PinDataType::String;
        evname.name="EventName"; evname.defaultValue=Variant(std::string("blueprint.test"));
        PinInfo payload;  payload.id=24;  payload.kind=PinKind::Input;  payload.dataType=PinDataType::Any;
        payload.name="Payload"; payload.defaultValue=Variant(std::string("hello_from_node"));
        n.pins.push_back(exec_in);
        n.pins.push_back(exec_out);
        n.pins.push_back(evname);
        n.pins.push_back(payload);
        bp.nodes.push_back(n);
    }
    bp.rebuildIndices();

    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");
    runner.SetLogCallback([](LogLevel, const std::string& m){ std::cout << "    LOG: " << m << "\n"; });

    CHECK_NEW(runner.Load(bp), "Event.Fire 蓝图加载成功");
    // 直接执行 Event.Fire 节点（绕过 event subgraph 过滤）
    auto result = runner.ExecuteNode(2);
    CHECK_NEW(result.success, "Event.Fire 节点 ExecuteNode 成功");

    CHECK_NEW(apiReceived.size() == 1, "Event.Fire 节点触发了 API 订阅者");
    CHECK_NEW(!apiReceived.empty() && apiReceived[0] == "hello_from_node",
              "Event.Fire 节点 payload 传递正确");
}

// ============================================================================
// Test E: FunctionDefinition 数据结构 + JSON 序列化/反序列化
// ============================================================================
static void test_function_serialization()
{
    std::cout << "\n[Test E] FunctionDefinition JSON 序列化/反序列化\n";

    BlueprintData bp;
    bp.metadata.name = "FuncSerialTest";

    // 定义一个函数
    FunctionDefinition fd;
    fd.id = "add_two";
    fd.name = "Add Two Numbers";
    fd.category = "Math";
    fd.description = "Adds A + B and returns Result";
    fd.isPublic = true;

    VariableDefinition inA; inA.name="A"; inA.dataType=PinDataType::Float; inA.defaultValue=Variant(0.0);
    VariableDefinition inB; inB.name="B"; inB.dataType=PinDataType::Float; inB.defaultValue=Variant(0.0);
    VariableDefinition outR; outR.name="Result"; outR.dataType=PinDataType::Float;
    fd.inputs.push_back(inA);
    fd.inputs.push_back(inB);
    fd.outputs.push_back(outR);

    // 函数体：一个 math_add 节点
    NodeInstance addNode;
    addNode.id = 100; addNode.definitionId = "math_add"; addNode.name = "Add";
    fd.nodes.push_back(addNode);

    bp.functions.push_back(fd);

    // 序列化
    JsonBlueprintExporter exporter;
    ExportOptions opts; opts.prettyPrint = true;
    std::string json = exporter.exportRuntimeToString(bp, opts);

    CHECK_NEW(!json.empty(), "序列化结果非空");
    CHECK_NEW(json.find("add_two") != std::string::npos, "序列化结果包含函数 id");
    CHECK_NEW(json.find("Add Two Numbers") != std::string::npos, "序列化结果包含函数 name");
    CHECK_NEW(json.find("functions") != std::string::npos, "序列化结果包含 functions 字段");

    // 反序列化
    BlueprintRunner runner;
    CHECK_NEW(runner.LoadFromJson(json), "含 functions 的 JSON 加载成功");

    const auto& loaded = runner.GetBlueprintData();
    CHECK_NEW(loaded.functions.size() == 1, "反序列化后 functions.size() == 1");
    if (!loaded.functions.empty()) {
        const auto& lf = loaded.functions[0];
        CHECK_NEW(lf.id == "add_two",             "函数 id 正确");
        CHECK_NEW(lf.name == "Add Two Numbers",   "函数 name 正确");
        CHECK_NEW(lf.category == "Math",           "函数 category 正确");
        CHECK_NEW(lf.isPublic == true,             "函数 isPublic 正确");
        CHECK_NEW(lf.inputs.size() == 2,           "函数 inputs.size() == 2");
        CHECK_NEW(lf.outputs.size() == 1,          "函数 outputs.size() == 1");
        CHECK_NEW(lf.nodes.size() == 1,            "函数 nodes.size() == 1");
    }
}

// ============================================================================
// Test F: Function.Call 节点执行（蓝图内函数调用）
// ============================================================================
static void test_function_call_node()
{
    std::cout << "\n[Test F] Function.Call 节点 — 调用蓝图内定义的函数\n";

    // 构造带函数定义的蓝图：
    //   函数 "greet"：包含一个 PrintString 节点，打印 "Hello from function"
    //   主图：BeginPlay → Function.Call("greet")

    BlueprintData bp;
    bp.metadata.name = "FuncCallTest";

    // 定义函数 greet（函数体：PrintString 节点）
    FunctionDefinition fd;
    fd.id = "greet"; fd.name = "Greet"; fd.isPublic = true;
    {
        // 函数入口节点
        NodeInstance entry;
        entry.id = 200; entry.definitionId = "Function.Entry";
        PinInfo out; out.id=2001; out.kind=PinKind::Output; out.isExec=true;
        entry.pins.push_back(out);
        fd.nodes.push_back(entry);

        // PrintString 节点
        NodeInstance print;
        print.id = 201; print.definitionId = "PrintString";
        PinInfo ei;  ei.id=2011;  ei.kind=PinKind::Input;  ei.isExec=true;
        PinInfo eo;  eo.id=2012;  eo.kind=PinKind::Output; eo.isExec=true;
        PinInfo str; str.id=2013; str.kind=PinKind::Input;  str.dataType=PinDataType::String;
        str.name="In String"; str.defaultValue=Variant(std::string("Hello from function"));
        print.pins.push_back(ei);
        print.pins.push_back(eo);
        print.pins.push_back(str);
        fd.nodes.push_back(print);

        // 函数返回节点
        NodeInstance ret;
        ret.id = 202; ret.definitionId = "Function.Return";
        PinInfo ri; ri.id=2021; ri.kind=PinKind::Input; ri.isExec=true;
        ret.pins.push_back(ri);
        fd.nodes.push_back(ret);

        // 链接：Entry.out → Print.in → Return.in
        { LinkInstance lk; lk.id=2001; lk.startPinId=2001; lk.endPinId=2011; fd.links.push_back(lk); }
        { LinkInstance lk; lk.id=2002; lk.startPinId=2012; lk.endPinId=2021; fd.links.push_back(lk); }
    }
    bp.functions.push_back(fd);

    // 主图：只有 Function.Call 节点，直接 ExecuteNode 触发
    {
        NodeInstance call;
        call.id = 2; call.definitionId = "Function.Call";
        PinInfo ei;   ei.id=21;   ei.kind=PinKind::Input;  ei.isExec=true;
        PinInfo eo;   eo.id=22;   eo.kind=PinKind::Output; eo.isExec=true;
        PinInfo fid;  fid.id=23;  fid.kind=PinKind::Input;  fid.dataType=PinDataType::String;
        fid.name="FunctionId"; fid.defaultValue=Variant(std::string("greet"));
        call.pins.push_back(ei);
        call.pins.push_back(eo);
        call.pins.push_back(fid);
        bp.nodes.push_back(call);
    }
    bp.rebuildIndices();

    std::vector<std::string> prints;
    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");
    runner.SetPrintCallback([&](LogLevel, const std::string& m){ prints.push_back(m); std::cout << "    PRINT: " << m << "\n"; });
    runner.SetLogCallback([](LogLevel, const std::string& m){ std::cout << "    LOG: " << m << "\n"; });

    CHECK_NEW(runner.Load(bp), "Function.Call 蓝图加载成功");
    // 直接执行 Function.Call 节点
    auto result = runner.ExecuteNode(2);
    CHECK_NEW(result.success, "Function.Call 节点 ExecuteNode 成功");

    bool foundHello = false;
    for (const auto& p : prints)
        if (p.find("Hello from function") != std::string::npos) { foundHello = true; break; }
    CHECK_NEW(foundHello, "Function.Call 成功调用函数，PrintString 输出正确");
}

// ============================================================================
// Test G: LoadFunctionLibrary — 目录不存在时静默跳过
// ============================================================================
static void test_function_library_missing_dir()
{
    std::cout << "\n[Test G] LoadFunctionLibrary — 目录不存在时静默跳过\n";

    DefaultNodeRegistry reg;
    int count = LoadFunctionLibrary(reg, "nonexistent_dir_xyz_12345");
    CHECK_NEW(count == 0, "不存在的目录返回 0，不崩溃");
}

// ============================================================================
// 测试入口（供 runtime-example main() 调用）
// ============================================================================
// 前向声明（Test H 定义在文件末尾）
static void test_blueprint_class_serialization();

int runNewFeatureTests()
{
    g_pass_new = 0;
    g_fail_new = 0;

    std::cout << "========== 新功能专项测试 (Functions / Events / StringPool) ==========\n";

    test_string_pool();
    test_event_bus_basic();
    test_event_bus_multi();
    test_event_nodes();
    test_function_serialization();
    test_function_call_node();
    test_function_library_missing_dir();
    test_blueprint_class_serialization();

    std::cout << "\n========== 结果 ==========\n";
    std::cout << "PASS: " << g_pass_new << "  FAIL: " << g_fail_new << "\n";
    return (g_fail_new == 0) ? 0 : 1;
}

// ============================================================================
// Test H: BlueprintClass 枚举序列化/反序列化
// ============================================================================
static void test_blueprint_class_serialization()
{
    std::cout << "\n[Test H] BlueprintClass 序列化/反序列化（枚举整数）\n";

    using namespace NodeEditor::Runtime;

    // ── Actor 蓝图序列化 ────────────────────────────────────────────────────
    {
        BlueprintData bp;
        bp.metadata.name = "TestActor";
        bp.metadata.blueprintClass = BlueprintClass::Actor;

        JsonBlueprintExporter ex;
        ExportOptions opts; opts.prettyPrint = false;
        std::string json = ex.exportRuntimeToString(bp, opts);

        // JSON 里应有 "blueprintClass":0
        CHECK_NEW(json.find("\"blueprintClass\":0") != std::string::npos ||
                  json.find("\"blueprintClass\": 0") != std::string::npos,
                  "Actor 序列化为 blueprintClass=0");

        // 反序列化
        BlueprintRunner r;
        CHECK_NEW(r.LoadFromJson(json), "Actor JSON 反序列化成功");
        CHECK_NEW(r.GetBlueprintData().metadata.blueprintClass == BlueprintClass::Actor,
                  "反序列化后 blueprintClass == Actor");
    }

    // ── FunctionLibrary 蓝图序列化 ─────────────────────────────────────────
    {
        BlueprintData bp;
        bp.metadata.name = "TestLib";
        bp.metadata.blueprintClass = BlueprintClass::FunctionLibrary;

        JsonBlueprintExporter ex;
        ExportOptions opts; opts.prettyPrint = false;
        std::string json = ex.exportRuntimeToString(bp, opts);

        CHECK_NEW(json.find("\"blueprintClass\":1") != std::string::npos ||
                  json.find("\"blueprintClass\": 1") != std::string::npos,
                  "FunctionLibrary 序列化为 blueprintClass=1");

        BlueprintRunner r;
        CHECK_NEW(r.LoadFromJson(json), "FunctionLibrary JSON 反序列化成功");
        CHECK_NEW(r.GetBlueprintData().metadata.blueprintClass == BlueprintClass::FunctionLibrary,
                  "反序列化后 blueprintClass == FunctionLibrary");
    }

    // ── 旧文件向后兼容（缺少 blueprintClass 字段）──────────────────────────
    {
        // 构造一个不含 blueprintClass 的旧格式 JSON
        std::string oldJson = R"({"metadata":{"schemaVersion":2,"name":"OldFile","description":"","version":""},"nodes":[],"links":[],"variables":[],"functions":[],"comments":[]})";

        BlueprintRunner r;
        CHECK_NEW(r.LoadFromJson(oldJson), "旧格式 JSON 加载成功");
        CHECK_NEW(r.GetBlueprintData().metadata.blueprintClass == BlueprintClass::Actor,
                  "旧格式默认 blueprintClass == Actor（向后兼容）");
    }

    // ── FunctionLibrary 禁止直接 Execute ───────────────────────────────────
    {
        BlueprintData bp;
        bp.metadata.blueprintClass = BlueprintClass::FunctionLibrary;

        BlueprintRunner r;
        r.Load(bp);
        auto result = r.Execute();
        CHECK_NEW(!result.success, "FunctionLibrary 直接 Execute 返回 failure");
        CHECK_NEW(result.errorMessage.find("FunctionLibrary") != std::string::npos,
                  "错误信息包含 'FunctionLibrary'");
    }
}
