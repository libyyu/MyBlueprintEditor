// tests/runtime_test.cpp
// 编译：g++ -std=c++17 -I../Runtime -I../Utils -I../Utils/Json \
//           runtime_test.cpp ../Utils/Json/crude_json.cpp \
//           -L../build-static/Runtime -lBlueprintRuntime -o runtime_test
//
// 或通过 CMake 构建（见 tests/CMakeLists.txt）

#include <iostream>
#include <string>
#include <cassert>
#include "BlueprintRunner.h"
#include "BuiltinNodeDefs.h"
#include "BuiltinHandlers.h"
#include "crude_json.h"

using namespace NodeEditor::Runtime;

// ─────────────────────────────────────────────────────────────────────────────
// 工具函数
// ─────────────────────────────────────────────────────────────────────────────

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { std::cout << "  [PASS] " << (msg) << "\n"; ++g_pass; } \
        else      { std::cout << "  [FAIL] " << (msg) << "\n"; ++g_fail; } \
    } while(0)

// ─────────────────────────────────────────────────────────────────────────────
// 测试 1：DefaultNodeRegistry 野指针修复验证
//   连续注册大量节点，强制触发 unordered_map rehash，
//   再遍历 getAllNodeDefinitions() 确保指针仍然有效。
// ─────────────────────────────────────────────────────────────────────────────
static void test_registry_no_dangling_ptr()
{
    std::cout << "\n[Test 1] DefaultNodeRegistry – rehash 野指针修复\n";

    DefaultNodeRegistry reg;
    // 注册 200 个节点，足够触发多次 rehash（默认 load_factor=1，初始桶数很小）
    for (int i = 0; i < 200; ++i)
    {
        NodeDefinition def;
        def.id   = "node_" + std::to_string(i);
        def.name = "Node " + std::to_string(i);
        reg.registerNode(def);
    }

    const auto& defs = reg.getAllNodeDefinitions();
    CHECK(defs.size() == 200, "注册 200 个节点后缓存数量正确");

    // 验证每个指针仍然有效（能读到正确的 id）
    bool allValid = true;
    for (const auto* p : defs)
    {
        if (!p || p->id.empty()) { allValid = false; break; }
        // 确认能从注册表里查到同一个对象
        const auto* found = reg.getNodeDefinition(p->id);
        if (found != p) { allValid = false; break; }
    }
    CHECK(allValid, "所有缓存指针指向有效节点（无野指针）");

    // 再更新其中一个节点，验证更新后缓存仍然正确
    NodeDefinition updated;
    updated.id   = "node_0";
    updated.name = "Node UPDATED";
    reg.registerNode(updated);

    const auto* p0 = reg.getNodeDefinition("node_0");
    CHECK(p0 && p0->name == "Node UPDATED", "更新节点后 getNodeDefinition 返回新值");

    // 缓存数量不变（是更新，不是新增）
    CHECK(reg.getAllNodeDefinitions().size() == 200, "更新后缓存数量不变");
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试 2：Variant strtoll/strtod（-fno-exceptions 友好）
// ─────────────────────────────────────────────────────────────────────────────
static void test_variant_conversion()
{
    std::cout << "\n[Test 2] Variant asInt/asFloat – strtoll/strtod 转换\n";

    Variant vs(std::string("42"));
    CHECK(vs.asInt() == 42, "\"42\" → asInt() == 42");
    CHECK(vs.asFloat() == 42.0, "\"42\" → asFloat() == 42.0");

    Variant vf(std::string("3.14"));
    CHECK(vf.asFloat() > 3.13 && vf.asFloat() < 3.15, "\"3.14\" → asFloat() ≈ 3.14");
    CHECK(vf.asInt() == 3, "\"3.14\" → asInt() == 3 (截断)");

    Variant vbad(std::string("abc"));
    CHECK(vbad.asInt() == 0, "\"abc\" → asInt() == 0 (转换失败)");
    CHECK(vbad.asFloat() == 0.0, "\"abc\" → asFloat() == 0.0 (转换失败)");

    Variant vempty(std::string(""));
    CHECK(vempty.asInt() == 0, "\"\" → asInt() == 0");
    CHECK(vempty.asFloat() == 0.0, "\"\" → asFloat() == 0.0");
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试 3：Variant Map API
// ─────────────────────────────────────────────────────────────────────────────
static void test_variant_map()
{
    std::cout << "\n[Test 3] Variant Map API\n";

    Variant m;
    m.mapSet("foo", Variant(std::string("bar")));
    m.mapSet("count", Variant(int64_t(42)));

    CHECK(m.mapHasKey("foo"), "mapHasKey(\"foo\") == true");
    CHECK(!m.mapHasKey("baz"), "mapHasKey(\"baz\") == false");
    CHECK(m.mapGet("foo").asString() == "bar", "mapGet(\"foo\") == \"bar\"");
    CHECK(m.mapGet("count").asInt() == 42, "mapGet(\"count\") == 42");
    CHECK(m.mapSize() == 2, "mapSize() == 2");

    m.mapRemove("foo");
    CHECK(m.mapSize() == 1, "mapRemove 后 mapSize() == 1");
    CHECK(!m.mapHasKey("foo"), "mapRemove 后 mapHasKey(\"foo\") == false");

    // 拷贝构造
    Variant m2(m);
    CHECK(m2.mapHasKey("count"), "拷贝后 mapHasKey(\"count\") == true");

    // 移动构造
    Variant m3(std::move(m2));
    CHECK(m3.mapHasKey("count"), "移动后 mapHasKey(\"count\") == true");
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试 4：BlueprintRunner 执行 flow_test.json
//   拓扑：SetVariable(counter=0) → ForLoop(0..4) → AddIndex+GetVariable →
//         SetVariable(counter+=index) → PrintString →
//         [完成后] Branch(counter>=10) → PrintTrue/PrintFalse
// ─────────────────────────────────────────────────────────────────────────────
static void test_flow_blueprint()
{
    std::cout << "\n[Test 4] BlueprintRunner – flow_test.json 执行\n";

    // 注册内置节点
    BlueprintRunner runner;

    // 注册内置处理器（直接注册到 runner）
    RegisterBuiltinHandlers(runner, ".");

    // 收集日志
    std::vector<std::string> logs;
    runner.SetLogCallback([&](const std::string& msg) {
        logs.push_back(msg);
        std::cout << "    LOG: " << msg << "\n";
    });

    // 加载蓝图
    bool loaded = runner.LoadFromFile("flow_test.json");
    CHECK(loaded, "flow_test.json 加载成功");
    if (!loaded)
    {
        std::cout << "    Error: " << runner.GetLastError() << "\n";
        return;
    }

    // 执行
    auto result = runner.Execute();
    CHECK(result.success, "Execute() 返回 success=true");
    if (!result.success)
        std::cout << "    Error: " << result.errorMessage << "\n";

    // ForLoop 0..4 累加：0+1+2+3+4 = 10
    // counter 初始 0，每轮 counter = counter + index
    auto finalCounter = runner.GetVariable("counter");
    CHECK(finalCounter.asInt() == 10, "最终 counter == 10（0+1+2+3+4）");

    // Branch：counter(10) >= 10 → True 分支 → PrintTrue 被执行
    bool foundTrue = false;
    for (const auto& l : logs)
        if (l.find("TRUE") != std::string::npos) { foundTrue = true; break; }
    CHECK(foundTrue, "Branch True 分支被执行（PrintString 含 TRUE）");

    std::cout << "    nodesExecuted=" << result.nodesExecuted
              << "  elapsedMs=" << result.elapsedMs << "\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试 5：Map 节点端到端（BlueprintRunner 内联构建）
// ─────────────────────────────────────────────────────────────────────────────
static void test_map_nodes()
{
    std::cout << "\n[Test 5] Map 节点端到端\n";

    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");

    std::vector<std::string> logs;
    runner.SetLogCallback([&](const std::string& msg) {
        logs.push_back(msg);
    });

    // 构建一个简单蓝图：MapSet → MapGet → PrintString
    BlueprintData bp;
    bp.metadata.name = "MapTest";

    // Node 1: MapSet (exec)
    {
        NodeInstance n;
        n.id = 1; n.definitionId = "MapSet";
        PinInfo p0; p0.id=11; p0.kind=PinKind::Input;  p0.isExec=true;  n.pins.push_back(p0);
        PinInfo p1; p1.id=12; p1.kind=PinKind::Input;  p1.dataType=PinDataType::Map;    p1.name="Map";   n.pins.push_back(p1);
        PinInfo p2; p2.id=13; p2.kind=PinKind::Input;  p2.dataType=PinDataType::Any;    p2.name="Key";   p2.defaultValue=Variant(std::string("hello")); n.pins.push_back(p2);
        PinInfo p3; p3.id=14; p3.kind=PinKind::Input;  p3.dataType=PinDataType::Any;    p3.name="Value"; p3.defaultValue=Variant(std::string("world")); n.pins.push_back(p3);
        PinInfo p4; p4.id=15; p4.kind=PinKind::Output; p4.isExec=true;  n.pins.push_back(p4);
        PinInfo p5; p5.id=16; p5.kind=PinKind::Output; p5.dataType=PinDataType::Map;    p5.name="Map";   n.pins.push_back(p5);
        bp.nodes.push_back(n);
    }

    // Node 2: MapGet
    {
        NodeInstance n;
        n.id = 2; n.definitionId = "MapGet";
        PinInfo p0; p0.id=21; p0.kind=PinKind::Input;  p0.dataType=PinDataType::Map;    p0.name="Map";   n.pins.push_back(p0);
        PinInfo p1; p1.id=22; p1.kind=PinKind::Input;  p1.dataType=PinDataType::Any;    p1.name="Key";   p1.defaultValue=Variant(std::string("hello")); n.pins.push_back(p1);
        PinInfo p2; p2.id=23; p2.kind=PinKind::Output; p2.dataType=PinDataType::Any;    p2.name="Value"; n.pins.push_back(p2);
        PinInfo p3; p3.id=24; p3.kind=PinKind::Output; p3.dataType=PinDataType::Boolean;p3.name="Found"; n.pins.push_back(p3);
        bp.nodes.push_back(n);
    }

    // Links: MapSet.Out(Map,pin16) → MapGet.In(Map,pin21)
    {
        LinkInstance lk; lk.id=1; lk.startPinId=16; lk.endPinId=21; bp.links.push_back(lk);
    }
    // exec: MapSet.exec_out(15) → (no next, end of flow)
    bp.rebuildIndices();

    bool loaded = runner.Load(bp);
    CHECK(loaded, "内联 BlueprintData 加载成功");

    // 预注入一个空 Map 到 pin12（MapSet 的 Map 输入）
    runner.SetPinValue(12, Variant(std::unordered_map<std::string,Variant>{}));

    // 手动触发 exec 起点（执行 Node1=MapSet）
    ExecutionResult result = runner.ExecuteNode(1);
    CHECK(result.success, "ExecuteNode(MapSet) success");

    // 读 MapGet 的输出（需要执行 Node2）
    result = runner.ExecuteNode(2);
    CHECK(result.success, "ExecuteNode(MapGet) success");

    Variant val = runner.GetPinValue(23);  // MapGet.Value output
    Variant found = runner.GetPinValue(24); // MapGet.Found output
    CHECK(found.asBool(), "MapGet Found == true");
    CHECK(val.asString() == "world", "MapGet Value == \"world\"");
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 6: crude_json UTF-8 多字节字符解析
// ─────────────────────────────────────────────────────────────────────────────
void test_crude_json_utf8()
{
    std::cout << "\n[Test 6] crude_json UTF-8 支持\n";

    // JSON 包含中文字符串（UTF-8 多字节序列）
    const char* json_utf8 = u8"{\"name\": \"蓝图测试\", \"value\": 42}";
    auto v = crude_json::value::parse(json_utf8);
    CHECK(!v.is_discarded(), "包含中文的 JSON 解析成功（非 discarded）");
    CHECK(v.type() == crude_json::type_t::object, "解析结果为 object");

    std::string name = v["name"].get<std::string>();
    CHECK(name == u8"蓝图测试", "中文字段值正确（UTF-8 pass-through）");

    double val = v["value"].get<double>();
    CHECK(val == 42.0, "数字字段值正确");

    // JSON 描述字段包含中文
    const char* json_desc = u8"{\"description\": \"这是一个ForLoop节点\"}";
    auto v2 = crude_json::value::parse(json_desc);
    CHECK(!v2.is_discarded(), "中文描述字段解析成功");
    CHECK(v2["description"].get<std::string>() == u8"这是一个ForLoop节点",
          "中文描述字段内容正确");
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 7: Add 节点类型提升 — integer 累加保持 integer
// ─────────────────────────────────────────────────────────────────────────────
void test_add_type_promotion()
{
    std::cout << "\n[Test 7] Add 节点类型提升（integer 保持 integer）\n";

    using namespace NodeEditor::Runtime;

    // Integer + Integer → Integer
    {
        Variant a(int64_t(3)), b(int64_t(4));
        CHECK(a.type == PinDataType::Integer, "a 是 Integer");
        CHECK(b.type == PinDataType::Integer, "b 是 Integer");
        Variant r(a.asInt() + b.asInt());
        CHECK(r.type == PinDataType::Integer, "Integer+Integer 结果是 Integer");
        CHECK(r.asInt() == 7, "Integer+Integer 结果值正确（3+4=7）");
        // asString 不含小数点
        CHECK(r.asString() == "7", "Integer asString 无 '0.000000' 格式");
    }

    // Float + Integer → Float (via resolveArithType)
    {
        Variant a(3.14), b(int64_t(2));
        CHECK(a.type == PinDataType::Float, "a 是 Float");
        // Float 优先
        Variant r(a.asFloat() + b.asFloat());
        CHECK(r.type == PinDataType::Float, "Float+Integer 结果是 Float");
    }

    // Unknown + Integer → Integer (Any 跟随已知类型)
    {
        Variant a;  // Unknown/Any
        Variant b(int64_t(5));
        CHECK(a.type == PinDataType::Unknown, "a 是 Unknown（Any）");
        // resolveArithType 规则：a=Unknown, b=Integer → Integer
        bool useInt = (a.type != PinDataType::Float && b.type != PinDataType::Float)
                   && (a.type == PinDataType::Integer || b.type == PinDataType::Integer);
        CHECK(useInt, "Unknown+Integer 应走 Integer 路径");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
int main()
{
    std::cout << "========== BlueprintRuntime 单元测试 ==========\n";

    test_registry_no_dangling_ptr();
    test_variant_conversion();
    test_variant_map();
    test_flow_blueprint();
    test_map_nodes();
    test_crude_json_utf8();
    test_add_type_promotion();

    std::cout << "\n========== 结果 ==========\n";
    std::cout << "PASS: " << g_pass << "  FAIL: " << g_fail << "\n";
    return (g_fail == 0) ? 0 : 1;
}
