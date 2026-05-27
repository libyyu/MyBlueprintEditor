// tests/test_serialization.cpp -- BlueprintExporter 序列化/反序列化测试
#include <gtest/gtest.h>
#include "BlueprintData.h"
#include "BlueprintExporter.h"
#include "../Utils/Json/crude_json.h"

using namespace NodeEditor::Runtime;

// 辅助：构建一个最小有效蓝图
static BlueprintData makeSampleBP()
{
    BlueprintData bp;
    bp.metadata.name = "TestBlueprint";
    bp.metadata.blueprintClass = BlueprintClass::Actor;
    bp.metadata.schemaVersion  = BLUEPRINT_CURRENT_SCHEMA_VERSION;

    NodeInstance n;
    n.id = 1;
    n.definitionId = "PrintString";
    PinInfo pin0; pin0.id = 10; pin0.kind = PinKind::Input; pin0.isExec = true;
    n.pins.push_back(pin0);
    PinInfo pin1; pin1.id = 11; pin1.kind = PinKind::Input;
    pin1.dataType = PinDataType::String; pin1.name = "String";
    pin1.defaultValue = Variant(std::string("hello"));
    n.pins.push_back(pin1);
    bp.nodes.push_back(n);

    VariableDefinition var;
    var.name = "counter"; var.dataType = PinDataType::Integer;
    bp.variables.push_back(var);

    bp.rebuildIndices();
    return bp;
}

// ── Runtime JSON 往返 ────────────────────────────────────────────────────────

TEST(SerializationTest, RuntimeRoundTrip)
{
    auto bp = makeSampleBP();
    JsonBlueprintExporter exporter;

    // 导出到 JSON 字符串（直接返回 string）
    std::string json = exporter.exportRuntimeToString(bp);
    ASSERT_FALSE(json.empty()) << "Export should produce non-empty JSON";

    // 反序列化
    ImportResult result = exporter.importRuntimeFromString(json);
    ASSERT_TRUE(result.success) << result.errorMessage;

    EXPECT_EQ(result.data.metadata.name, "TestBlueprint");
    ASSERT_EQ(result.data.nodes.size(), 1u);
    EXPECT_EQ(result.data.nodes[0].definitionId, "PrintString");
    ASSERT_EQ(result.data.variables.size(), 1u);
    EXPECT_EQ(result.data.variables[0].name, "counter");
}

// ── 变量默认值往返 ───────────────────────────────────────────────────────────

TEST(SerializationTest, VariableDefaultValues)
{
    BlueprintData bp;
    bp.metadata.name = "VarTest";

    auto makeVar = [](const char* name, PinDataType dt, Variant dv) {
        VariableDefinition v; v.name = name; v.dataType = dt; v.defaultValue = dv; return v;
    };
    bp.variables.push_back(makeVar("intVar",   PinDataType::Integer, Variant(int64_t(42))));
    bp.variables.push_back(makeVar("floatVar", PinDataType::Float,   Variant(3.14)));
    bp.variables.push_back(makeVar("strVar",   PinDataType::String,  Variant(std::string("hello"))));
    bp.variables.push_back(makeVar("boolVar",  PinDataType::Boolean, Variant(true)));

    JsonBlueprintExporter exporter;
    std::string json = exporter.exportRuntimeToString(bp);
    ASSERT_FALSE(json.empty());

    ImportResult result = exporter.importRuntimeFromString(json);
    ASSERT_TRUE(result.success) << result.errorMessage;

    const auto& vars = result.data.variables;
    ASSERT_EQ(vars.size(), 4u);
    EXPECT_EQ(vars[0].defaultValue.asInt(),    42);
    EXPECT_NEAR(vars[1].defaultValue.asFloat(), 3.14, 0.001);
    EXPECT_EQ(vars[2].defaultValue.asString(), "hello");
    EXPECT_EQ(vars[3].defaultValue.asBool(),   true);
}

// ── 空蓝图不崩溃 ─────────────────────────────────────────────────────────────

TEST(SerializationTest, EmptyBlueprintRoundTrip)
{
    BlueprintData bp;
    bp.metadata.name = "Empty";

    JsonBlueprintExporter exporter;
    std::string json = exporter.exportRuntimeToString(bp);
    EXPECT_FALSE(json.empty());

    ImportResult result = exporter.importRuntimeFromString(json);
    EXPECT_TRUE(result.success) << result.errorMessage;
    EXPECT_EQ(result.data.nodes.size(), 0u);
    EXPECT_EQ(result.data.links.size(), 0u);
}

// ── schema 版本字段保留 ───────────────────────────────────────────────────────

TEST(SerializationTest, SchemaVersionPreserved)
{
    auto bp = makeSampleBP();
    JsonBlueprintExporter exporter;
    std::string json = exporter.exportRuntimeToString(bp);
    ASSERT_FALSE(json.empty());

    EXPECT_NE(json.find("schemaVersion"), std::string::npos)
        << "JSON must contain schemaVersion field";

    ImportResult result = exporter.importRuntimeFromString(json);
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.data.metadata.schemaVersion, BLUEPRINT_CURRENT_SCHEMA_VERSION);
}

// ── 节点 pins 默认值往返 ─────────────────────────────────────────────────────

TEST(SerializationTest, PinDefaultValueRoundTrip)
{
    BlueprintData bp;
    bp.metadata.name = "PinTest";

    NodeInstance n;
    n.id = 1; n.definitionId = "Add";
    PinInfo p; p.id = 10; p.kind = PinKind::Input;
    p.dataType = PinDataType::Integer; p.name = "A";
    p.defaultValue = Variant(int64_t(99));
    n.pins.push_back(p);
    bp.nodes.push_back(n);

    JsonBlueprintExporter exporter;
    std::string json = exporter.exportRuntimeToString(bp);
    ASSERT_FALSE(json.empty());

    ImportResult result = exporter.importRuntimeFromString(json);
    ASSERT_TRUE(result.success) << result.errorMessage;
    ASSERT_EQ(result.data.nodes.size(), 1u);
    ASSERT_EQ(result.data.nodes[0].pins.size(), 1u);
    EXPECT_EQ(result.data.nodes[0].pins[0].defaultValue.asInt(), 99);
}

// ── 损坏 JSON 不崩溃 ─────────────────────────────────────────────────────────

TEST(SerializationTest, MalformedJsonDoesNotCrash)
{
    JsonBlueprintExporter exporter;
    ImportResult result = exporter.importRuntimeFromString("{invalid json{{");
    EXPECT_FALSE(result.success);  // 应返回失败，而不是崩溃
}

// ── UTF-8 / 中文字符串字段 ─────────────────────────────────────────────────────
// (Migrated from legacy tests/runtime_test.cpp::test_crude_json_utf8)
// 验证 crude_json 作为底层 JSON 库能正确 pass-through 中文等多字节序列
// （蓝图节点描述、变量名等会出现中文）。
//
// 注意：所有 \xHH 转义后用 "" 断开字符串字面量，避免 MSVC 把 \x 转义
// 与后续 hex/ASCII 字符合并（如 \xaaF -> \xaaf -> 超大字符 C2022）。

// "蓝图测试" 的 UTF-8 字节序列
#define UTF8_LANTU_CESHI \
    "\xe8\x93\x9d" "\xe5\x9b\xbe" "\xe6\xb5\x8b" "\xe8\xaf\x95"

// "这是一个ForLoop节点" 的 UTF-8 字节序列
#define UTF8_FORLOOP_DESC \
    "\xe8\xbf\x99" "\xe6\x98\xaf" "\xe4\xb8\x80" "\xe4\xb8\xaa" "ForLoop" "\xe8\x8a\x82" "\xe7\x82\xb9"

TEST(SerializationTest, CrudeJsonParsesChineseStrings)
{
    const char* json_utf8 = "{\"name\": \"" UTF8_LANTU_CESHI "\", \"value\": 42}";
    auto v = crude_json::value::parse(json_utf8);
    ASSERT_FALSE(v.is_discarded()) << "包含中文的 JSON 应解析成功";
    ASSERT_EQ(v.type(), crude_json::type_t::object);

    auto& nameVal = v["name"];
    ASSERT_EQ(nameVal.type(), crude_json::type_t::string);
    EXPECT_EQ(nameVal.get<std::string>(), std::string(UTF8_LANTU_CESHI));

    auto& valNum = v["value"];
    ASSERT_EQ(valNum.type(), crude_json::type_t::number);
    EXPECT_DOUBLE_EQ(valNum.get<double>(), 42.0);
}

TEST(SerializationTest, CrudeJsonHandlesChineseDescriptionField)
{
    const char* json_desc = "{\"description\": \"" UTF8_FORLOOP_DESC "\"}";
    auto v = crude_json::value::parse(json_desc);
    ASSERT_FALSE(v.is_discarded());
    EXPECT_EQ(v["description"].get<std::string>(), std::string(UTF8_FORLOOP_DESC));
}

#undef UTF8_LANTU_CESHI
#undef UTF8_FORLOOP_DESC
