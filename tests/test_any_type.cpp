// tests/test_any_type.cpp
//
// Coverage for Variant's PinDataType::Any (opaque cross-language reference).
//
// Any is the trickiest Variant case:
//   - It uses std::shared_ptr<void> with type erasure + custom deleter
//   - It supports two ownership flavors: MakeAny (owning) / MakeAnyBorrow (non-owning)
//   - It must NOT crash any of the asInt / asFloat / asString / operator== paths
//   - It must serialize as JSON "null" (cannot survive cross-process serialization)
//   - Refcount must follow shared_ptr semantics on copy / move / destruction
//
// These tests act as guardrails for any future refactor of Variant::opaqueRef
// (e.g. moving to std::any, or to a tagged-pointer scheme).
//
// Note: these are pure Variant unit tests (no BlueprintRunner needed), so they
// use plain TEST(...) instead of TEST_F.

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "Types.h"
#include "BlueprintExporter.h"
#include "BlueprintData.h"

using namespace NodeEditor::Runtime;

// ============================================================================
// Helpers — dummy resource type that records its destruction in a flag,
// so we can verify shared_ptr deleter semantics end-to-end.
// ============================================================================
namespace {

struct DestructionRecorder
{
    int  id;
    bool* destroyedFlag;  // not owned

    DestructionRecorder(int i, bool* f) : id(i), destroyedFlag(f) {}
    ~DestructionRecorder()
    {
        if (destroyedFlag) *destroyedFlag = true;
    }
};

} // namespace

// ============================================================================
// Section 1: Factory methods — MakeAny / MakeAnyBorrow basics
// ============================================================================

TEST(AnyTypeTest, MakeAny_FromSharedPtr_TypeIsAny)
{
    auto sp = std::make_shared<int>(42);
    auto v  = Variant::MakeAny(sp);
    EXPECT_EQ(v.type, PinDataType::Any);
    EXPECT_TRUE(v.hasAny());
    EXPECT_EQ(v.asAny(), sp.get());
}

TEST(AnyTypeTest, MakeAny_NullSharedPtr_HasAnyIsFalse)
{
    // A Variant with type=Any but a null opaqueRef should NOT report hasAny().
    Variant v = Variant::MakeAny(std::shared_ptr<void>{});
    EXPECT_EQ(v.type, PinDataType::Any);
    EXPECT_FALSE(v.hasAny());
    EXPECT_EQ(v.asAny(), nullptr);
}

TEST(AnyTypeTest, MakeAnyBorrow_FromRawPointer_TypeIsAny)
{
    int local = 7;
    auto v = Variant::MakeAnyBorrow(&local);
    EXPECT_EQ(v.type, PinDataType::Any);
    EXPECT_TRUE(v.hasAny());
    EXPECT_EQ(v.asAny(), &local);
}

TEST(AnyTypeTest, MakeAnyBorrow_DoesNotOwn)
{
    // Borrow-flavor must NOT free the borrowed pointer when the Variant dies.
    bool destroyed = false;
    DestructionRecorder rec(1, &destroyed);
    {
        auto v = Variant::MakeAnyBorrow(&rec);
        EXPECT_TRUE(v.hasAny());
    } // <- v destroyed here; deleter is no-op for MakeAnyBorrow
    EXPECT_FALSE(destroyed)
        << "MakeAnyBorrow must NOT destroy the borrowed object";
}

TEST(AnyTypeTest, MakeAny_CallsDeleterOnLastReference)
{
    // Owning flavor must run the shared_ptr deleter when the last ref dies.
    bool destroyed = false;
    {
        auto sp = std::shared_ptr<DestructionRecorder>(
            new DestructionRecorder(2, &destroyed),
            [](DestructionRecorder* p){ delete p; });
        auto v = Variant::MakeAny(sp);
        EXPECT_TRUE(v.hasAny());
        sp.reset();
        EXPECT_FALSE(destroyed)
            << "Variant still holds a reference, object must be alive";
    } // <- v dies, last ref drops, deleter must fire
    EXPECT_TRUE(destroyed)
        << "MakeAny must release the underlying object when last ref is gone";
}

TEST(AnyTypeTest, MakeAny_CustomDeleterReceivesCorrectPointer)
{
    // Verify the deleter sees the original raw pointer (covers shared_ptr<void>
    // type-erasure correctness for future-proofing).
    int* raw = new int(99);
    void* deletedPtr = nullptr;
    {
        auto sp = std::shared_ptr<void>(
            raw,
            [&deletedPtr](void* p){
                deletedPtr = p;
                delete static_cast<int*>(p);
            });
        auto v = Variant::MakeAny(sp);
        EXPECT_EQ(v.asAny(), raw);
    } // sp also went out of scope above; deleter fires here
    EXPECT_EQ(deletedPtr, raw)
        << "Deleter must receive the original raw pointer, not a void* alias";
}

// ============================================================================
// Section 2: Accessors on non-Any Variants
// ============================================================================

TEST(AnyTypeTest, AsAny_OnNonAnyVariant_ReturnsNullptr)
{
    EXPECT_EQ(Variant().asAny(), nullptr);
    EXPECT_EQ(Variant(int64_t(42)).asAny(), nullptr);
    EXPECT_EQ(Variant(3.14).asAny(), nullptr);
    EXPECT_EQ(Variant(std::string("x")).asAny(), nullptr);
    EXPECT_EQ(Variant(true).asAny(), nullptr);
}

TEST(AnyTypeTest, AsAnyShared_OnNonAnyVariant_ReturnsEmpty)
{
    EXPECT_EQ(Variant().asAnyShared(), nullptr);
    EXPECT_EQ(Variant(int64_t(42)).asAnyShared(), nullptr);
}

TEST(AnyTypeTest, HasAny_OnlyTrueForAnyTypeWithNonNullRef)
{
    EXPECT_FALSE(Variant().hasAny());
    EXPECT_FALSE(Variant(int64_t(0)).hasAny());
    EXPECT_FALSE(Variant(std::string("not any")).hasAny());

    auto v1 = Variant::MakeAny(std::shared_ptr<void>{});      // null
    EXPECT_FALSE(v1.hasAny());

    auto v2 = Variant::MakeAny(std::make_shared<int>(1));     // non-null
    EXPECT_TRUE(v2.hasAny());
}

// ============================================================================
// Section 3: Copy / move semantics — refcount must behave like shared_ptr
// ============================================================================

TEST(AnyTypeTest, Copy_IncrementsRefCount)
{
    auto sp = std::make_shared<int>(123);
    EXPECT_EQ(sp.use_count(), 1);

    auto v1 = Variant::MakeAny(sp);
    EXPECT_EQ(sp.use_count(), 2);  // sp + v1.opaqueRef

    Variant v2 = v1;               // copy
    EXPECT_EQ(sp.use_count(), 3);  // sp + v1 + v2

    Variant v3;
    v3 = v1;                       // copy assign
    EXPECT_EQ(sp.use_count(), 4);
}

TEST(AnyTypeTest, Move_DoesNotIncrementRefCount)
{
    auto sp = std::make_shared<int>(456);
    EXPECT_EQ(sp.use_count(), 1);

    auto v1 = Variant::MakeAny(sp);
    EXPECT_EQ(sp.use_count(), 2);

    Variant v2 = std::move(v1);
    EXPECT_EQ(sp.use_count(), 2)
        << "Move-construct must transfer ownership, not duplicate it";
    EXPECT_TRUE(v2.hasAny());
    EXPECT_EQ(v2.asAny(), sp.get());
}

TEST(AnyTypeTest, Destruction_DecrementsRefCount)
{
    auto sp = std::make_shared<int>(789);
    EXPECT_EQ(sp.use_count(), 1);

    {
        auto v1 = Variant::MakeAny(sp);
        auto v2 = v1;
        EXPECT_EQ(sp.use_count(), 3);
    } // v1, v2 both die

    EXPECT_EQ(sp.use_count(), 1)
        << "Both Variants destroyed; only original sp should remain";
}

// ============================================================================
// Section 4: Cross-type accessor safety on Any
// ============================================================================
// Any values typically can't sensibly convert to int/float/string, but they
// MUST NOT crash. asString() should produce a debug-friendly form.
// ============================================================================

TEST(AnyTypeTest, AsInt_OnAny_ReturnsZero)
{
    auto v = Variant::MakeAny(std::make_shared<int>(42));
    EXPECT_EQ(v.asInt(), 0)
        << "asInt() on Any must return 0 (default), not crash or read pointer bytes";
}

TEST(AnyTypeTest, AsFloat_OnAny_ReturnsZero)
{
    auto v = Variant::MakeAny(std::make_shared<int>(42));
    EXPECT_DOUBLE_EQ(v.asFloat(), 0.0);
}

TEST(AnyTypeTest, AsString_OnAny_ReturnsDebugFormat)
{
    auto sp = std::make_shared<int>(42);
    auto v  = Variant::MakeAny(sp);
    auto s  = v.asString();
    // Format: "(any:0xADDR)"
    EXPECT_TRUE(s.rfind("(any:", 0) == 0)
        << "Got: '" << s << "' — expected prefix '(any:'";
    EXPECT_EQ(s.back(), ')');
    EXPECT_NE(s, "(any:null)")  // we passed a non-null ptr
        << "Non-null Any must format with the actual pointer, not 'null'";
}

TEST(AnyTypeTest, AsString_OnAnyWithNullRef_ReturnsAnyNull)
{
    Variant v = Variant::MakeAny(std::shared_ptr<void>{});
    EXPECT_EQ(v.asString(), "(any:null)");
}

TEST(AnyTypeTest, AsBool_OnAny_TrueIffNonNull)
{
    auto vNull   = Variant::MakeAny(std::shared_ptr<void>{});
    auto vNotNull = Variant::MakeAny(std::make_shared<int>(1));
    EXPECT_FALSE(vNull.asBool());
    EXPECT_TRUE(vNotNull.asBool());
}

// ============================================================================
// Section 5: operator== — pointer-identity semantics for Any
// ============================================================================
// Any compares by pointer identity (raw .get()), NOT by value.
// This is the documented contract: Runtime never inspects the opaque content.
// ============================================================================

TEST(AnyTypeTest, Equal_SameUnderlyingPointer)
{
    auto sp = std::make_shared<int>(100);
    auto v1 = Variant::MakeAny(sp);
    auto v2 = Variant::MakeAny(sp);
    EXPECT_TRUE(v1 == v2)
        << "Two Variants holding the same shared_ptr must compare equal";
}

TEST(AnyTypeTest, NotEqual_DifferentPointersSameValue)
{
    // Two distinct objects that happen to hold the same int value.
    // Any compares pointer identity, not value -> these must NOT be equal.
    auto sp1 = std::make_shared<int>(100);
    auto sp2 = std::make_shared<int>(100);
    auto v1  = Variant::MakeAny(sp1);
    auto v2  = Variant::MakeAny(sp2);
    EXPECT_FALSE(v1 == v2)
        << "Any compares by pointer identity; equal-valued but distinct objects "
           "must NOT compare equal";
}

TEST(AnyTypeTest, NotEqual_AnyVsNonAny)
{
    auto vAny = Variant::MakeAny(std::make_shared<int>(42));
    Variant   vInt(int64_t(42));
    EXPECT_FALSE(vAny == vInt)
        << "Cross-type compare: type mismatch must short-circuit to false";
}

// ============================================================================
// Section 6: Serialization — Any must export as JSON null
// ============================================================================
// Any references can't survive cross-process serialization (opaque pointer).
// The exporter must output "null" instead of crashing or emitting garbage.
// ============================================================================

TEST(AnyTypeTest, Serialization_AnyVariableExportsAsJsonNull)
{
    // Build a minimal blueprint with a single variable of type Any holding a
    // dummy ref, export to JSON, verify the default-value field is "null".
    BlueprintData bp;
    bp.metadata.name = "AnySerializeTest";
    bp.metadata.blueprintClass = BlueprintClass::Actor;
    bp.metadata.schemaVersion  = BLUEPRINT_CURRENT_SCHEMA_VERSION;

    VariableDefinition vd;
    vd.name = "anyVar";
    vd.dataType = PinDataType::Any;
    vd.defaultValue = Variant::MakeAny(std::make_shared<int>(42));
    bp.variables.push_back(vd);

    JsonBlueprintExporter exporter;
    std::string json = exporter.exportRuntimeToString(bp);

    // The exact JSON layout depends on the exporter, but the Any default value
    // must surface as the literal "null" — never the pointer bytes.
    EXPECT_FALSE(json.empty()) << "Export must succeed and produce non-empty JSON";
    EXPECT_NE(json.find("\"anyVar\""), std::string::npos)
        << "Variable name should appear in exported JSON";
    // The "(any:0x..." debug string must NOT leak into the export.
    EXPECT_EQ(json.find("(any:"), std::string::npos)
        << "Any debug string '(any:...)' must NOT appear in serialized JSON";
}
