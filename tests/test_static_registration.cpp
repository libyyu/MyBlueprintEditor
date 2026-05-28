// tests/test_static_registration.cpp
//
// 验证 BlueprintRunner::RegisterNodeDef / RegisterHandler 等 8 个方法已改为 static
// 后的进程级行为（v1.x 重构 — 见 commit notes）：
//
//   1. 可直接通过 BlueprintRunner::RegisterX 调用，不需要任何 instance（编译期）
//   2. 注册一次，所有 Runner 都能看到（HandlerRegistry 单例）
//   3. 一个 Runner 析构后，handler 在其他 Runner 中仍然有效（无 GCHandle 误释放）
//   4. UnregisterHandler 是真正的进程级移除
//   5. SetDefaultHandler 仍是 per-runner 状态（不应被全局化）
//
// 这些不变量是 C ABI / C# binding 设计的基础 —— 这里直接在 C++ 层面锁住。

#include <gtest/gtest.h>
#include <atomic>
#include <memory>

#include "BlueprintRunner.h"
#include "BlueprintData.h"
#include "Types.h"
#include "NodeDefinition.h"

using namespace NodeEditor::Runtime;

// ── 测试 fixture ────────────────────────────────────────────────────────────
//
// 每个 test 自带一个 unique handler id（用 GetTestInfo()->name() 派生），
// teardown 时 unregister 自己注册的 id，避免污染后续 test 的全局表。

class StaticRegistrationTest : public ::testing::Test {
protected:
    std::string m_handlerId;
    std::string m_nodeDefId;

    void SetUp() override {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        std::string base = std::string(info->test_suite_name()) + "." + info->name();
        m_handlerId  = "TestHandler__"  + base;
        m_nodeDefId  = "TestNodeDef__"  + base;
    }

    void TearDown() override {
        // 即使 test 中途断言失败也清理
        BlueprintRunner::UnregisterHandler(m_handlerId);
        BlueprintRunner::UnregisterNodeDef(m_nodeDefId);
    }
};

// ── 1. 编译期可直接以类名调用（无 instance） ────────────────────────────────

TEST_F(StaticRegistrationTest, CallableViaClassName_NoInstanceNeeded)
{
    // 这条 test 主要靠编译过 = 通过。如果 RegisterHandler 不是 static，
    // 这里就会编译失败：'cannot call member function without object'。
    BlueprintRunner::RegisterHandler(m_handlerId,
        [](ExecutionContext&) -> bool { return true; });

    EXPECT_TRUE(BlueprintRunner::HasHandler(m_handlerId));

    BlueprintRunner::UnregisterHandler(m_handlerId);
    EXPECT_FALSE(BlueprintRunner::HasHandler(m_handlerId));
}

TEST_F(StaticRegistrationTest, NodeDefCallableViaClassName_NoInstanceNeeded)
{
    NodeDefinition def;
    def.id       = m_nodeDefId;
    def.name     = "Test Node";
    def.category = "Test";

    BlueprintRunner::RegisterNodeDef(def);
    EXPECT_TRUE(BlueprintRunner::HasNodeDef(m_nodeDefId));

    const NodeDefinition* got = BlueprintRunner::GetNodeDef(m_nodeDefId);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->id, m_nodeDefId);
    EXPECT_EQ(got->name, "Test Node");

    BlueprintRunner::UnregisterNodeDef(m_nodeDefId);
    EXPECT_FALSE(BlueprintRunner::HasNodeDef(m_nodeDefId));
}

// ── 2. 进程级共享：注册一次，所有 Runner 都能看到 ───────────────────────────

TEST_F(StaticRegistrationTest, HandlerVisibleAcrossRunners)
{
    BlueprintRunner runnerA;
    BlueprintRunner runnerB;
    BlueprintRunner runnerC;

    BlueprintRunner::RegisterHandler(m_handlerId,
        [](ExecutionContext&) -> bool { return true; });

    // 注意：HasHandler 调用语法可以是 instance 方式（C++ 允许 instance 调 static），
    // 但行为是查全局表 —— 三个 runner 都能看到。
    EXPECT_TRUE(runnerA.HasHandler(m_handlerId));
    EXPECT_TRUE(runnerB.HasHandler(m_handlerId));
    EXPECT_TRUE(runnerC.HasHandler(m_handlerId));

    // 也可以用类名直接查
    EXPECT_TRUE(BlueprintRunner::HasHandler(m_handlerId));
}

TEST_F(StaticRegistrationTest, NodeDefVisibleAcrossRunners)
{
    BlueprintRunner runnerA;
    BlueprintRunner runnerB;

    NodeDefinition def;
    def.id   = m_nodeDefId;
    def.name = "Cross Runner";
    BlueprintRunner::RegisterNodeDef(def);

    EXPECT_TRUE(runnerA.HasNodeDef(m_nodeDefId));
    EXPECT_TRUE(runnerB.HasNodeDef(m_nodeDefId));
    ASSERT_NE(runnerA.GetNodeDef(m_nodeDefId), nullptr);
    ASSERT_NE(runnerB.GetNodeDef(m_nodeDefId), nullptr);

    // 同一对象指针（单例）
    EXPECT_EQ(runnerA.GetNodeDef(m_nodeDefId), runnerB.GetNodeDef(m_nodeDefId));
}

// ── 3. Runner 析构不影响全局 handler ─────────────────────────────────────────

TEST_F(StaticRegistrationTest, HandlerSurvivesRunnerDestruction)
{
    // 用 shared_ptr<atomic<int>> 追踪 handler 是否还能被调用
    auto callCount = std::make_shared<std::atomic<int>>(0);
    BlueprintRunner::RegisterHandler(m_handlerId,
        [callCount](ExecutionContext&) -> bool {
            callCount->fetch_add(1);
            return true;
        });

    {
        // 创建并立即销毁一个 Runner
        BlueprintRunner shortLived;
        EXPECT_TRUE(shortLived.HasHandler(m_handlerId));
    }
    // shortLived 已析构 —— 验证 handler 仍在
    EXPECT_TRUE(BlueprintRunner::HasHandler(m_handlerId));

    // 创建另一个 Runner，加载只含一个该类型节点的 blueprint，执行
    BlueprintRunner runnerB;
    NodeDefinition def;
    def.id = m_nodeDefId;
    def.name = "Custom";
    PinDefinition execIn;  execIn.name = ""; execIn.kind = PinKind::Input;  execIn.isExec = true;
    PinDefinition execOut; execOut.name = ""; execOut.kind = PinKind::Output; execOut.isExec = true;
    def.inputPins.push_back(execIn);
    def.outputPins.push_back(execOut);
    BlueprintRunner::RegisterNodeDef(def);

    // handler 用 m_nodeDefId 这个 def 执行
    BlueprintRunner::RegisterHandler(m_nodeDefId,
        [callCount](ExecutionContext&) -> bool {
            callCount->fetch_add(1);
            return true;
        });

    BlueprintData bp;
    NodeInstance node;
    node.id = 1;
    node.definitionId = m_nodeDefId;
    PinInfo p; p.id = 10; p.kind = PinKind::Input; p.isExec = true;
    node.pins.push_back(p);
    bp.nodes.push_back(node);

    runnerB.Load(bp);
    runnerB.ExecuteNode(1);
    EXPECT_GE(callCount->load(), 1) << "Handler 应在 runnerB 中正常执行";

    // 清理 m_nodeDefId 对应 handler（fixture TearDown 只清 m_handlerId）
    BlueprintRunner::UnregisterHandler(m_nodeDefId);
}

// ── 4. UnregisterHandler 进程级生效 ─────────────────────────────────────────

TEST_F(StaticRegistrationTest, UnregisterIsProcessGlobal)
{
    BlueprintRunner runnerA;
    BlueprintRunner runnerB;

    BlueprintRunner::RegisterHandler(m_handlerId,
        [](ExecutionContext&) -> bool { return true; });
    EXPECT_TRUE(runnerA.HasHandler(m_handlerId));
    EXPECT_TRUE(runnerB.HasHandler(m_handlerId));

    BlueprintRunner::UnregisterHandler(m_handlerId);

    EXPECT_FALSE(runnerA.HasHandler(m_handlerId));
    EXPECT_FALSE(runnerB.HasHandler(m_handlerId));
    EXPECT_FALSE(BlueprintRunner::HasHandler(m_handlerId));
}

// ── 5. SetDefaultHandler 仍是 per-runner（重要的反向不变量） ───────────────

TEST_F(StaticRegistrationTest, SetDefaultHandlerStaysPerRunner)
{
    auto countA = std::make_shared<std::atomic<int>>(0);
    auto countB = std::make_shared<std::atomic<int>>(0);

    BlueprintRunner runnerA;
    BlueprintRunner runnerB;

    runnerA.SetDefaultHandler(
        [countA](ExecutionContext&) -> bool { countA->fetch_add(1); return true; });
    runnerB.SetDefaultHandler(
        [countB](ExecutionContext&) -> bool { countB->fetch_add(1); return true; });

    // 加载一个使用未注册 def 的 blueprint，触发默认 handler
    BlueprintData bp;
    NodeInstance node;
    node.id = 1;
    node.definitionId = "UNREGISTERED__" + m_handlerId;  // 故意不存在
    PinInfo p; p.id = 10; p.kind = PinKind::Input; p.isExec = true;
    node.pins.push_back(p);
    bp.nodes.push_back(node);

    // runnerA 只触发 countA，不触发 countB
    runnerA.Load(bp);
    runnerA.ExecuteNode(1);
    EXPECT_GE(countA->load(), 1) << "runnerA 的默认 handler 应被调用";
    EXPECT_EQ(countB->load(), 0) << "runnerB 的默认 handler 不应被 runnerA 调用";

    // runnerB 同样只触发 countB
    runnerB.Load(bp);
    runnerB.ExecuteNode(1);
    EXPECT_GE(countB->load(), 1) << "runnerB 的默认 handler 应被调用";
}

// ── 6. RegisterHandlers 批量注册也是 static & 全局 ─────────────────────────

TEST_F(StaticRegistrationTest, RegisterHandlersBatchIsGlobal)
{
    std::string idA = m_handlerId + "_A";
    std::string idB = m_handlerId + "_B";

    std::unordered_map<std::string, NodeHandler> batch;
    batch[idA] = [](ExecutionContext&) -> bool { return true; };
    batch[idB] = [](ExecutionContext&) -> bool { return true; };

    BlueprintRunner::RegisterHandlers(batch);

    EXPECT_TRUE(BlueprintRunner::HasHandler(idA));
    EXPECT_TRUE(BlueprintRunner::HasHandler(idB));

    BlueprintRunner::UnregisterHandler(idA);
    BlueprintRunner::UnregisterHandler(idB);
}
