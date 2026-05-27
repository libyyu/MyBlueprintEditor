// tests/test_map_set_type.cpp
//
// Coverage for Variant's PinDataType::Map and PinDataType::Set.
//
// Map: vector<pair<Variant,Variant>>  — preserves insertion order, supports
//      arbitrary key types (Int/Float/Bool/String/Object/Array/Map nested).
//      Key matching uses Variant::matchesKey() (Float uses ULP-based epsilon).
//
// Set: shares arrayValue storage with Array but is semantically distinct via
//      type=Set. setAdd() dedups using matchesKey().
//
// Key invariants exercised here:
//   1. matchesKey is used (not operator==) — Float keys tolerate small drift
//   2. mapSet on existing key OVERWRITES (does not append duplicate)
//   3. mapSet auto-promotes Unknown -> Map type
//   4. mapGet(missing) returns empty Variant (no crash, no throw)
//   5. Insertion order is preserved in iteration (mapKeys / mapValues)
//   6. setAdd duplicate is silently ignored
//   7. setAdd auto-promotes Unknown -> Set type
//   8. Mixed key types coexist (Int 1 != String "1" != Bool true)
//   9. Serialization round-trips structure AND key types
//  10. Empty Map/Set behavior

#include <gtest/gtest.h>
#include <string>
#include "Types.h"
#include "BlueprintExporter.h"
#include "BlueprintData.h"

using namespace NodeEditor::Runtime;

// ============================================================================
// Section 1: Map basics — mapSet / mapGet / mapHasKey / mapSize
// ============================================================================

TEST(MapTypeTest, MapSet_AutoPromotesUnknownToMap)
{
    Variant v;
    ASSERT_EQ(v.type, PinDataType::Unknown);

    v.mapSet(std::string("key1"), Variant(int64_t(42)));
    EXPECT_EQ(v.type, PinDataType::Map);
    EXPECT_EQ(v.mapSize(), 1u);
}

TEST(MapTypeTest, MapGet_MissingKeyReturnsEmpty)
{
    Variant m;
    m.mapSet(std::string("exists"), Variant(int64_t(1)));

    // Missing key -> empty Variant (Unknown), NOT crash, NOT throw.
    auto v = m.mapGet(std::string("missing"));
    EXPECT_EQ(v.type, PinDataType::Unknown);
}

TEST(MapTypeTest, MapHasKey_DiscriminatesPresentAndAbsent)
{
    Variant m;
    m.mapSet(std::string("a"), Variant(int64_t(1)));
    EXPECT_TRUE(m.mapHasKey(std::string("a")));
    EXPECT_FALSE(m.mapHasKey(std::string("b")));
}

TEST(MapTypeTest, MapSet_OverwritesExistingKey)
{
    // Critical invariant: setting the same key MUST overwrite the value,
    // not append a duplicate entry.
    Variant m;
    m.mapSet(std::string("k"), Variant(int64_t(1)));
    m.mapSet(std::string("k"), Variant(int64_t(2)));
    m.mapSet(std::string("k"), Variant(int64_t(3)));

    EXPECT_EQ(m.mapSize(), 1u) << "Duplicate keys must not accumulate";
    EXPECT_EQ(m.mapGet(std::string("k")).asInt(), 3);
}

TEST(MapTypeTest, MapRemove_ReturnsTrueIfRemoved_FalseIfMissing)
{
    Variant m;
    m.mapSet(std::string("k"), Variant(int64_t(1)));

    EXPECT_TRUE(m.mapRemove(std::string("k")));
    EXPECT_EQ(m.mapSize(), 0u);
    EXPECT_FALSE(m.mapRemove(std::string("k")))
        << "Second remove of same key returns false";
}

TEST(MapTypeTest, MapClear_EmptiesMap_TypeStaysMap)
{
    Variant m;
    m.mapSet(std::string("a"), Variant(int64_t(1)));
    m.mapSet(std::string("b"), Variant(int64_t(2)));
    m.mapClear();
    EXPECT_EQ(m.mapSize(), 0u);
    EXPECT_EQ(m.type, PinDataType::Map) << "Clear should not change type";
}

// ============================================================================
// Section 2: Map key type variety — arbitrary key types coexist
// ============================================================================
// Map uses matchesKey which short-circuits on type mismatch, so Int 1 and
// String "1" and Bool true are 3 DISTINCT keys.
// ============================================================================

TEST(MapTypeTest, ArbitraryKeyTypes_DistinguishedByType)
{
    Variant m;
    m.mapSet(Variant(int64_t(1)),         Variant(std::string("int_one")));
    m.mapSet(Variant(std::string("1")),   Variant(std::string("string_one")));
    m.mapSet(Variant(true),               Variant(std::string("bool_true")));

    EXPECT_EQ(m.mapSize(), 3u) << "Int 1, String '1', Bool true are 3 different keys";
    EXPECT_EQ(m.mapGet(Variant(int64_t(1))).asString(),        "int_one");
    EXPECT_EQ(m.mapGet(Variant(std::string("1"))).asString(),  "string_one");
    EXPECT_EQ(m.mapGet(Variant(true)).asString(),              "bool_true");
}

TEST(MapTypeTest, IntKeys_BasicWorks)
{
    Variant m;
    m.mapSet(Variant(int64_t(0)),   Variant(std::string("zero")));
    m.mapSet(Variant(int64_t(100)), Variant(std::string("hundred")));
    m.mapSet(Variant(int64_t(-5)),  Variant(std::string("neg5")));

    EXPECT_EQ(m.mapGet(Variant(int64_t(0))).asString(),   "zero");
    EXPECT_EQ(m.mapGet(Variant(int64_t(100))).asString(), "hundred");
    EXPECT_EQ(m.mapGet(Variant(int64_t(-5))).asString(),  "neg5");
}

// ============================================================================
// Section 3: Float key epsilon comparison via matchesKey
// ============================================================================
// matchesKey uses ULP-based epsilon (1e-9 relative, 1e-12 absolute).
// This is THE key reason matchesKey exists separate from operator==:
//   - 0.1 + 0.2 == 0.3 is FALSE in operator== (binary float drift)
//   - 0.1 + 0.2 matchesKey 0.3 is TRUE (within epsilon)
// ============================================================================

TEST(MapTypeTest, FloatKey_ExactMatch)
{
    Variant m;
    m.mapSet(Variant(1.5), Variant(std::string("one-point-five")));
    EXPECT_TRUE(m.mapHasKey(Variant(1.5)));
    EXPECT_EQ(m.mapGet(Variant(1.5)).asString(), "one-point-five");
}

TEST(MapTypeTest, FloatKey_BinaryDriftStillMatches)
{
    // 0.1 + 0.2 is 0.30000000000000004 in IEEE 754, not exactly 0.3.
    // With matchesKey's epsilon, these should be treated as the same key.
    Variant m;
    m.mapSet(Variant(0.3), Variant(std::string("found")));

    double drifted = 0.1 + 0.2;  // 0.30000000000000004
    EXPECT_NE(drifted, 0.3) << "Sanity check: floats are NOT exactly equal";

    EXPECT_TRUE(m.mapHasKey(Variant(drifted)))
        << "matchesKey must absorb IEEE 754 drift for Float keys";
    EXPECT_EQ(m.mapGet(Variant(drifted)).asString(), "found");
}

TEST(MapTypeTest, FloatKey_DistinctValuesStayDistinct)
{
    // Values far apart must NOT collapse.
    Variant m;
    m.mapSet(Variant(1.0), Variant(std::string("one")));
    m.mapSet(Variant(2.0), Variant(std::string("two")));
    EXPECT_EQ(m.mapSize(), 2u);
    EXPECT_EQ(m.mapGet(Variant(1.0)).asString(), "one");
    EXPECT_EQ(m.mapGet(Variant(2.0)).asString(), "two");
}

TEST(MapTypeTest, FloatKey_DriftOverwritesNotAppends)
{
    // Critical: mapSet with a drifted-but-matching key must OVERWRITE the
    // existing entry, not append a 2nd entry that's invisible to mapGet.
    Variant m;
    m.mapSet(Variant(0.3), Variant(std::string("first")));
    m.mapSet(Variant(0.1 + 0.2), Variant(std::string("second")));

    EXPECT_EQ(m.mapSize(), 1u)
        << "matchesKey treats 0.3 and 0.1+0.2 as same key -> overwrite, not append";
    EXPECT_EQ(m.mapGet(Variant(0.3)).asString(), "second");
}

// ============================================================================
// Section 4: mapKeys / mapValues — insertion order preserved
// ============================================================================
// vector<pair> guarantees insertion-order iteration (unlike std::unordered_map).
// This is part of Map's contract — handlers like ForEachMapLoop rely on it.
// ============================================================================

TEST(MapTypeTest, MapKeys_PreservesInsertionOrder)
{
    Variant m;
    m.mapSet(std::string("c"), Variant(int64_t(3)));
    m.mapSet(std::string("a"), Variant(int64_t(1)));
    m.mapSet(std::string("b"), Variant(int64_t(2)));

    auto keys = m.mapKeys();
    ASSERT_EQ(keys.size(), 3u);
    EXPECT_EQ(keys[0].asString(), "c");
    EXPECT_EQ(keys[1].asString(), "a");
    EXPECT_EQ(keys[2].asString(), "b");
}

TEST(MapTypeTest, MapValues_PreservesInsertionOrder)
{
    Variant m;
    m.mapSet(std::string("c"), Variant(int64_t(30)));
    m.mapSet(std::string("a"), Variant(int64_t(10)));
    m.mapSet(std::string("b"), Variant(int64_t(20)));

    auto vals = m.mapValues();
    ASSERT_EQ(vals.size(), 3u);
    EXPECT_EQ(vals[0].asInt(), 30);
    EXPECT_EQ(vals[1].asInt(), 10);
    EXPECT_EQ(vals[2].asInt(), 20);
}

TEST(MapTypeTest, MapKeysValues_OverwriteDoesNotReorder)
{
    // Updating a key's value should not move it to the end.
    Variant m;
    m.mapSet(std::string("first"),  Variant(int64_t(1)));
    m.mapSet(std::string("second"), Variant(int64_t(2)));
    m.mapSet(std::string("first"),  Variant(int64_t(99)));  // overwrite

    auto keys = m.mapKeys();
    ASSERT_EQ(keys.size(), 2u);
    EXPECT_EQ(keys[0].asString(), "first")
        << "Overwriting an existing key must keep its position";
    EXPECT_EQ(keys[1].asString(), "second");

    auto vals = m.mapValues();
    EXPECT_EQ(vals[0].asInt(), 99);
}

// ============================================================================
// Section 5: Empty Map behavior
// ============================================================================

TEST(MapTypeTest, EmptyMap_AsBoolIsFalse)
{
    Variant m;
    m.mapSet(std::string("k"), Variant(int64_t(1)));
    m.mapClear();
    EXPECT_FALSE(m.asBool());
}

TEST(MapTypeTest, EmptyMap_AsStringIsEmptyBraces)
{
    Variant m;
    m.type = PinDataType::Map;
    EXPECT_EQ(m.asString(), "{}");
}

TEST(MapTypeTest, EmptyMap_AsIntIsZero)
{
    Variant m;
    m.type = PinDataType::Map;
    EXPECT_EQ(m.asInt(), 0);
}

TEST(MapTypeTest, MapAsString_IncludesAllPairs)
{
    Variant m;
    m.mapSet(std::string("k1"), Variant(int64_t(1)));
    m.mapSet(std::string("k2"), Variant(int64_t(2)));
    std::string s = m.asString();
    EXPECT_NE(s.find("k1"), std::string::npos);
    EXPECT_NE(s.find("k2"), std::string::npos);
    EXPECT_EQ(s.front(), '{');
    EXPECT_EQ(s.back(),  '}');
}

// ============================================================================
// Section 6: Set basics — setAdd / setRemove / setContains / setSize
// ============================================================================

TEST(SetTypeTest, SetAdd_AutoPromotesUnknownToSet)
{
    Variant s;
    ASSERT_EQ(s.type, PinDataType::Unknown);
    s.setAdd(Variant(int64_t(42)));
    EXPECT_EQ(s.type, PinDataType::Set);
    EXPECT_EQ(s.setSize(), 1u);
}

TEST(SetTypeTest, SetAdd_DuplicateIsIgnoredSilently)
{
    Variant s;
    s.setAdd(Variant(int64_t(1)));
    s.setAdd(Variant(int64_t(1)));  // duplicate
    s.setAdd(Variant(int64_t(1)));  // duplicate
    s.setAdd(Variant(int64_t(2)));
    EXPECT_EQ(s.setSize(), 2u)
        << "Duplicate setAdd must be silently ignored (deduplication)";
}

TEST(SetTypeTest, SetContains_DiscriminatesPresentAndAbsent)
{
    Variant s;
    s.setAdd(Variant(std::string("hello")));
    EXPECT_TRUE(s.setContains(Variant(std::string("hello"))));
    EXPECT_FALSE(s.setContains(Variant(std::string("world"))));
}

TEST(SetTypeTest, SetRemove_ReturnsTrueIfRemoved_FalseIfMissing)
{
    Variant s;
    s.setAdd(Variant(int64_t(1)));
    EXPECT_TRUE(s.setRemove(Variant(int64_t(1))));
    EXPECT_EQ(s.setSize(), 0u);
    EXPECT_FALSE(s.setRemove(Variant(int64_t(1))))
        << "Second remove returns false";
}

TEST(SetTypeTest, SetClear_EmptiesSet)
{
    Variant s;
    s.setAdd(Variant(int64_t(1)));
    s.setAdd(Variant(int64_t(2)));
    s.setClear();
    EXPECT_EQ(s.setSize(), 0u);
}

// ============================================================================
// Section 7: Set mixed-type elements (matchesKey discriminates)
// ============================================================================

TEST(SetTypeTest, MixedTypes_DistinctEntries)
{
    Variant s;
    s.setAdd(Variant(int64_t(1)));
    s.setAdd(Variant(std::string("1")));  // string "1" != int 1
    s.setAdd(Variant(true));              // bool != int 1
    EXPECT_EQ(s.setSize(), 3u);
    EXPECT_TRUE(s.setContains(Variant(int64_t(1))));
    EXPECT_TRUE(s.setContains(Variant(std::string("1"))));
    EXPECT_TRUE(s.setContains(Variant(true)));
}

TEST(SetTypeTest, FloatEpsilon_DriftedDuplicateIgnored)
{
    Variant s;
    s.setAdd(Variant(0.3));
    s.setAdd(Variant(0.1 + 0.2));  // binary-drifted duplicate
    EXPECT_EQ(s.setSize(), 1u)
        << "Drifted-but-matching float must be deduplicated via matchesKey";
}

// ============================================================================
// Section 8: setToArray / SetFromArray semantics
// ============================================================================
// SetFromArray dedups; SetToArray preserves Set's internal order (which is
// insertion order since duplicates are skipped at setAdd time).
// ============================================================================

TEST(SetTypeTest, SetToArray_PreservesInsertionOrder)
{
    Variant s;
    s.setAdd(Variant(int64_t(3)));
    s.setAdd(Variant(int64_t(1)));
    s.setAdd(Variant(int64_t(2)));
    auto arr = s.setToArray();
    ASSERT_EQ(arr.size(), 3u);
    EXPECT_EQ(arr[0].asInt(), 3);
    EXPECT_EQ(arr[1].asInt(), 1);
    EXPECT_EQ(arr[2].asInt(), 2);
}

TEST(SetTypeTest, EmptySet_AsStringIsEmptyBraces)
{
    Variant s;
    s.type = PinDataType::Set;
    EXPECT_EQ(s.asString(), "{}");
}

TEST(SetTypeTest, SetAsString_HasBraces)
{
    Variant s;
    s.setAdd(Variant(int64_t(42)));
    std::string str = s.asString();
    EXPECT_EQ(str.front(), '{') << "Set asString uses {} brackets (not [])";
    EXPECT_EQ(str.back(),  '}');
    EXPECT_NE(str.find("42"), std::string::npos);
}

// ============================================================================
// Section 9: Set vs Array — shared storage but distinct type semantics
// ============================================================================
// Set and Array both use arrayValue, but type=Set vs type=Array changes
// what handlers / asString / operator== will do.
// ============================================================================

TEST(SetTypeTest, SetSize_EqualsArraySize_SharedStorage)
{
    Variant s;
    s.setAdd(Variant(int64_t(1)));
    s.setAdd(Variant(int64_t(2)));
    s.setAdd(Variant(int64_t(3)));
    EXPECT_EQ(s.setSize(), s.arraySize())
        << "Set and Array share underlying arrayValue";
}

TEST(SetTypeTest, ArrayPlusDuplicates_VsSetMinusDuplicates)
{
    // Array allows duplicates; Set does not. Same input -> different size.
    std::vector<Variant> elems;
    elems.push_back(Variant(int64_t(1)));
    elems.push_back(Variant(int64_t(2)));
    elems.push_back(Variant(int64_t(2)));  // dup
    elems.push_back(Variant(int64_t(3)));
    Variant arr(std::move(elems));
    EXPECT_EQ(arr.arraySize(), 4u);

    Variant s;
    for (size_t i = 0; i < arr.arraySize(); ++i) s.setAdd(arr.arrayGet(i));
    EXPECT_EQ(s.setSize(), 3u) << "Set drops the duplicate";
}

// ============================================================================
// Section 10: Serialization round-trip
// ============================================================================
// Exporter serializes Map as [[k,v],[k,v]] to preserve non-string keys.
// Set is serialized as a regular JSON array.
// ============================================================================

TEST(MapTypeTest, Serialization_StringKeyMap_RoundTrip)
{
    // Build a blueprint variable holding a Map<String,Int>, export, re-import,
    // and verify the Map is fully reconstructed.
    BlueprintData bp;
    bp.metadata.name = "MapRoundTripTest";
    bp.metadata.blueprintClass = BlueprintClass::Actor;
    bp.metadata.schemaVersion  = BLUEPRINT_CURRENT_SCHEMA_VERSION;

    VariableDefinition vd;
    vd.name = "settings";
    vd.dataType = PinDataType::Map;
    vd.containerType = ContainerType::Map;
    vd.itemType = PinDataType::Integer;
    vd.mapKeyType = PinDataType::String;
    vd.defaultValue.mapSet(std::string("hp"),  Variant(int64_t(100)));
    vd.defaultValue.mapSet(std::string("atk"), Variant(int64_t(15)));
    vd.defaultValue.mapSet(std::string("def"), Variant(int64_t(8)));
    bp.variables.push_back(vd);

    JsonBlueprintExporter exporter;
    std::string json = exporter.exportRuntimeToString(bp);
    ASSERT_FALSE(json.empty());

    // Roundtrip: import the serialized JSON back.
    auto result = exporter.importRuntimeFromString(json);
    ASSERT_TRUE(result.success) << "Re-import of exported Map must succeed";
    ASSERT_EQ(result.data.variables.size(), 1u);

    const auto& reloaded = result.data.variables[0];
    EXPECT_EQ(reloaded.name, "settings");
    EXPECT_EQ(reloaded.dataType, PinDataType::Map);
    EXPECT_EQ(reloaded.defaultValue.mapSize(), 3u);
    EXPECT_EQ(reloaded.defaultValue.mapGet(std::string("hp")).asInt(),  100);
    EXPECT_EQ(reloaded.defaultValue.mapGet(std::string("atk")).asInt(),  15);
    EXPECT_EQ(reloaded.defaultValue.mapGet(std::string("def")).asInt(),   8);
}

TEST(SetTypeTest, Serialization_SetExportContainsAllElements)
{
    // Export-only test (re-import path for Sets goes through Array reconstruct
    // which is tested elsewhere). Just verify the export produces non-empty
    // JSON containing all elements and doesn't crash.
    BlueprintData bp;
    bp.metadata.name = "SetExportTest";
    bp.metadata.blueprintClass = BlueprintClass::Actor;
    bp.metadata.schemaVersion  = BLUEPRINT_CURRENT_SCHEMA_VERSION;

    VariableDefinition vd;
    vd.name = "tags";
    vd.dataType = PinDataType::Set;
    vd.containerType = ContainerType::Single;  // Set is a container value, single binding
    vd.itemType = PinDataType::String;
    vd.defaultValue.setAdd(Variant(std::string("alpha")));
    vd.defaultValue.setAdd(Variant(std::string("beta")));
    vd.defaultValue.setAdd(Variant(std::string("gamma")));
    bp.variables.push_back(vd);

    JsonBlueprintExporter exporter;
    std::string json = exporter.exportRuntimeToString(bp);
    ASSERT_FALSE(json.empty());
    EXPECT_NE(json.find("alpha"), std::string::npos);
    EXPECT_NE(json.find("beta"),  std::string::npos);
    EXPECT_NE(json.find("gamma"), std::string::npos);
}
