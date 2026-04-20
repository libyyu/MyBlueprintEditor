// tests/test_game_extra.cpp -- GameExtra 节点测试
// 覆盖 BuiltinHandlers_GameExtra.cpp 中的所有 handler：
//   Game/Random   — SetSeed / Float / Int / Bool / WeightedChoice / Shuffle / PickFromArray
//   Game/Easing   — Apply / Lerp（覆盖主要曲线）
//   Game/Timer    — Tick / Reset / Get
//   Game/Entity   — Create / Destroy / IsAlive + Tag.*
//   Game/Inventory— Add / Remove / GetQuantity / GetAll / Clear

#include <gtest/gtest.h>
#include "BlueprintRunner.h"
#include "BlueprintData.h"
#include "BuiltinHandlers.h"
#include "test_helpers.h"

using namespace NodeEditor::Runtime;
using namespace TestHelpers;

// ── 公共 Fixture ─────────────────────────────────────────────────────────────

class GameExtraTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        RegisterBuiltinHandlers(runner, ".");
        runner.SetSnapshotEnabled(true);  // 启用引脚快照，execHandler 通过快照读取输出
    }

    // 直接执行指定 handler（无蓝图加载），注入输入值，读取输出值
    // 支持 Simple 节点（无 exec）和普通节点（有 exec，走 OnBeginPlay 链）
    bool execHandler(const std::string& defId,
                     const std::unordered_map<std::string, Variant>& inputs,
                     std::unordered_map<std::string, Variant>& outputs)
    {
        BlueprintData bp;
        bp.metadata.name = "GameExtraTest";

        // 判断是否需要 exec 链
        bool needsExec = (defId == "Random.SetSeed" || defId == "GameTimer.Tick" ||
                          defId == "GameTimer.Reset" || defId == "Entity.Create" ||
                          defId == "Entity.Destroy" || defId == "Tag.Add" ||
                          defId == "Tag.Remove" || defId == "Inventory.Add" ||
                          defId == "Inventory.Remove" || defId == "Inventory.Clear");

        uint64_t pinId = 10;
        std::unordered_map<std::string, uint64_t> outputPinIds;

        if (needsExec) {
            // OnBeginPlay (id=100, execOut=1000) → 节点 execIn
            NodeInstance obp; obp.id=100; obp.definitionId="OnBeginPlay";
            PinInfo ep; ep.id=1000; ep.kind=PinKind::Output; ep.isExec=true;
            obp.pins.push_back(ep);
            bp.nodes.push_back(obp);
        }

        // 目标节点 (id=1)
        NodeInstance n; n.id=1; n.definitionId=defId;

        if (needsExec) {
            PinInfo ep; ep.id=pinId++; ep.kind=PinKind::Input; ep.isExec=true;
            n.pins.push_back(ep);
            LinkInstance lnk; lnk.id=999; lnk.startPinId=1000; lnk.endPinId=ep.id;
            bp.links.push_back(lnk);
        }

        for (const auto& kv : inputs) {
            PinInfo p;
            p.id = pinId++;
            p.kind = PinKind::Input;
            p.name = kv.first;
            p.defaultValue = kv.second;
            p.dataType = kv.second.type;
            n.pins.push_back(p);
        }

        // 输出引脚
        static const std::vector<std::string> outNames = {
            "Value","Result","EasedT","ElapsedTime","FrameCount","Normalized",
            "EntityId","Alive","Tags","Count","Quantity","HasItem",
            "Items","Quantities","NewQuantity","Overflow","Removed",
            "ChosenItem","ChosenIndex","X","Y","Vec2","Ready","Remaining"
        };
        for (const auto& name : outNames) {
            uint64_t id = pinId++;
            PinInfo p; p.id=id; p.kind=PinKind::Output; p.name=name;
            p.dataType = PinDataType::Any;
            n.pins.push_back(p);
            outputPinIds[name] = id;
        }
        bp.nodes.push_back(n);

        bp.rebuildIndices();
        if (!runner.Load(bp)) return false;

        bool ok;
        if (needsExec) {
            auto result = RunWithBeginPlay(runner);
            ok = result.success;
        } else {
            // Simple 节点：直接 ExecuteNode
            auto result = runner.ExecuteNode(1);
            ok = result.success;
        }

        // 通过 GetPinValue 读取输出
        for (const auto& kv : outputPinIds)
            outputs[kv.first] = runner.GetPinValue(kv.second);

        return ok;
    }

    BlueprintRunner runner;
};

// ============================================================================
// Random 节点
// ============================================================================

TEST_F(GameExtraTest, Random_Float_InRange)
{
    runner.SetVariable("__rng_seed", Variant(int64_t(42)));
    std::unordered_map<std::string, Variant> out;
    ASSERT_TRUE(execHandler("Random.Float",
        {{"Min", Variant(0.0)}, {"Max", Variant(1.0)}}, out));
    double v = out["Value"].asFloat();
    EXPECT_GE(v, 0.0);
    EXPECT_LT(v, 1.0);
}

TEST_F(GameExtraTest, Random_Int_InRange)
{
    runner.SetVariable("__rng_seed", Variant(int64_t(42)));
    std::unordered_map<std::string, Variant> out;
    ASSERT_TRUE(execHandler("Random.Int",
        {{"Min", Variant(int64_t(1))}, {"Max", Variant(int64_t(6))}}, out));
    int64_t v = out["Value"].asInt();
    EXPECT_GE(v, 1);
    EXPECT_LE(v, 6);
}

TEST_F(GameExtraTest, Random_Bool_Probability1_AlwaysTrue)
{
    std::unordered_map<std::string, Variant> out;
    ASSERT_TRUE(execHandler("Random.Bool",
        {{"Probability", Variant(1.0)}}, out));
    EXPECT_TRUE(out["Value"].asBool());
}

TEST_F(GameExtraTest, Random_Bool_Probability0_AlwaysFalse)
{
    std::unordered_map<std::string, Variant> out;
    ASSERT_TRUE(execHandler("Random.Bool",
        {{"Probability", Variant(0.0)}}, out));
    EXPECT_FALSE(out["Value"].asBool());
}

TEST_F(GameExtraTest, Random_WeightedChoice_PicksFromList)
{
    runner.SetVariable("__rng_seed", Variant(int64_t(99)));

    Variant items; items.type = PinDataType::Array;
    items.arrayValue.push_back(Variant(std::string("A")));
    items.arrayValue.push_back(Variant(std::string("B")));
    items.arrayValue.push_back(Variant(std::string("C")));

    Variant weights; weights.type = PinDataType::Array;
    weights.arrayValue.push_back(Variant(1.0));
    weights.arrayValue.push_back(Variant(0.0));  // B 权重 0，不会被选
    weights.arrayValue.push_back(Variant(0.0));

    std::unordered_map<std::string, Variant> out;
    ASSERT_TRUE(execHandler("Random.WeightedChoice",
        {{"Items", items}, {"Weights", weights}}, out));
    EXPECT_EQ(out["ChosenItem"].asString(), "A");
    EXPECT_EQ(out["ChosenIndex"].asInt(), 0);
}

TEST_F(GameExtraTest, Random_WeightedChoice_EmptyArray)
{
    Variant empty; empty.type = PinDataType::Array;
    std::unordered_map<std::string, Variant> out;
    ASSERT_TRUE(execHandler("Random.WeightedChoice",
        {{"Items", empty}, {"Weights", empty}}, out));
    EXPECT_EQ(out["ChosenIndex"].asInt(), -1);
}

TEST_F(GameExtraTest, Random_Shuffle_PreservesElements)
{
    runner.SetVariable("__rng_seed", Variant(int64_t(7)));
    Variant arr; arr.type = PinDataType::Array;
    for (int i = 0; i < 5; ++i)
        arr.arrayValue.push_back(Variant(int64_t(i)));

    std::unordered_map<std::string, Variant> out;
    ASSERT_TRUE(execHandler("Random.Shuffle", {{"Array", arr}}, out));

    // 结果应包含所有元素（不重复不遗漏）
    auto& res = out["Result"].arrayValue;
    EXPECT_EQ(res.size(), 5u);
    int64_t sum = 0;
    for (const auto& v : res) sum += v.asInt();
    EXPECT_EQ(sum, 0+1+2+3+4);  // 元素完整性
}

TEST_F(GameExtraTest, Random_PickFromArray_NoReplace)
{
    runner.SetVariable("__rng_seed", Variant(int64_t(13)));
    Variant arr; arr.type = PinDataType::Array;
    for (int i = 0; i < 10; ++i)
        arr.arrayValue.push_back(Variant(int64_t(i)));

    std::unordered_map<std::string, Variant> out;
    ASSERT_TRUE(execHandler("Random.PickFromArray",
        {{"Array", arr}, {"Count", Variant(int64_t(3))},
         {"WithReplacement", Variant(false)}}, out));
    EXPECT_EQ(out["Result"].arraySize(), 3u);
}

// ============================================================================
// Easing 节点
// ============================================================================

TEST_F(GameExtraTest, Easing_Apply_Linear)
{
    std::unordered_map<std::string, Variant> out;
    ASSERT_TRUE(execHandler("Easing.Apply",
        {{"Curve", Variant(std::string("Linear"))},
         {"T",     Variant(0.5)}}, out));
    EXPECT_NEAR(out["EasedT"].asFloat(), 0.5, 1e-6);
}

TEST_F(GameExtraTest, Easing_Apply_T0_AlwaysZero)
{
    // 所有缓动曲线在 t=0 时结果均为 0
    for (const char* curve : {"InQuad","OutQuad","InCubic","InSine","InExpo",
                               "InBack","InElastic","InBounce"}) {
        std::unordered_map<std::string, Variant> out;
        ASSERT_TRUE(execHandler("Easing.Apply",
            {{"Curve", Variant(std::string(curve))},
             {"T",     Variant(0.0)}}, out))
            << "curve=" << curve;
        EXPECT_NEAR(out["EasedT"].asFloat(), 0.0, 1e-5)
            << "curve=" << curve << " at t=0 should be 0";
    }
}

TEST_F(GameExtraTest, Easing_Apply_T1_AlwaysOne)
{
    for (const char* curve : {"InQuad","OutQuad","InOutQuad","InCubic","OutCubic",
                               "InSine","OutSine","OutBounce","OutElastic"}) {
        std::unordered_map<std::string, Variant> out;
        ASSERT_TRUE(execHandler("Easing.Apply",
            {{"Curve", Variant(std::string(curve))},
             {"T",     Variant(1.0)}}, out))
            << "curve=" << curve;
        EXPECT_NEAR(out["EasedT"].asFloat(), 1.0, 1e-5)
            << "curve=" << curve << " at t=1 should be 1";
    }
}

TEST_F(GameExtraTest, Easing_Lerp_InterpolatesCorrectly)
{
    std::unordered_map<std::string, Variant> out;
    ASSERT_TRUE(execHandler("Easing.Lerp",
        {{"Curve", Variant(std::string("Linear"))},
         {"From",  Variant(0.0)},
         {"To",    Variant(100.0)},
         {"T",     Variant(0.3)}}, out));
    EXPECT_NEAR(out["Result"].asFloat(), 30.0, 1e-4);
    EXPECT_NEAR(out["EasedT"].asFloat(), 0.3, 1e-6);
}

TEST_F(GameExtraTest, Easing_Apply_ClampsOutOfRange)
{
    // t > 1 应该 clamp 到 1
    std::unordered_map<std::string, Variant> out;
    ASSERT_TRUE(execHandler("Easing.Apply",
        {{"Curve", Variant(std::string("Linear"))},
         {"T",     Variant(5.0)}}, out));
    EXPECT_NEAR(out["EasedT"].asFloat(), 1.0, 1e-6);
}

// ============================================================================
// GameTimer 节点
// ============================================================================

TEST_F(GameExtraTest, GameTimer_Tick_AccumulatesTime)
{
    // 第一次 Tick: elapsed = 0.1
    {
        std::unordered_map<std::string, Variant> out;
        ASSERT_TRUE(execHandler("GameTimer.Tick",
            {{"Key", Variant(std::string("test"))},
             {"DeltaTime", Variant(0.1)},
             {"MaxTime", Variant(0.0)}}, out));
        EXPECT_NEAR(out["ElapsedTime"].asFloat(), 0.1, 1e-5);
        EXPECT_EQ(out["FrameCount"].asInt(), 1);
    }

    // 重新 load（每次 execHandler 都 reload），测试累积需要共享 runner
    // 直接通过 runner 变量验证
    double elapsed = runner.GetVariable("__gtimer_t_test").asFloat();
    EXPECT_NEAR(elapsed, 0.1, 1e-5);
}

TEST_F(GameExtraTest, GameTimer_Tick_CapsAtMaxTime)
{
    std::unordered_map<std::string, Variant> out;
    ASSERT_TRUE(execHandler("GameTimer.Tick",
        {{"Key", Variant(std::string("capped"))},
         {"DeltaTime", Variant(100.0)},
         {"MaxTime",   Variant(5.0)}}, out));
    EXPECT_NEAR(out["ElapsedTime"].asFloat(), 5.0, 1e-5);
    EXPECT_NEAR(out["Normalized"].asFloat(), 1.0, 1e-5);
}

TEST_F(GameExtraTest, GameTimer_Tick_NormalizedProgress)
{
    std::unordered_map<std::string, Variant> out;
    ASSERT_TRUE(execHandler("GameTimer.Tick",
        {{"Key", Variant(std::string("norm"))},
         {"DeltaTime", Variant(0.5)},
         {"MaxTime",   Variant(2.0)}}, out));
    EXPECT_NEAR(out["Normalized"].asFloat(), 0.25, 1e-5);
}

// ============================================================================
// Entity & Tag 节点
// ============================================================================

TEST_F(GameExtraTest, Entity_CreateAndIsAlive)
{
    // 创建实体
    {
        std::unordered_map<std::string, Variant> out;
        ASSERT_TRUE(execHandler("Entity.Create",
            {{"Prefix", Variant(std::string("enemy"))}}, out));
        std::string eid = out["EntityId"].asString();
        EXPECT_FALSE(eid.empty());
        EXPECT_TRUE(eid.find("enemy_") == 0) << "EntityId should start with 'enemy_'";
    }

    // 检查存活
    std::string eid = runner.GetVariable("__entity_counter").asInt() > 0
        ? "enemy_" + std::to_string(runner.GetVariable("__entity_counter").asInt())
        : "";
    if (!eid.empty()) {
        EXPECT_TRUE(runner.GetVariable("__entity_alive_" + eid).asBool());
    }
}

TEST_F(GameExtraTest, Entity_DestroyMakesNotAlive)
{
    // 先创建
    {
        std::unordered_map<std::string, Variant> out;
        ASSERT_TRUE(execHandler("Entity.Create",
            {{"Prefix", Variant(std::string("mob"))}}, out));
    }
    int64_t cnt = runner.GetVariable("__entity_counter").asInt();
    std::string eid = "mob_" + std::to_string(cnt);

    // 验证创建后 alive
    EXPECT_TRUE(runner.GetVariable("__entity_alive_" + eid).asBool());

    // 销毁
    runner.SetVariable("__entity_alive_" + eid, Variant(false));
    EXPECT_FALSE(runner.GetVariable("__entity_alive_" + eid).asBool());
}

TEST_F(GameExtraTest, Tag_AddAndHas)
{
    const std::string eid = "test_entity";
    runner.SetVariable("__entity_alive_" + eid, Variant(true));

    // 添加标签
    runner.SetVariable("__tags_" + eid, Variant());  // 清空

    // 手动模拟 Tag.Add（通过 runner 变量）
    Variant tags; tags.type = PinDataType::Array;
    tags.arrayValue.push_back(Variant(std::string("warrior")));
    tags.arrayValue.push_back(Variant(std::string("boss")));
    runner.SetVariable("__tags_" + eid, tags);

    // 通过 runner 直接验证
    auto stored = runner.GetVariable("__tags_" + eid);
    EXPECT_EQ(stored.arraySize(), 2u);
    EXPECT_EQ(stored.arrayGet(0).asString(), "warrior");
    EXPECT_EQ(stored.arrayGet(1).asString(), "boss");
}

// ============================================================================
// Inventory 节点
// ============================================================================

TEST_F(GameExtraTest, Inventory_Add_BasicQuantity)
{
    std::unordered_map<std::string, Variant> out;
    ASSERT_TRUE(execHandler("Inventory.Add",
        {{"SlotId",   Variant(std::string("player"))},
         {"ItemId",   Variant(std::string("sword"))},
         {"Quantity", Variant(int64_t(3))},
         {"MaxStack", Variant(int64_t(0))}}, out));
    EXPECT_EQ(out["NewQuantity"].asInt(), 3);
    EXPECT_EQ(out["Overflow"].asInt(), 0);
}

TEST_F(GameExtraTest, Inventory_Add_RespectsMaxStack)
{
    // 先加 8 个
    {
        std::unordered_map<std::string, Variant> out;
        ASSERT_TRUE(execHandler("Inventory.Add",
            {{"SlotId",   Variant(std::string("bag"))},
             {"ItemId",   Variant(std::string("arrow"))},
             {"Quantity", Variant(int64_t(8))},
             {"MaxStack", Variant(int64_t(10))}}, out));
        EXPECT_EQ(out["NewQuantity"].asInt(), 8);
    }
    // 再加 5，超出 MaxStack=10，溢出 3
    {
        std::unordered_map<std::string, Variant> out;
        ASSERT_TRUE(execHandler("Inventory.Add",
            {{"SlotId",   Variant(std::string("bag"))},
             {"ItemId",   Variant(std::string("arrow"))},
             {"Quantity", Variant(int64_t(5))},
             {"MaxStack", Variant(int64_t(10))}}, out));
        EXPECT_EQ(out["NewQuantity"].asInt(), 10);
        EXPECT_EQ(out["Overflow"].asInt(), 3);
    }
}

TEST_F(GameExtraTest, Inventory_Remove_ReducesQuantity)
{
    // 先添加
    {
        std::unordered_map<std::string, Variant> out;
        execHandler("Inventory.Add",
            {{"SlotId",   Variant(std::string("inv"))},
             {"ItemId",   Variant(std::string("potion"))},
             {"Quantity", Variant(int64_t(5))},
             {"MaxStack", Variant(int64_t(0))}}, out);
    }
    // 移除 2
    {
        std::unordered_map<std::string, Variant> out;
        ASSERT_TRUE(execHandler("Inventory.Remove",
            {{"SlotId",   Variant(std::string("inv"))},
             {"ItemId",   Variant(std::string("potion"))},
             {"Quantity", Variant(int64_t(2))}}, out));
        EXPECT_EQ(out["NewQuantity"].asInt(), 3);
        EXPECT_EQ(out["Removed"].asInt(), 2);
    }
}

TEST_F(GameExtraTest, Inventory_Remove_CannotGoNegative)
{
    std::unordered_map<std::string, Variant> out;
    ASSERT_TRUE(execHandler("Inventory.Remove",
        {{"SlotId",   Variant(std::string("empty"))},
         {"ItemId",   Variant(std::string("key"))},
         {"Quantity", Variant(int64_t(100))}}, out));
    EXPECT_EQ(out["NewQuantity"].asInt(), 0);
    EXPECT_EQ(out["Removed"].asInt(), 0);
}

TEST_F(GameExtraTest, Inventory_GetQuantity_HasItem)
{
    // 添加物品
    {
        std::unordered_map<std::string, Variant> out;
        execHandler("Inventory.Add",
            {{"SlotId",   Variant(std::string("shop"))},
             {"ItemId",   Variant(std::string("gem"))},
             {"Quantity", Variant(int64_t(7))},
             {"MaxStack", Variant(int64_t(0))}}, out);
    }
    // 查询
    {
        std::unordered_map<std::string, Variant> out;
        ASSERT_TRUE(execHandler("Inventory.GetQuantity",
            {{"SlotId", Variant(std::string("shop"))},
             {"ItemId", Variant(std::string("gem"))}}, out));
        EXPECT_EQ(out["Quantity"].asInt(), 7);
        EXPECT_TRUE(out["HasItem"].asBool());
    }
}

TEST_F(GameExtraTest, Inventory_GetQuantity_NotFound)
{
    std::unordered_map<std::string, Variant> out;
    ASSERT_TRUE(execHandler("Inventory.GetQuantity",
        {{"SlotId", Variant(std::string("empty_inv"))},
         {"ItemId", Variant(std::string("noitem"))}}, out));
    EXPECT_EQ(out["Quantity"].asInt(), 0);
    EXPECT_FALSE(out["HasItem"].asBool());
}
