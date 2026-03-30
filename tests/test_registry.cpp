// tests/test_registry.cpp -- DefaultNodeRegistry 测试
#include <gtest/gtest.h>
#include "NodeDefinition.h"

using namespace NodeEditor::Runtime;

// ── 基础注册 / 查找 ──────────────────────────────────────────────────────────

TEST(RegistryTest, RegisterAndFind)
{
    DefaultNodeRegistry reg;
    NodeDefinition def;
    def.id   = "TestNode";
    def.name = "Test Node";
    def.category = "Test";

    EXPECT_TRUE(reg.registerNode(def));
    const NodeDefinition* found = reg.getNodeDefinition("TestNode");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id, "TestNode");
    EXPECT_EQ(found->name, "Test Node");
}

TEST(RegistryTest, FindNonExistent)
{
    DefaultNodeRegistry reg;
    EXPECT_EQ(reg.getNodeDefinition("NoSuchNode"), nullptr);
}

TEST(RegistryTest, RegisterDuplicate)
{
    DefaultNodeRegistry reg;
    NodeDefinition def;
    def.id = "Node"; def.name = "v1";
    reg.registerNode(def);

    def.name = "v2";
    reg.registerNode(def);  // 应覆盖

    const auto* p = reg.getNodeDefinition("Node");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->name, "v2");
    EXPECT_EQ(reg.getAllNodeDefinitions().size(), 1u);
}

TEST(RegistryTest, Unregister)
{
    DefaultNodeRegistry reg;
    NodeDefinition def; def.id = "Node"; def.name = "N";
    reg.registerNode(def);
    EXPECT_TRUE(reg.unregisterNode("Node"));
    EXPECT_FALSE(reg.unregisterNode("Node"));  // 已不存在
    EXPECT_EQ(reg.getNodeDefinition("Node"), nullptr);
}

// ── rehash 野指针修复 ────────────────────────────────────────────────────────

TEST(RegistryTest, NoDanglingPtrAfterRehash)
{
    DefaultNodeRegistry reg;
    // 注册 200 个节点，足够触发 unordered_map 多次 rehash
    for (int i = 0; i < 200; ++i)
    {
        NodeDefinition def;
        def.id   = "node_" + std::to_string(i);
        def.name = "Node " + std::to_string(i);
        reg.registerNode(def);
    }

    const auto& cache = reg.getAllNodeDefinitions();
    EXPECT_EQ(cache.size(), 200u);

    for (const auto* p : cache)
    {
        ASSERT_NE(p, nullptr);
        // 缓存指针必须指向注册表中同一个对象
        const auto* found = reg.getNodeDefinition(p->id);
        EXPECT_EQ(found, p) << "Dangling pointer detected for id=" << p->id;
    }
}

// ── 分类查找 ─────────────────────────────────────────────────────────────────

TEST(RegistryTest, GetNodesByCategory)
{
    DefaultNodeRegistry reg;
    auto make = [](const char* id, const char* cat) {
        NodeDefinition d; d.id = id; d.category = cat; return d;
    };
    reg.registerNode(make("A", "Math"));
    reg.registerNode(make("B", "Math/Add"));
    reg.registerNode(make("C", "Flow"));

    auto mathExact = reg.getNodesByCategory("Math");
    EXPECT_EQ(mathExact.size(), 1u);
    EXPECT_EQ(mathExact[0].id, "A");

    auto mathPrefix = reg.getNodesByCategoryPrefix("Math");
    EXPECT_EQ(mathPrefix.size(), 2u);  // Math + Math/Add

    auto flow = reg.getNodesByCategoryPrefix("Flow");
    EXPECT_EQ(flow.size(), 1u);
}

// ── 类别注册 ─────────────────────────────────────────────────────────────────

TEST(RegistryTest, RegisterCategory)
{
    DefaultNodeRegistry reg;
    NodeCategory cat; cat.id = "Flow"; cat.name = "Flow Control";
    EXPECT_TRUE(reg.registerCategory(cat));
    EXPECT_FALSE(reg.registerCategory(NodeCategory{}));  // 空 id 失败

    auto cats = reg.getAllCategories();
    EXPECT_EQ(cats.size(), 1u);
    EXPECT_EQ(cats[0].name, "Flow Control");
}
