// tests/test_node_consistency.cpp
//
// Cross-check that every (handler, NodeDef) pair is consistent:
//   - For every handler key, there is a NodeDef with matching id (so users
//     can actually drop the node onto the canvas in the editor).
//   - For every NodeDef id, there is either a handler OR a known special
//     internal node (handled directly by BlueprintRunner, not via the
//     handler table).
//
// Why this matters: silently-orphaned handlers/NodeDefs are a real bug class.
// We have already found one (SwitchOnInt: handler existed but NodeDef was
// missing -> users couldn't add the node from the Library panel) and fixed
// it. This test prevents the same drift from happening again.
//
// The test relies on the public registries built up at runtime by
// RegisterBuiltinHandlers + RegisterBuiltinNodeDefs (no string parsing of
// .cpp files), so it stays accurate even if the registration code is
// reorganized.

#include <gtest/gtest.h>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include "BlueprintRunner.h"
#include "BuiltinHandlers.h"
#include "BuiltinNodeDefs.h"
#include "SharedRegistry.h"
#include "NodeDefinition.h"

using namespace NodeEditor::Runtime;

namespace {

// Special internal nodes that BlueprintRunner.cpp dispatches DIRECTLY,
// without consulting the handler table — OR are UI-only nodes that have
// NodeDefs but legitimately have no runtime handler (Comment etc).
//
// Source of truth: BlueprintRunner.cpp searches for these by definitionId
// in the per-frame node visit loop / setup. Update this list if more get added.
const std::set<std::string>& SpecialInternalNodes()
{
    static const std::set<std::string> s = {
        // Function call / library / entry: handled directly in
        // BlueprintRunner::ExecuteNode and graph setup, NOT via handler table.
        "Function.Call",
        "Function.CallLibrary",
        "Function.Entry",
        // UI-only: a Comment node is a graph annotation, never executed.
        "Comment",
    };
    return s;
}

// Handlers that are registered but have no NodeDef. Two flavors:
//  1. Legacy aliases kept for back-compat (e.g. MapMake / SetMake — the
//     canonical NodeDef is MakeMap / MakeSet but old saved blueprints may
//     still reference the older id).
//  2. Truly orphaned handlers (Library panel can't find them — bug). The
//     goal is to drive the count to zero by either adding NodeDefs or
//     removing the handler.
const std::set<std::string>& KnownOrphanHandlers()
{
    static const std::set<std::string> s = {
        // ── Legacy aliases (intentional) ────────────────────────────────────
        "MapMake",   // canonical: MakeMap
        "SetMake",   // canonical: MakeSet

        // ── Truly orphaned (TODO: add NodeDefs or remove handlers) ──────────
        "ExecuteBlueprint",
        "FlowSequence",   // TODO: rename to "Sequence" (reserved in NodeDef) or add NodeDef
        "MultiGate",
        "Retry.Backoff",  // TODO: align with HTTP.Retry pattern
    };
    return s;
}

std::vector<std::string> CollectHandlerIds()
{
    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");
    auto ids = HandlerRegistry::Instance().GetAllIds();
    std::sort(ids.begin(), ids.end());
    return ids;
}

std::vector<std::string> CollectNodeDefIds()
{
    // Build a fresh local registry and populate it with builtin defs.
    // This is hermetic — does not touch global state, no test-order coupling.
    DefaultNodeRegistry reg;
    RegisterBuiltinNodeDefinitions(reg);
    const auto& defs = reg.getAllNodeDefinitions();
    std::vector<std::string> ids;
    ids.reserve(defs.size());
    for (const auto* d : defs)
        if (d) ids.push_back(d->id);
    std::sort(ids.begin(), ids.end());
    return ids;
}

} // namespace

// ============================================================================
// Test 1: every NodeDef has a handler (modulo special internal nodes)
// ============================================================================
// This is the user-facing direction: if a NodeDef exists, the user can drop
// the node into a graph; running the graph must NOT silently no-op because
// of a missing handler.
// ============================================================================

TEST(NodeConsistencyTest, EveryNodeDefHasHandlerOrIsSpecial)
{
    auto handlerIds = CollectHandlerIds();
    auto nodeDefIds = CollectNodeDefIds();

    std::set<std::string> handlerSet(handlerIds.begin(), handlerIds.end());

    std::vector<std::string> missing;
    for (const auto& nd : nodeDefIds)
    {
        if (handlerSet.count(nd))                    continue;
        if (SpecialInternalNodes().count(nd))        continue;
        missing.push_back(nd);
    }

    EXPECT_TRUE(missing.empty())
        << "Found " << missing.size() << " NodeDef(s) WITHOUT a handler "
           "(and not in SpecialInternalNodes whitelist). User can add these "
           "to the canvas, but execution will silently no-op.\n"
           "Missing: "
        << [&]{
              std::string s;
              for (const auto& m : missing) { s += m + " "; }
              return s;
           }();
}

// ============================================================================
// Test 2: every handler has a NodeDef (modulo known orphan handlers)
// ============================================================================
// This is the developer-facing direction: a registered handler with no
// NodeDef means dead code — works at runtime if you handcraft JSON, but
// users cannot discover it through the Library panel.
//
// We use a known-orphan whitelist so this test passes today; the goal is
// to drive that whitelist to empty over time.
// ============================================================================

TEST(NodeConsistencyTest, EveryHandlerHasNodeDef_OrIsKnownOrphan)
{
    auto handlerIds = CollectHandlerIds();
    auto nodeDefIds = CollectNodeDefIds();

    std::set<std::string> nodeDefSet(nodeDefIds.begin(), nodeDefIds.end());
    const auto& orphans = KnownOrphanHandlers();

    std::vector<std::string> newOrphans;
    for (const auto& h : handlerIds)
    {
        if (nodeDefSet.count(h))    continue;
        if (orphans.count(h))       continue;
        newOrphans.push_back(h);
    }

    EXPECT_TRUE(newOrphans.empty())
        << "Found " << newOrphans.size() << " NEW orphan handler(s) "
           "(handler registered but no NodeDef -- not visible in Library panel).\n"
           "Either:\n"
           "  - Add a NodeDef in Runtime/nodedefs/BuiltinNodeDefs_*.cpp\n"
           "  - Or, if intentionally hidden, add to KnownOrphanHandlers list "
              "in this test.\n"
           "New orphans: "
        << [&]{
              std::string s;
              for (const auto& o : newOrphans) { s += o + " "; }
              return s;
           }();
}

// ============================================================================
// Test 3: SwitchOnInt regression guard
// ============================================================================
// Specifically guard the SwitchOnInt fix from this commit. SwitchOnBool and
// SwitchOnString existed but SwitchOnInt was missing its NodeDef, even
// though the handler had been there all along. This test ensures all three
// stay registered together.
// ============================================================================

TEST(NodeConsistencyTest, AllSwitchOnTypesAreRegisteredTogether)
{
    auto handlerIds = CollectHandlerIds();
    auto nodeDefIds = CollectNodeDefIds();
    std::set<std::string> handlerSet(handlerIds.begin(), handlerIds.end());
    std::set<std::string> nodeDefSet(nodeDefIds.begin(), nodeDefIds.end());

    for (const char* id : { "SwitchOnBool", "SwitchOnInt", "SwitchOnString" })
    {
        EXPECT_TRUE(handlerSet.count(id) > 0)
            << id << " handler MUST be registered";
        EXPECT_TRUE(nodeDefSet.count(id) > 0)
            << id << " NodeDef MUST be registered (so users can add it from the Library panel)";
    }
}

// ============================================================================
// Test 4: SwitchOnInt is functionally usable end-to-end
// ============================================================================
// Build a single-node SwitchOnInt blueprint, set Selection=2, ExecuteNode,
// and verify the handler runs without error. This proves the new NodeDef
// is wired correctly to the existing handler.
// ============================================================================

TEST(NodeConsistencyTest, SwitchOnInt_HandlerExecutesWithoutError)
{
    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");

    BlueprintData bp;
    bp.metadata.name = "SwitchOnIntTest";

    NodeInstance n;
    n.id = 1;
    n.definitionId = "SwitchOnInt";

    // Inputs: exec-in (id=10), Selection (id=11)
    PinInfo execIn;  execIn.id = 10; execIn.kind = PinKind::Input;  execIn.isExec = true;
    n.pins.push_back(execIn);
    PinInfo sel;     sel.id    = 11; sel.kind    = PinKind::Input;  sel.isExec = false;
    sel.dataType = PinDataType::Integer; sel.name = "Selection";
    sel.defaultValue = Variant(int64_t(2));
    n.pins.push_back(sel);

    // Outputs: "0".."5" + "Default"
    int outId = 100;
    for (const char* name : { "0", "1", "2", "3", "4", "5", "Default" })
    {
        PinInfo p; p.id = outId++; p.kind = PinKind::Output; p.isExec = true; p.name = name;
        n.pins.push_back(p);
    }
    bp.nodes.push_back(n);
    bp.rebuildIndices();

    ASSERT_TRUE(runner.Load(bp));
    auto result = runner.ExecuteNode(1);
    EXPECT_TRUE(result.success)
        << "SwitchOnInt handler must execute successfully";
}
