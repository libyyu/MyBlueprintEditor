// tests/test_serialization.cpp -- BlueprintExporter 序列化/反序列化测试
#include <gtest/gtest.h>
#include "BlueprintData.h"
#include "BlueprintExporter.h"

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
