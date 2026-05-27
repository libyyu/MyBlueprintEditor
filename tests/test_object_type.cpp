// tests/test_object_type.cpp
//
// Coverage for Variant's PinDataType::Object (string-id object reference).
//
// Object is the type system's MOST AMBIGUOUS case — it has overlapping
// semantics with String:
//   - Both serialize to the same JSON form ("foo")
//   - Both store data in Variant::stringValue
//   - asObjectId() has String FALLBACK (intentional duck-typing for handlers)
//   - operator== STRICT-TYPE compares (Object("foo") != String("foo"))
//
// The 5 documented usage points across the codebase reveal TWO different
// equality semantics being used in parallel:
//   - Variant::operator==   : strict-type (used by Equal node, hash logic)
//   - Object.Equal handler  : duck-typed via asObjectId (Object/String both OK)
//
// These tests pin down EVERY one of those subtleties so future refactors
// can't accidentally break either contract.

#include <gtest/gtest.h>
#include <string>
#include "Types.h"
#include "BlueprintExporter.h"
#include "BlueprintData.h"

using namespace NodeEditor::Runtime;

// ============================================================================
// Section 1: MakeObject factory
// ============================================================================

TEST(ObjectTypeTest, MakeObject_TypeIsObject)
{
    auto v = Variant::MakeObject("actor_42");
    EXPECT_EQ(v.type, PinDataType::Object);
    EXPECT_EQ(v.asObjectId(), "actor_42");
}

TEST(ObjectTypeTest, MakeObject_EmptyId_StillObjectType)
{
    // An empty id is a valid Object value (represents "null reference").
    auto v = Variant::MakeObject("");
    EXPECT_EQ(v.type, PinDataType::Object);
    EXPECT_EQ(v.asObjectId(), "");
}

// ============================================================================
// Section 2: asObjectId() — INTENTIONAL String fallback (duck-typing)
// ============================================================================
// asObjectId() returns stringValue if type == Object OR type == String.
// This lets handlers like Object.GetId / Object.SetProperty / Object.Equal
// accept either a String literal or an Object value as the "id" parameter.
// All other types return empty string.
// ============================================================================

TEST(ObjectTypeTest, AsObjectId_OnObject_ReturnsId)
{
    EXPECT_EQ(Variant::MakeObject("npc_5").asObjectId(), "npc_5");
}

TEST(ObjectTypeTest, AsObjectId_OnString_AlsoReturnsStringValue_DuckTyping)
{
    // CRITICAL CONTRACT: String("foo").asObjectId() returns "foo".
    // This is the documented duck-typing fallback so a literal-string node
    // can drive an Object handler input without an explicit converter.
    EXPECT_EQ(Variant(std::string("npc_5")).asObjectId(), "npc_5");
}

TEST(ObjectTypeTest, AsObjectId_OnOtherTypes_ReturnsEmpty)
{
    EXPECT_EQ(Variant().asObjectId(), "");                        // Unknown
    EXPECT_EQ(Variant(int64_t(42)).asObjectId(), "");             // Integer
    EXPECT_EQ(Variant(3.14).asObjectId(), "");                    // Float
    EXPECT_EQ(Variant(true).asObjectId(), "");                    // Boolean
    EXPECT_EQ(Variant::MakeAny(std::make_shared<int>(1)).asObjectId(), "");
}

// ============================================================================
// Section 3: TWO equality semantics (the most subtle Object behavior)
// ============================================================================
// Variant::operator==   : strict-type. Object("foo") != String("foo")
// Object.Equal handler  : asObjectId-based. Object("foo") == String("foo")
//
// Both are "correct" in their own context but they DISAGREE on cross-type
// comparison. This test locks down which is which.
// ============================================================================

TEST(ObjectTypeTest, Equal_OperatorEq_StrictType_ObjectVsString)
{
    auto o = Variant::MakeObject("foo");
    Variant s(std::string("foo"));
    EXPECT_FALSE(o == s)
        << "operator== is STRICT-TYPE: Object('foo') != String('foo')";
    EXPECT_FALSE(s == o);  // symmetric
}

TEST(ObjectTypeTest, Equal_OperatorEq_SameType_SameValue)
{
    auto a = Variant::MakeObject("npc_5");
    auto b = Variant::MakeObject("npc_5");
    EXPECT_TRUE(a == b);
}

TEST(ObjectTypeTest, Equal_OperatorEq_SameType_DifferentValue)
{
    auto a = Variant::MakeObject("npc_5");
    auto b = Variant::MakeObject("npc_6");
    EXPECT_FALSE(a == b);
}

TEST(ObjectTypeTest, Equal_OperatorEq_BothEmptyObject_AreEqual)
{
    auto a = Variant::MakeObject("");
    auto b = Variant::MakeObject("");
    EXPECT_TRUE(a == b) << "Two empty-id Object values must compare equal";
}

TEST(ObjectTypeTest, Equal_DuckTypedEqualityViaAsObjectId)
{
    // Simulate what Object.Equal handler does internally.
    // (We can't link the handler here, so reproduce its logic exactly.)
    auto o = Variant::MakeObject("foo");
    Variant s(std::string("foo"));

    auto aId = o.asObjectId();
    auto bId = s.asObjectId();
    EXPECT_EQ(aId, bId)
        << "Object.Equal handler treats Object('foo') and String('foo') as equal "
        << "via asObjectId duck-typing — even though operator== returns false";
}

// ============================================================================
// Section 4: asBool — empty id is falsy
// ============================================================================
// Object asBool() returns !stringValue.empty(). This matches String.
// Important: MakeObject("") is the conventional "null reference" sentinel
// and must be falsy so `if (obj) {...}` works in handler logic.
// ============================================================================

TEST(ObjectTypeTest, AsBool_NonEmptyObject_True)
{
    EXPECT_TRUE(Variant::MakeObject("actor_42").asBool());
}

TEST(ObjectTypeTest, AsBool_EmptyObject_False)
{
    EXPECT_FALSE(Variant::MakeObject("").asBool())
        << "Empty-id Object is the conventional null reference; must be falsy";
}

// ============================================================================
// Section 5: asString — ASYMMETRIC with String!
// ============================================================================
// Object: empty -> "(none)", non-empty -> the id
// String: empty -> ""        (no special "(none)" sentinel)
// This is intentional: Object asString is for DEBUG / log display where
// "(none)" tells the reader "this is a null reference", not "empty string".
// ============================================================================

TEST(ObjectTypeTest, AsString_NonEmptyObject_ReturnsId)
{
    EXPECT_EQ(Variant::MakeObject("actor_42").asString(), "actor_42");
}

TEST(ObjectTypeTest, AsString_EmptyObject_ReturnsNoneSentinel)
{
    // CRITICAL ASYMMETRY: empty Object -> "(none)", empty String -> "".
    EXPECT_EQ(Variant::MakeObject("").asString(), "(none)")
        << "Empty Object surfaces '(none)' for debug clarity; String stays ''";
    // Sanity-check the contrasting String behavior:
    EXPECT_EQ(Variant(std::string("")).asString(), "");
}

// ============================================================================
// Section 6: asInt / asFloat — Object does NOT parse its id as a number
// ============================================================================
// Unlike String, which DOES parse "42" as int 42, Object's asInt/asFloat
// fall through to the default branch and return 0. This is correct: an
// object id like "actor_42" is not a number, even if it happens to look like
// one. Tests pin this down so a future refactor doesn't "helpfully" add
// id parsing and break handlers that rely on the 0 default.
// ============================================================================

TEST(ObjectTypeTest, AsInt_OnObjectWithNumericId_ReturnsZero)
{
    // String("42") -> asInt() == 42 (sanity check)
    EXPECT_EQ(Variant(std::string("42")).asInt(), 42);
    // Object("42") -> asInt() == 0 (does NOT parse)
    EXPECT_EQ(Variant::MakeObject("42").asInt(), 0)
        << "Object MUST NOT parse its id as a number, even if the id looks numeric";
}

TEST(ObjectTypeTest, AsFloat_OnObject_ReturnsZero)
{
    EXPECT_DOUBLE_EQ(Variant(std::string("3.14")).asFloat(), 3.14);  // sanity
    EXPECT_DOUBLE_EQ(Variant::MakeObject("3.14").asFloat(), 0.0)
        << "Object MUST NOT parse its id as a number";
}

// ============================================================================
// Section 7: Default-constructed Variant interaction with Object accessors
// ============================================================================

TEST(ObjectTypeTest, DefaultVariant_NotObject_AsObjectIdEmpty)
{
    Variant v;
    EXPECT_EQ(v.type, PinDataType::Unknown);
    EXPECT_EQ(v.asObjectId(), "");
    EXPECT_FALSE(v.asBool());
}

// ============================================================================
// Section 8: Serialization round-trip via VariableDefinition.dataType
// ============================================================================
// At the raw Variant level, Object and String serialize to the SAME JSON
// (a quoted string). Type info is preserved separately on
// VariableDefinition.dataType. Round-trip a variable of type Object and
// verify the type comes back as Object, not String.
// ============================================================================

TEST(ObjectTypeTest, Serialization_ObjectVariableRoundTrip_PreservesType)
{
    BlueprintData bp;
    bp.metadata.name = "ObjectRoundTrip";
    bp.metadata.blueprintClass = BlueprintClass::Actor;
    bp.metadata.schemaVersion  = BLUEPRINT_CURRENT_SCHEMA_VERSION;

    VariableDefinition vd;
    vd.name = "playerRef";
    vd.dataType = PinDataType::Object;
    vd.containerType = ContainerType::Single;
    vd.defaultValue = Variant::MakeObject("player_01");
    bp.variables.push_back(vd);

    JsonBlueprintExporter exporter;
    std::string json = exporter.exportRuntimeToString(bp);
    ASSERT_FALSE(json.empty());
    EXPECT_NE(json.find("player_01"), std::string::npos)
        << "The object id must appear verbatim in serialized JSON";

    auto result = exporter.importRuntimeFromString(json);
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.data.variables.size(), 1u);

    const auto& reloaded = result.data.variables[0];
    EXPECT_EQ(reloaded.name, "playerRef");
    EXPECT_EQ(reloaded.dataType, PinDataType::Object)
        << "Variable type must come back as Object, not silently downgrade to String";
    EXPECT_EQ(reloaded.defaultValue.type, PinDataType::Object)
        << "Default-value Variant type must also be Object after round-trip";
    EXPECT_EQ(reloaded.defaultValue.asObjectId(), "player_01");
}

TEST(ObjectTypeTest, Serialization_EmptyObjectRef_RoundTrip)
{
    // The "null reference" case: empty id Object should also survive round-trip.
    BlueprintData bp;
    bp.metadata.name = "ObjectNullRoundTrip";
    bp.metadata.blueprintClass = BlueprintClass::Actor;
    bp.metadata.schemaVersion  = BLUEPRINT_CURRENT_SCHEMA_VERSION;

    VariableDefinition vd;
    vd.name = "noneRef";
    vd.dataType = PinDataType::Object;
    vd.containerType = ContainerType::Single;
    vd.defaultValue = Variant::MakeObject("");
    bp.variables.push_back(vd);

    JsonBlueprintExporter exporter;
    std::string json = exporter.exportRuntimeToString(bp);
    ASSERT_FALSE(json.empty());

    auto result = exporter.importRuntimeFromString(json);
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.data.variables.size(), 1u);

    const auto& reloaded = result.data.variables[0];
    EXPECT_EQ(reloaded.dataType, PinDataType::Object);
    EXPECT_EQ(reloaded.defaultValue.asObjectId(), "");
    EXPECT_FALSE(reloaded.defaultValue.asBool())
        << "Empty-id Object should remain falsy after round-trip";
}
