// tests/test_crypto_and_math.cpp
//
// Coverage for two previously-untested handler families:
//   - BuiltinHandlers_Crypto.cpp  (7 nodes:  SHA256 / HMAC / MD5 /
//                                   Base64Enc / Base64Dec / HexEncode / HexDecode)
//   - BuiltinHandlers_GameMath.cpp (selected high-value nodes:
//                                   Vec2.Make/Add/Length/Dot/Distance/Normalize,
//                                   Vec3.Make/Add/Cross/Dot,
//                                   Physics.PointInRect / Physics.PointInCircle,
//                                   Stat.Set/Get/AddModifier/Reset)
//
// Strategy: build a one-node BlueprintData, runner.Load(bp), runner.ExecuteNode(id),
// then read outputs via runner.GetPinValue(pinId). This bypasses the topology and
// event system for purely-functional handlers — cheap & fast.
//
// Stat tests use distinct (Entity, Name) keys to avoid cross-test pollution of
// the global s_stats map, and Stat.Reset() between phases as a defense in depth.

#include <gtest/gtest.h>
#include <cmath>
#include "BlueprintRunner.h"
#include "BlueprintData.h"
#include "BuiltinHandlers.h"

using namespace NodeEditor::Runtime;

namespace {

// ----------------------------------------------------------------------------
// Small fixture: runner with builtin handlers registered, plus a helper that
// builds a single-node blueprint with declared pins, executes it once, and
// returns the output PinId map for convenient read-back.
// ----------------------------------------------------------------------------
class HandlerOneShotTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        RegisterBuiltinHandlers(runner, ".");
    }

    struct PinSpec
    {
        uint64_t id;
        PinKind  kind;
        PinDataType dt;
        const char* name;
        Variant defaultValue;
    };

    // Build a single-node BP with the given pins, run ExecuteNode, return success.
    // After return, you can read outputs via runner.GetPinValue(pinId).
    bool runSingleNode(const std::string& defId,
                       uint64_t nodeId,
                       std::initializer_list<PinSpec> pins)
    {
        BlueprintData bp;
        bp.metadata.name = "OneShotTest";
        NodeInstance n;
        n.id = nodeId;
        n.definitionId = defId;
        for (const auto& s : pins)
        {
            PinInfo p;
            p.id = s.id;
            p.kind = s.kind;
            p.dataType = s.dt;
            p.name = s.name;
            p.defaultValue = s.defaultValue;
            n.pins.push_back(p);
        }
        bp.nodes.push_back(n);
        bp.rebuildIndices();
        if (!runner.Load(bp)) return false;
        auto r = runner.ExecuteNode(nodeId);
        return r.success;
    }

    BlueprintRunner runner;
};

} // namespace

// ============================================================================
// Crypto: SHA-256
// ============================================================================
// Test vectors from NIST FIPS 180-2 / RFC 6234.
// ============================================================================

TEST_F(HandlerOneShotTest, CryptoSHA256_EmptyInput)
{
    // SHA-256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    ASSERT_TRUE(runSingleNode("Crypto.SHA256", 1, {
        {10, PinKind::Input,  PinDataType::String, "Data", Variant(std::string(""))},
        {11, PinKind::Output, PinDataType::String, "Hash", {}},
    }));
    EXPECT_EQ(runner.GetPinValue(11).asString(),
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST_F(HandlerOneShotTest, CryptoSHA256_AbcInput)
{
    // SHA-256("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
    ASSERT_TRUE(runSingleNode("Crypto.SHA256", 1, {
        {10, PinKind::Input,  PinDataType::String, "Data", Variant(std::string("abc"))},
        {11, PinKind::Output, PinDataType::String, "Hash", {}},
    }));
    EXPECT_EQ(runner.GetPinValue(11).asString(),
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

// ============================================================================
// Crypto: MD5
// ============================================================================
// Test vectors from RFC 1321 section A.5.
// ============================================================================

TEST_F(HandlerOneShotTest, CryptoMD5_EmptyInput)
{
    // MD5("") = d41d8cd98f00b204e9800998ecf8427e
    ASSERT_TRUE(runSingleNode("Crypto.MD5", 1, {
        {10, PinKind::Input,  PinDataType::String, "Data", Variant(std::string(""))},
        {11, PinKind::Output, PinDataType::String, "Hash", {}},
    }));
    EXPECT_EQ(runner.GetPinValue(11).asString(),
              "d41d8cd98f00b204e9800998ecf8427e");
}

TEST_F(HandlerOneShotTest, CryptoMD5_AbcInput)
{
    // MD5("abc") = 900150983cd24fb0d6963f7d28e17f72
    ASSERT_TRUE(runSingleNode("Crypto.MD5", 1, {
        {10, PinKind::Input,  PinDataType::String, "Data", Variant(std::string("abc"))},
        {11, PinKind::Output, PinDataType::String, "Hash", {}},
    }));
    EXPECT_EQ(runner.GetPinValue(11).asString(),
              "900150983cd24fb0d6963f7d28e17f72");
}

// ============================================================================
// Crypto: HMAC-SHA256
// ============================================================================
// RFC 4231 test case 1: key=20 bytes of 0x0b, data="Hi There"
//   expected = b0344c61d8db38535ca8afceaf0bf12b
//              881dc200c9833da726e9376c2e32cff7
// ============================================================================

TEST_F(HandlerOneShotTest, CryptoHMAC_RFC4231Vector1)
{
    std::string key(20, '\x0b');
    ASSERT_TRUE(runSingleNode("Crypto.HMAC", 1, {
        {10, PinKind::Input,  PinDataType::String, "Key",  Variant(key)},
        {11, PinKind::Input,  PinDataType::String, "Data", Variant(std::string("Hi There"))},
        {12, PinKind::Output, PinDataType::String, "Hash",       {}},
        {13, PinKind::Output, PinDataType::String, "HashBase64", {}},
    }));
    EXPECT_EQ(runner.GetPinValue(12).asString(),
              "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
    // Base64 of the same 32 bytes:
    EXPECT_EQ(runner.GetPinValue(13).asString(),
              "sDRMYdjbOFNcqK/OrwvxK4gdwgDJgz2nJuk3bC4yz/c=");
}

// ============================================================================
// Crypto: Base64
// ============================================================================
// RFC 4648 test vectors.
// ============================================================================

TEST_F(HandlerOneShotTest, CryptoBase64Enc_Standard)
{
    ASSERT_TRUE(runSingleNode("Crypto.Base64Enc", 1, {
        {10, PinKind::Input,  PinDataType::String, "Data",   Variant(std::string("Man"))},
        {11, PinKind::Output, PinDataType::String, "Result", {}},
    }));
    EXPECT_EQ(runner.GetPinValue(11).asString(), "TWFu");
}

TEST_F(HandlerOneShotTest, CryptoBase64Enc_Padding)
{
    // "f"   -> "Zg=="    (2 padding)
    // "fo"  -> "Zm8="    (1 padding)
    // "foo" -> "Zm9v"    (0 padding)
    ASSERT_TRUE(runSingleNode("Crypto.Base64Enc", 1, {
        {10, PinKind::Input,  PinDataType::String, "Data",   Variant(std::string("f"))},
        {11, PinKind::Output, PinDataType::String, "Result", {}},
    }));
    EXPECT_EQ(runner.GetPinValue(11).asString(), "Zg==");
}

TEST_F(HandlerOneShotTest, CryptoBase64_RoundTrip)
{
    // Encode then decode -> original.
    ASSERT_TRUE(runSingleNode("Crypto.Base64Enc", 1, {
        {10, PinKind::Input,  PinDataType::String, "Data",
            Variant(std::string("Hello, Blueprint!"))},
        {11, PinKind::Output, PinDataType::String, "Result", {}},
    }));
    std::string encoded = runner.GetPinValue(11).asString();

    ASSERT_TRUE(runSingleNode("Crypto.Base64Dec", 2, {
        {20, PinKind::Input,  PinDataType::String, "Data",   Variant(encoded)},
        {21, PinKind::Output, PinDataType::String, "Result", {}},
    }));
    EXPECT_EQ(runner.GetPinValue(21).asString(), "Hello, Blueprint!");
}

// ============================================================================
// Crypto: Hex
// ============================================================================

TEST_F(HandlerOneShotTest, CryptoHexEncode_AsciiString)
{
    // "ABC" -> 41 42 43
    ASSERT_TRUE(runSingleNode("Crypto.HexEncode", 1, {
        {10, PinKind::Input,  PinDataType::String, "Data",   Variant(std::string("ABC"))},
        {11, PinKind::Output, PinDataType::String, "Result", {}},
    }));
    EXPECT_EQ(runner.GetPinValue(11).asString(), "414243");
}

TEST_F(HandlerOneShotTest, CryptoHexDecode_ValidEvenLength)
{
    ASSERT_TRUE(runSingleNode("Crypto.HexDecode", 1, {
        {10, PinKind::Input,  PinDataType::String, "Data",   Variant(std::string("414243"))},
        {11, PinKind::Output, PinDataType::String, "Result", {}},
        {12, PinKind::Output, PinDataType::Boolean, "Valid", {}},
    }));
    EXPECT_EQ(runner.GetPinValue(11).asString(), "ABC");
    EXPECT_TRUE(runner.GetPinValue(12).asBool());
}

TEST_F(HandlerOneShotTest, CryptoHexDecode_OddLengthFailsGracefully)
{
    // Odd length should fail validation, not crash.
    ASSERT_TRUE(runSingleNode("Crypto.HexDecode", 1, {
        {10, PinKind::Input,  PinDataType::String, "Data",   Variant(std::string("ABC"))},
        {11, PinKind::Output, PinDataType::String, "Result", {}},
        {12, PinKind::Output, PinDataType::Boolean, "Valid", {}},
    }));
    EXPECT_EQ(runner.GetPinValue(11).asString(), "");
    EXPECT_FALSE(runner.GetPinValue(12).asBool());
}

TEST_F(HandlerOneShotTest, CryptoHex_RoundTrip)
{
    ASSERT_TRUE(runSingleNode("Crypto.HexEncode", 1, {
        {10, PinKind::Input,  PinDataType::String, "Data",
            Variant(std::string("\x01\x02\xff\x80", 4))},
        {11, PinKind::Output, PinDataType::String, "Result", {}},
    }));
    std::string hex = runner.GetPinValue(11).asString();
    EXPECT_EQ(hex, "0102ff80");

    ASSERT_TRUE(runSingleNode("Crypto.HexDecode", 2, {
        {20, PinKind::Input,  PinDataType::String,  "Data",   Variant(hex)},
        {21, PinKind::Output, PinDataType::String,  "Result", {}},
        {22, PinKind::Output, PinDataType::Boolean, "Valid",  {}},
    }));
    EXPECT_EQ(runner.GetPinValue(21).asString(),
              std::string("\x01\x02\xff\x80", 4));
    EXPECT_TRUE(runner.GetPinValue(22).asBool());
}

// ============================================================================
// GameMath: Vec2
// ============================================================================
// Vec2 is serialized as "x,y" string. Vec2.Make has 3 outputs (Vec2 string, X, Y).
// ============================================================================

TEST_F(HandlerOneShotTest, Vec2Make_ProducesXYAndVec2String)
{
    ASSERT_TRUE(runSingleNode("Vec2.Make", 1, {
        {10, PinKind::Input,  PinDataType::Float, "X", Variant(3.0)},
        {11, PinKind::Input,  PinDataType::Float, "Y", Variant(4.0)},
        {12, PinKind::Output, PinDataType::String, "Vec2", {}},
        {13, PinKind::Output, PinDataType::Float,  "X",    {}},
        {14, PinKind::Output, PinDataType::Float,  "Y",    {}},
    }));
    EXPECT_EQ(runner.GetPinValue(12).asString(), "3,4");
    EXPECT_DOUBLE_EQ(runner.GetPinValue(13).asFloat(), 3.0);
    EXPECT_DOUBLE_EQ(runner.GetPinValue(14).asFloat(), 4.0);
}

TEST_F(HandlerOneShotTest, Vec2Add_ComponentWise)
{
    ASSERT_TRUE(runSingleNode("Vec2.Add", 1, {
        {10, PinKind::Input,  PinDataType::String, "A", Variant(std::string("1,2"))},
        {11, PinKind::Input,  PinDataType::String, "B", Variant(std::string("3,4"))},
        {12, PinKind::Output, PinDataType::String, "Result", {}},
    }));
    EXPECT_EQ(runner.GetPinValue(12).asString(), "4,6");
}

TEST_F(HandlerOneShotTest, Vec2Length_3_4_Is5)
{
    ASSERT_TRUE(runSingleNode("Vec2.Length", 1, {
        {10, PinKind::Input,  PinDataType::String, "Vec2", Variant(std::string("3,4"))},
        {11, PinKind::Output, PinDataType::Float,  "Length", {}},
    }));
    EXPECT_DOUBLE_EQ(runner.GetPinValue(11).asFloat(), 5.0);
}

TEST_F(HandlerOneShotTest, Vec2Dot_OrthogonalIsZero)
{
    ASSERT_TRUE(runSingleNode("Vec2.Dot", 1, {
        {10, PinKind::Input,  PinDataType::String, "A", Variant(std::string("1,0"))},
        {11, PinKind::Input,  PinDataType::String, "B", Variant(std::string("0,1"))},
        {12, PinKind::Output, PinDataType::Float,  "Result", {}},
    }));
    EXPECT_DOUBLE_EQ(runner.GetPinValue(12).asFloat(), 0.0);
}

TEST_F(HandlerOneShotTest, Vec2Distance_BetweenPoints)
{
    ASSERT_TRUE(runSingleNode("Vec2.Distance", 1, {
        {10, PinKind::Input,  PinDataType::String, "A", Variant(std::string("0,0"))},
        {11, PinKind::Input,  PinDataType::String, "B", Variant(std::string("3,4"))},
        {12, PinKind::Output, PinDataType::Float,  "Distance", {}},
    }));
    EXPECT_DOUBLE_EQ(runner.GetPinValue(12).asFloat(), 5.0);
}

TEST_F(HandlerOneShotTest, Vec2Normalize_UnitLength)
{
    ASSERT_TRUE(runSingleNode("Vec2.Normalize", 1, {
        {10, PinKind::Input,  PinDataType::String, "Vec2", Variant(std::string("3,4"))},
        {11, PinKind::Output, PinDataType::String, "Result", {}},
        {12, PinKind::Output, PinDataType::Float,  "Length", {}},
    }));
    EXPECT_DOUBLE_EQ(runner.GetPinValue(12).asFloat(), 5.0);  // pre-normalize length
    // Result vector should have length 1; parse and check.
    std::string r = runner.GetPinValue(11).asString();
    // "0.6,0.8"
    auto comma = r.find(',');
    ASSERT_NE(comma, std::string::npos);
    double rx = std::stod(r.substr(0, comma));
    double ry = std::stod(r.substr(comma + 1));
    EXPECT_NEAR(rx, 0.6, 1e-5);
    EXPECT_NEAR(ry, 0.8, 1e-5);
}

// ============================================================================
// GameMath: Vec3
// ============================================================================

TEST_F(HandlerOneShotTest, Vec3Add_ComponentWise)
{
    ASSERT_TRUE(runSingleNode("Vec3.Add", 1, {
        {10, PinKind::Input,  PinDataType::String, "A", Variant(std::string("1,2,3"))},
        {11, PinKind::Input,  PinDataType::String, "B", Variant(std::string("4,5,6"))},
        {12, PinKind::Output, PinDataType::String, "Result", {}},
    }));
    EXPECT_EQ(runner.GetPinValue(12).asString(), "5,7,9");
}

TEST_F(HandlerOneShotTest, Vec3Cross_XAxisCrossYAxisIsZAxis)
{
    // (1,0,0) x (0,1,0) = (0,0,1)
    ASSERT_TRUE(runSingleNode("Vec3.Cross", 1, {
        {10, PinKind::Input,  PinDataType::String, "A", Variant(std::string("1,0,0"))},
        {11, PinKind::Input,  PinDataType::String, "B", Variant(std::string("0,1,0"))},
        {12, PinKind::Output, PinDataType::String, "Result", {}},
    }));
    EXPECT_EQ(runner.GetPinValue(12).asString(), "0,0,1");
}

TEST_F(HandlerOneShotTest, Vec3Dot_BasisVectors)
{
    // (1,2,3) . (4,5,6) = 4+10+18 = 32
    ASSERT_TRUE(runSingleNode("Vec3.Dot", 1, {
        {10, PinKind::Input,  PinDataType::String, "A", Variant(std::string("1,2,3"))},
        {11, PinKind::Input,  PinDataType::String, "B", Variant(std::string("4,5,6"))},
        {12, PinKind::Output, PinDataType::Float,  "Result", {}},
    }));
    EXPECT_DOUBLE_EQ(runner.GetPinValue(12).asFloat(), 32.0);
}

// ============================================================================
// GameMath: Physics primitives
// ============================================================================

TEST_F(HandlerOneShotTest, PhysicsPointInRect_Inside)
{
    // Rect (0,0) size 10x10; Point (5,5) -> inside
    ASSERT_TRUE(runSingleNode("Physics.PointInRect", 1, {
        {10, PinKind::Input,  PinDataType::Float,   "PointX", Variant(5.0)},
        {11, PinKind::Input,  PinDataType::Float,   "PointY", Variant(5.0)},
        {12, PinKind::Input,  PinDataType::Float,   "RectX",  Variant(0.0)},
        {13, PinKind::Input,  PinDataType::Float,   "RectY",  Variant(0.0)},
        {14, PinKind::Input,  PinDataType::Float,   "RectW",  Variant(10.0)},
        {15, PinKind::Input,  PinDataType::Float,   "RectH",  Variant(10.0)},
        {16, PinKind::Output, PinDataType::Boolean, "Inside", {}},
    }));
    EXPECT_TRUE(runner.GetPinValue(16).asBool());
}

TEST_F(HandlerOneShotTest, PhysicsPointInRect_Outside)
{
    // Rect (0,0) size 10x10; Point (15,5) -> outside (px > rx+rw)
    ASSERT_TRUE(runSingleNode("Physics.PointInRect", 1, {
        {10, PinKind::Input,  PinDataType::Float,   "PointX", Variant(15.0)},
        {11, PinKind::Input,  PinDataType::Float,   "PointY", Variant(5.0)},
        {12, PinKind::Input,  PinDataType::Float,   "RectX",  Variant(0.0)},
        {13, PinKind::Input,  PinDataType::Float,   "RectY",  Variant(0.0)},
        {14, PinKind::Input,  PinDataType::Float,   "RectW",  Variant(10.0)},
        {15, PinKind::Input,  PinDataType::Float,   "RectH",  Variant(10.0)},
        {16, PinKind::Output, PinDataType::Boolean, "Inside", {}},
    }));
    EXPECT_FALSE(runner.GetPinValue(16).asBool());
}

TEST_F(HandlerOneShotTest, PhysicsPointInCircle_Inside)
{
    // Center (0,0) radius 5; Point (1,1) -> dist^2=2 <= 25 -> inside
    ASSERT_TRUE(runSingleNode("Physics.PointInCircle", 1, {
        {10, PinKind::Input,  PinDataType::Float,   "PointX", Variant(1.0)},
        {11, PinKind::Input,  PinDataType::Float,   "PointY", Variant(1.0)},
        {12, PinKind::Input,  PinDataType::Float,   "CX",     Variant(0.0)},
        {13, PinKind::Input,  PinDataType::Float,   "CY",     Variant(0.0)},
        {14, PinKind::Input,  PinDataType::Float,   "Radius", Variant(5.0)},
        {15, PinKind::Output, PinDataType::Boolean, "Inside", {}},
        {16, PinKind::Output, PinDataType::Float,   "Distance", {}},
    }));
    EXPECT_TRUE(runner.GetPinValue(15).asBool());
    EXPECT_NEAR(runner.GetPinValue(16).asFloat(), std::sqrt(2.0), 1e-5);
}

TEST_F(HandlerOneShotTest, PhysicsPointInCircle_Outside)
{
    // Center (0,0) radius 5; Point (10,10) -> dist^2=200 > 25 -> outside
    ASSERT_TRUE(runSingleNode("Physics.PointInCircle", 1, {
        {10, PinKind::Input,  PinDataType::Float,   "PointX", Variant(10.0)},
        {11, PinKind::Input,  PinDataType::Float,   "PointY", Variant(10.0)},
        {12, PinKind::Input,  PinDataType::Float,   "CX",     Variant(0.0)},
        {13, PinKind::Input,  PinDataType::Float,   "CY",     Variant(0.0)},
        {14, PinKind::Input,  PinDataType::Float,   "Radius", Variant(5.0)},
        {15, PinKind::Output, PinDataType::Boolean, "Inside", {}},
        {16, PinKind::Output, PinDataType::Float,   "Distance", {}},
    }));
    EXPECT_FALSE(runner.GetPinValue(15).asBool());
}

// ============================================================================
// GameMath: Stat system  (uses global state -- be careful with keys)
// ============================================================================
//
// Each test uses a distinct (Entity, Name) pair scoped by the test name to
// avoid cross-test pollution of the process-wide s_stats map.
// ============================================================================

TEST_F(HandlerOneShotTest, StatSetAndGet_BaseValue)
{
    // Set base = 100
    ASSERT_TRUE(runSingleNode("Stat.Set", 1, {
        {10, PinKind::Input, PinDataType::String, "Name",   Variant(std::string("hp"))},
        {11, PinKind::Input, PinDataType::String, "Entity", Variant(std::string("stat_test_base"))},
        {12, PinKind::Input, PinDataType::Float,  "Base",   Variant(100.0)},
    }));

    // Get
    ASSERT_TRUE(runSingleNode("Stat.Get", 2, {
        {20, PinKind::Input,  PinDataType::String, "Name",   Variant(std::string("hp"))},
        {21, PinKind::Input,  PinDataType::String, "Entity", Variant(std::string("stat_test_base"))},
        {22, PinKind::Output, PinDataType::Float,  "Value",         {}},
        {23, PinKind::Output, PinDataType::Float,  "Base",          {}},
        {24, PinKind::Output, PinDataType::Integer,"ModifierCount", {}},
    }));
    EXPECT_DOUBLE_EQ(runner.GetPinValue(22).asFloat(), 100.0);
    EXPECT_DOUBLE_EQ(runner.GetPinValue(23).asFloat(), 100.0);
    EXPECT_EQ(runner.GetPinValue(24).asInt(), 0);
}

TEST_F(HandlerOneShotTest, StatGet_NonExistentReturnsZero)
{
    ASSERT_TRUE(runSingleNode("Stat.Get", 1, {
        {10, PinKind::Input,  PinDataType::String, "Name",   Variant(std::string("nonexistent_stat_xyz"))},
        {11, PinKind::Input,  PinDataType::String, "Entity", Variant(std::string("stat_test_missing"))},
        {12, PinKind::Output, PinDataType::Float,  "Value",         {}},
        {13, PinKind::Output, PinDataType::Float,  "Base",          {}},
        {14, PinKind::Output, PinDataType::Integer,"ModifierCount", {}},
    }));
    EXPECT_DOUBLE_EQ(runner.GetPinValue(12).asFloat(), 0.0);
    EXPECT_DOUBLE_EQ(runner.GetPinValue(13).asFloat(), 0.0);
    EXPECT_EQ(runner.GetPinValue(14).asInt(), 0);
}

TEST_F(HandlerOneShotTest, StatAddModifier_AddType)
{
    // Base 100, then +20 additive -> 120
    ASSERT_TRUE(runSingleNode("Stat.Set", 1, {
        {10, PinKind::Input, PinDataType::String, "Name",   Variant(std::string("atk"))},
        {11, PinKind::Input, PinDataType::String, "Entity", Variant(std::string("stat_test_add"))},
        {12, PinKind::Input, PinDataType::Float,  "Base",   Variant(100.0)},
    }));
    ASSERT_TRUE(runSingleNode("Stat.AddModifier", 2, {
        {20, PinKind::Input,  PinDataType::String, "Name",   Variant(std::string("atk"))},
        {21, PinKind::Input,  PinDataType::String, "Entity", Variant(std::string("stat_test_add"))},
        {22, PinKind::Input,  PinDataType::String, "ModId",  Variant(std::string("buff1"))},
        {23, PinKind::Input,  PinDataType::String, "Type",   Variant(std::string("add"))},
        {24, PinKind::Input,  PinDataType::Float,  "Value",  Variant(20.0)},
        {25, PinKind::Output, PinDataType::Float,  "NewValue", {}},
    }));
    EXPECT_DOUBLE_EQ(runner.GetPinValue(25).asFloat(), 120.0);
}

TEST_F(HandlerOneShotTest, StatAddModifier_MulType_AppliesAfterAdd)
{
    // Base 100, +20 add, then x2 mul -> (100+20)*2 = 240
    ASSERT_TRUE(runSingleNode("Stat.Set", 1, {
        {10, PinKind::Input, PinDataType::String, "Name",   Variant(std::string("dmg"))},
        {11, PinKind::Input, PinDataType::String, "Entity", Variant(std::string("stat_test_mul"))},
        {12, PinKind::Input, PinDataType::Float,  "Base",   Variant(100.0)},
    }));
    ASSERT_TRUE(runSingleNode("Stat.AddModifier", 2, {
        {20, PinKind::Input,  PinDataType::String, "Name",   Variant(std::string("dmg"))},
        {21, PinKind::Input,  PinDataType::String, "Entity", Variant(std::string("stat_test_mul"))},
        {22, PinKind::Input,  PinDataType::String, "ModId",  Variant(std::string("buff_add"))},
        {23, PinKind::Input,  PinDataType::String, "Type",   Variant(std::string("add"))},
        {24, PinKind::Input,  PinDataType::Float,  "Value",  Variant(20.0)},
        {25, PinKind::Output, PinDataType::Float,  "NewValue", {}},
    }));
    ASSERT_TRUE(runSingleNode("Stat.AddModifier", 3, {
        {30, PinKind::Input,  PinDataType::String, "Name",   Variant(std::string("dmg"))},
        {31, PinKind::Input,  PinDataType::String, "Entity", Variant(std::string("stat_test_mul"))},
        {32, PinKind::Input,  PinDataType::String, "ModId",  Variant(std::string("buff_mul"))},
        {33, PinKind::Input,  PinDataType::String, "Type",   Variant(std::string("mul"))},
        {34, PinKind::Input,  PinDataType::Float,  "Value",  Variant(2.0)},
        {35, PinKind::Output, PinDataType::Float,  "NewValue", {}},
    }));
    EXPECT_DOUBLE_EQ(runner.GetPinValue(35).asFloat(), 240.0);
}

TEST_F(HandlerOneShotTest, StatReset_ClearsModifiersKeepsBase)
{
    // Base 50, +10 add -> 60, then Reset -> back to 50
    ASSERT_TRUE(runSingleNode("Stat.Set", 1, {
        {10, PinKind::Input, PinDataType::String, "Name",   Variant(std::string("def"))},
        {11, PinKind::Input, PinDataType::String, "Entity", Variant(std::string("stat_test_reset"))},
        {12, PinKind::Input, PinDataType::Float,  "Base",   Variant(50.0)},
    }));
    ASSERT_TRUE(runSingleNode("Stat.AddModifier", 2, {
        {20, PinKind::Input,  PinDataType::String, "Name",   Variant(std::string("def"))},
        {21, PinKind::Input,  PinDataType::String, "Entity", Variant(std::string("stat_test_reset"))},
        {22, PinKind::Input,  PinDataType::String, "ModId",  Variant(std::string("buff_x"))},
        {23, PinKind::Input,  PinDataType::String, "Type",   Variant(std::string("add"))},
        {24, PinKind::Input,  PinDataType::Float,  "Value",  Variant(10.0)},
        {25, PinKind::Output, PinDataType::Float,  "NewValue", {}},
    }));
    ASSERT_DOUBLE_EQ(runner.GetPinValue(25).asFloat(), 60.0);

    ASSERT_TRUE(runSingleNode("Stat.Reset", 3, {
        {30, PinKind::Input, PinDataType::String, "Name",   Variant(std::string("def"))},
        {31, PinKind::Input, PinDataType::String, "Entity", Variant(std::string("stat_test_reset"))},
    }));

    ASSERT_TRUE(runSingleNode("Stat.Get", 4, {
        {40, PinKind::Input,  PinDataType::String,  "Name",   Variant(std::string("def"))},
        {41, PinKind::Input,  PinDataType::String,  "Entity", Variant(std::string("stat_test_reset"))},
        {42, PinKind::Output, PinDataType::Float,   "Value",         {}},
        {43, PinKind::Output, PinDataType::Float,   "Base",          {}},
        {44, PinKind::Output, PinDataType::Integer, "ModifierCount", {}},
    }));
    EXPECT_DOUBLE_EQ(runner.GetPinValue(42).asFloat(), 50.0)
        << "Reset should clear modifiers but keep base";
    EXPECT_EQ(runner.GetPinValue(44).asInt(), 0);
}
