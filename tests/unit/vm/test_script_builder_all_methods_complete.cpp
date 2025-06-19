#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "neo/vm/script_builder.h"
#include "neo/vm/opcode.h"
#include "neo/extensions/utility.h"
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <random>
#include <numeric>

using namespace neo;
using namespace neo::vm;

// Complete conversion of C# UT_ScriptBuilder.cs - ALL 11 test methods
class ScriptBuilderAllMethodsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize random number generator for helper functions
        rng_.seed(12345); // Fixed seed for reproducible tests
    }
    
    void TearDown() override {
        // Clean up resources if needed
    }
    
    // Helper functions equivalent to C# RandomHelper
    std::vector<uint8_t> RandBuffer(size_t length) {
        std::vector<uint8_t> buffer(length);
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        for (size_t i = 0; i < length; i++) {
            buffer[i] = dist(rng_);
        }
        return buffer;
    }
    
    std::string RandString(size_t length) {
        std::string str(length, 0);
        std::uniform_int_distribution<char> dist('a', 'z');
        for (size_t i = 0; i < length; i++) {
            str[i] = dist(rng_);
        }
        return str;
    }
    
    // Helper to convert bytes to hex string like C# ToHexString()
    std::string ToHexString(const std::vector<uint8_t>& bytes) {
        return "0x" + Utility::ToHexString(bytes);
    }
    
    // Helper to compare byte arrays like C# CollectionAssert.AreEqual
    void AssertBytesEqual(const std::vector<uint8_t>& expected, const std::vector<uint8_t>& actual) {
        EXPECT_EQ(expected.size(), actual.size());
        EXPECT_TRUE(std::equal(expected.begin(), expected.end(), actual.begin()));
    }
    
    std::mt19937 rng_;
};

// C# Test Method: TestEmit()
TEST_F(ScriptBuilderAllMethodsTest, TestEmit) {
    // Test basic emit
    {
        ScriptBuilder script;
        EXPECT_EQ(0, script.Length());
        script.Emit(OpCode::NOP);
        EXPECT_EQ(1, script.Length());
        
        std::vector<uint8_t> expected = {0x21}; // NOP opcode
        AssertBytesEqual(expected, script.ToArray());
    }
    
    // Test emit with data
    {
        ScriptBuilder script;
        std::vector<uint8_t> data = {0x66};
        script.Emit(OpCode::NOP, data);
        std::vector<uint8_t> expected = {0x21, 0x66}; // NOP + data
        AssertBytesEqual(expected, script.ToArray());
    }
}

// C# Test Method: TestNullAndEmpty()
TEST_F(ScriptBuilderAllMethodsTest, TestNullAndEmpty) {
    ScriptBuilder script;
    
    // Test null span (empty in C++)
    std::vector<uint8_t> null_span;
    script.EmitPush(null_span);
    
    // Test empty span
    std::vector<uint8_t> empty_span;
    script.EmitPush(empty_span);
    
    std::vector<uint8_t> expected = {
        static_cast<uint8_t>(OpCode::PUSHDATA1), 0,
        static_cast<uint8_t>(OpCode::PUSHDATA1), 0
    };
    AssertBytesEqual(expected, script.ToArray());
}

// C# Test Method: TestBigInteger()
TEST_F(ScriptBuilderAllMethodsTest, TestBigInteger) {
    // Test negative big integer
    {
        ScriptBuilder script;
        EXPECT_EQ(0, script.Length());
        script.EmitPush(BigInteger(-100000));
        EXPECT_EQ(5, script.Length());
        
        std::vector<uint8_t> expected = {2, 96, 121, 254, 255};
        AssertBytesEqual(expected, script.ToArray());
    }
    
    // Test positive big integer
    {
        ScriptBuilder script;
        EXPECT_EQ(0, script.Length());
        script.EmitPush(BigInteger(100000));
        EXPECT_EQ(5, script.Length());
        
        std::vector<uint8_t> expected = {2, 160, 134, 1, 0};
        AssertBytesEqual(expected, script.ToArray());
    }
}

// C# Test Method: TestEmitSysCall()
TEST_F(ScriptBuilderAllMethodsTest, TestEmitSysCall) {
    ScriptBuilder script;
    script.EmitSysCall(0xE393C875);
    
    std::vector<uint8_t> expected = {
        static_cast<uint8_t>(OpCode::SYSCALL), 
        0x75, 0xC8, 0x93, 0xE3
    };
    AssertBytesEqual(expected, script.ToArray());
}

// C# Test Method: TestEmitCall()
TEST_F(ScriptBuilderAllMethodsTest, TestEmitCall) {
    // Test short call (offset 0)
    {
        ScriptBuilder script;
        script.EmitCall(0);
        std::vector<uint8_t> expected = {
            static_cast<uint8_t>(OpCode::CALL), 
            static_cast<uint8_t>(0)
        };
        AssertBytesEqual(expected, script.ToArray());
    }
    
    // Test long call (positive offset)
    {
        ScriptBuilder script;
        script.EmitCall(12345);
        std::vector<uint8_t> expected = {static_cast<uint8_t>(OpCode::CALL_L)};
        
        // Add little-endian bytes for 12345
        uint32_t offset = 12345;
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&offset);
        for (int i = 0; i < 4; i++) {
            expected.push_back(bytes[i]);
        }
        
        AssertBytesEqual(expected, script.ToArray());
    }
    
    // Test long call (negative offset)
    {
        ScriptBuilder script;
        script.EmitCall(-12345);
        std::vector<uint8_t> expected = {static_cast<uint8_t>(OpCode::CALL_L)};
        
        // Add little-endian bytes for -12345
        int32_t offset = -12345;
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&offset);
        for (int i = 0; i < 4; i++) {
            expected.push_back(bytes[i]);
        }
        
        AssertBytesEqual(expected, script.ToArray());
    }
}

// C# Test Method: TestEmitJump()
TEST_F(ScriptBuilderAllMethodsTest, TestEmitJump) {
    int8_t offset_i8 = std::numeric_limits<int8_t>::max();
    int32_t offset_i32 = std::numeric_limits<int32_t>::max();
    
    // Test all jump opcodes with max values
    for (int op_val = static_cast<int>(OpCode::JMP); 
         op_val <= static_cast<int>(OpCode::JMPLE_L); 
         op_val++) {
        
        OpCode op = static_cast<OpCode>(op_val);
        ScriptBuilder script;
        
        script.EmitJump(op, offset_i8);
        script.EmitJump(op, offset_i32);
        
        std::vector<uint8_t> expected;
        
        if (op_val % 2 == 0) {
            // Short jump version
            expected.push_back(static_cast<uint8_t>(op));
            expected.push_back(static_cast<uint8_t>(offset_i8));
            
            // Long jump version
            expected.push_back(static_cast<uint8_t>(op_val + 1));
            uint8_t* bytes = reinterpret_cast<uint8_t*>(&offset_i32);
            for (int i = 0; i < 4; i++) {
                expected.push_back(bytes[i]);
            }
        } else {
            // Long jump version only
            expected.push_back(static_cast<uint8_t>(op));
            int32_t ext_offset_i8 = static_cast<int32_t>(offset_i8);
            uint8_t* bytes1 = reinterpret_cast<uint8_t*>(&ext_offset_i8);
            for (int i = 0; i < 4; i++) {
                expected.push_back(bytes1[i]);
            }
            
            expected.push_back(static_cast<uint8_t>(op));
            uint8_t* bytes2 = reinterpret_cast<uint8_t*>(&offset_i32);
            for (int i = 0; i < 4; i++) {
                expected.push_back(bytes2[i]);
            }
        }
        
        AssertBytesEqual(expected, script.ToArray());
    }
    
    // Test with min values
    offset_i8 = std::numeric_limits<int8_t>::min();
    offset_i32 = std::numeric_limits<int32_t>::min();
    
    for (int op_val = static_cast<int>(OpCode::JMP); 
         op_val <= static_cast<int>(OpCode::JMPLE_L); 
         op_val++) {
        
        OpCode op = static_cast<OpCode>(op_val);
        ScriptBuilder script;
        
        script.EmitJump(op, offset_i8);
        script.EmitJump(op, offset_i32);
        
        std::vector<uint8_t> expected;
        
        if (op_val % 2 == 0) {
            // Short jump version
            expected.push_back(static_cast<uint8_t>(op));
            expected.push_back(static_cast<uint8_t>(offset_i8));
            
            // Long jump version
            expected.push_back(static_cast<uint8_t>(op_val + 1));
            uint8_t* bytes = reinterpret_cast<uint8_t*>(&offset_i32);
            for (int i = 0; i < 4; i++) {
                expected.push_back(bytes[i]);
            }
        } else {
            // Long jump version only
            expected.push_back(static_cast<uint8_t>(op));
            int32_t ext_offset_i8 = static_cast<int32_t>(offset_i8);
            uint8_t* bytes1 = reinterpret_cast<uint8_t*>(&ext_offset_i8);
            for (int i = 0; i < 4; i++) {
                expected.push_back(bytes1[i]);
            }
            
            expected.push_back(static_cast<uint8_t>(op));
            uint8_t* bytes2 = reinterpret_cast<uint8_t*>(&offset_i32);
            for (int i = 0; i < 4; i++) {
                expected.push_back(bytes2[i]);
            }
        }
        
        AssertBytesEqual(expected, script.ToArray());
    }
    
    // Test invalid opcodes
    for (int op_val = 0; op_val < static_cast<int>(OpCode::JMP); op_val++) {
        OpCode op = static_cast<OpCode>(op_val);
        ScriptBuilder script;
        EXPECT_THROW(script.EmitJump(op, offset_i8), std::out_of_range);
        EXPECT_THROW(script.EmitJump(op, offset_i32), std::out_of_range);
    }
    
    for (int op_val = static_cast<int>(OpCode::JMPLE_L) + 1; op_val <= 255; op_val++) {
        OpCode op = static_cast<OpCode>(op_val);
        ScriptBuilder script;
        EXPECT_THROW(script.EmitJump(op, offset_i8), std::out_of_range);
        EXPECT_THROW(script.EmitJump(op, offset_i32), std::out_of_range);
    }
}

// C# Test Method: TestEmitPushBigInteger()
TEST_F(ScriptBuilderAllMethodsTest, TestEmitPushBigInteger) {
    // Test small integers (-1 to 16)
    for (int i = -1; i <= 16; i++) {
        ScriptBuilder script;
        script.EmitPush(BigInteger(i));
        std::vector<uint8_t> expected = {
            static_cast<uint8_t>(static_cast<int>(OpCode::PUSH0) + i)
        };
        AssertBytesEqual(expected, script.ToArray());
    }
    
    // Test -1 specifically
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger(-1));
        EXPECT_EQ("0x0f", ToHexString(script.ToArray()));
    }
    
    // Test PUSHINT8 edge cases
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger(std::numeric_limits<int8_t>::min()));
        EXPECT_EQ("0x0080", ToHexString(script.ToArray()));
    }
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger(std::numeric_limits<int8_t>::max()));
        EXPECT_EQ("0x007f", ToHexString(script.ToArray()));
    }
    
    // Test PUSHINT16 edge cases
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger(std::numeric_limits<int16_t>::min()));
        EXPECT_EQ("0x010080", ToHexString(script.ToArray()));
    }
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger(std::numeric_limits<int16_t>::max()));
        EXPECT_EQ("0x01ff7f", ToHexString(script.ToArray()));
    }
    
    // Test PUSHINT32 edge cases
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger(std::numeric_limits<int32_t>::min()));
        EXPECT_EQ("0x0200000080", ToHexString(script.ToArray()));
    }
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger(std::numeric_limits<int32_t>::max()));
        EXPECT_EQ("0x02ffffff7f", ToHexString(script.ToArray()));
    }
    
    // Test PUSHINT64 edge cases
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger(std::numeric_limits<int64_t>::min()));
        EXPECT_EQ("0x030000000000000080", ToHexString(script.ToArray()));
    }
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger(std::numeric_limits<int64_t>::max()));
        EXPECT_EQ("0x03ffffffffffffff7f", ToHexString(script.ToArray()));
    }
    
    // Test PUSHINT128
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger::Parse("18446744073709551615")); // uint64 max
        EXPECT_EQ("0x04ffffffffffffffff0000000000000000", ToHexString(script.ToArray()));
    }
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger::Parse("18446744073709551616")); // uint64 max + 1
        EXPECT_EQ("0x0400000000000000000100000000000000", ToHexString(script.ToArray()));
    }
    
    // Test PUSHINT256 edge cases
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger::Parse("-57896044618658097711785492504343953926634992332820282019728792003956564819968"));
        EXPECT_EQ("0x050000000000000000000000000000000000000000000000000000000000000080", 
                  ToHexString(script.ToArray()));
    }
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger::Parse("57896044618658097711785492504343953926634992332820282019728792003956564819967"));
        EXPECT_EQ("0x05ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f", 
                  ToHexString(script.ToArray()));
    }
    
    // Test exceeding 256-bit value (should throw)
    {
        ScriptBuilder script;
        EXPECT_THROW(script.EmitPush(BigInteger::Parse("115792089237316195423570985008687907853269984665640564039457584007913129639936")), 
                     std::out_of_range);
    }
    
    // Test negative numbers
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger(-2));
        EXPECT_EQ("0x00fe", ToHexString(script.ToArray()));
    }
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger(-256));
        EXPECT_EQ("0x0100ff", ToHexString(script.ToArray()));
    }
    
    // Test boundary values
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger::Parse("18446744073709551615"));
        EXPECT_EQ("0x04ffffffffffffffff0000000000000000", ToHexString(script.ToArray()));
    }
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger::Parse("18446744073709551616"));
        EXPECT_EQ("0x0400000000000000000100000000000000", ToHexString(script.ToArray()));
    }
    
    // Test very large negative number
    {
        ScriptBuilder script;
        script.EmitPush(BigInteger::Parse("-18446744073709551616"));
        EXPECT_EQ("0x040000000000000000ffffffffffffffff", ToHexString(script.ToArray()));
    }
    
    // Test exception for too large BigInteger
    {
        ScriptBuilder script;
        EXPECT_THROW(script.EmitPush(BigInteger::Parse("115792089237316195423570985008687907853269984665640564039457584007913129639937")), 
                     std::out_of_range);
    }
}

// C# Test Method: TestEmitPushBool()
TEST_F(ScriptBuilderAllMethodsTest, TestEmitPushBool) {
    // Test push true
    {
        ScriptBuilder script;
        script.EmitPush(true);
        std::vector<uint8_t> expected = {static_cast<uint8_t>(OpCode::PUSHT)};
        AssertBytesEqual(expected, script.ToArray());
    }
    
    // Test push false
    {
        ScriptBuilder script;
        script.EmitPush(false);
        std::vector<uint8_t> expected = {static_cast<uint8_t>(OpCode::PUSHF)};
        AssertBytesEqual(expected, script.ToArray());
    }
}

// C# Test Method: TestEmitPushReadOnlySpan()
TEST_F(ScriptBuilderAllMethodsTest, TestEmitPushReadOnlySpan) {
    ScriptBuilder script;
    std::vector<uint8_t> data = {0x01, 0x02};
    script.EmitPush(data); // C++ equivalent of ReadOnlySpan<byte>
    
    std::vector<uint8_t> expected = {
        static_cast<uint8_t>(OpCode::PUSHDATA1), 
        static_cast<uint8_t>(data.size())
    };
    expected.insert(expected.end(), data.begin(), data.end());
    
    AssertBytesEqual(expected, script.ToArray());
}

// C# Test Method: TestEmitPushByteArray()
TEST_F(ScriptBuilderAllMethodsTest, TestEmitPushByteArray) {
    // Test null array (empty in C++)
    {
        ScriptBuilder script;
        std::vector<uint8_t> null_data;
        script.EmitPush(null_data);
        std::vector<uint8_t> expected = {
            static_cast<uint8_t>(OpCode::PUSHDATA1), 0
        };
        AssertBytesEqual(expected, script.ToArray());
    }
    
    // Test PUSHDATA1 (up to 0x4C bytes)
    {
        ScriptBuilder script;
        auto data = RandBuffer(0x4C);
        script.EmitPush(data);
        
        std::vector<uint8_t> expected = {
            static_cast<uint8_t>(OpCode::PUSHDATA1), 
            static_cast<uint8_t>(data.size())
        };
        expected.insert(expected.end(), data.begin(), data.end());
        
        AssertBytesEqual(expected, script.ToArray());
    }
    
    // Test PUSHDATA2 (0x100 bytes)
    {
        ScriptBuilder script;
        auto data = RandBuffer(0x100);
        script.EmitPush(data);
        
        std::vector<uint8_t> expected = {static_cast<uint8_t>(OpCode::PUSHDATA2)};
        
        // Add little-endian length
        uint16_t length = static_cast<uint16_t>(data.size());
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&length);
        expected.push_back(bytes[0]);
        expected.push_back(bytes[1]);
        
        expected.insert(expected.end(), data.begin(), data.end());
        
        AssertBytesEqual(expected, script.ToArray());
    }
    
    // Test PUSHDATA4 (0x10000 bytes)
    {
        ScriptBuilder script;
        auto data = RandBuffer(0x10000);
        script.EmitPush(data);
        
        std::vector<uint8_t> expected = {static_cast<uint8_t>(OpCode::PUSHDATA4)};
        
        // Add little-endian length
        uint32_t length = static_cast<uint32_t>(data.size());
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&length);
        for (int i = 0; i < 4; i++) {
            expected.push_back(bytes[i]);
        }
        
        expected.insert(expected.end(), data.begin(), data.end());
        
        AssertBytesEqual(expected, script.ToArray());
    }
}

// C# Test Method: TestEmitPushString()
TEST_F(ScriptBuilderAllMethodsTest, TestEmitPushString) {
    // Test null string (should throw)
    {
        ScriptBuilder script;
        EXPECT_THROW(script.EmitPush(static_cast<const char*>(nullptr)), std::invalid_argument);
    }
    
    // Test PUSHDATA1 string (0x4C chars)
    {
        ScriptBuilder script;
        auto data = RandString(0x4C);
        script.EmitPush(data);
        
        std::vector<uint8_t> expected = {
            static_cast<uint8_t>(OpCode::PUSHDATA1), 
            static_cast<uint8_t>(data.length())
        };
        
        // Add UTF-8 bytes
        for (char c : data) {
            expected.push_back(static_cast<uint8_t>(c));
        }
        
        AssertBytesEqual(expected, script.ToArray());
    }
    
    // Test PUSHDATA2 string (0x100 chars)
    {
        ScriptBuilder script;
        auto data = RandString(0x100);
        script.EmitPush(data);
        
        std::vector<uint8_t> expected = {static_cast<uint8_t>(OpCode::PUSHDATA2)};
        
        // Add little-endian length
        uint16_t length = static_cast<uint16_t>(data.length());
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&length);
        expected.push_back(bytes[0]);
        expected.push_back(bytes[1]);
        
        // Add UTF-8 bytes
        for (char c : data) {
            expected.push_back(static_cast<uint8_t>(c));
        }
        
        AssertBytesEqual(expected, script.ToArray());
    }
    
    // Test PUSHDATA4 string (0x10000 chars)
    {
        ScriptBuilder script;
        auto data = RandString(0x10000);
        script.EmitPush(data);
        
        std::vector<uint8_t> expected = {static_cast<uint8_t>(OpCode::PUSHDATA4)};
        
        // Add little-endian length
        uint32_t length = static_cast<uint32_t>(data.length());
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&length);
        for (int i = 0; i < 4; i++) {
            expected.push_back(bytes[i]);
        }
        
        // Add UTF-8 bytes
        for (char c : data) {
            expected.push_back(static_cast<uint8_t>(c));
        }
        
        AssertBytesEqual(expected, script.ToArray());
    }
}

// Additional comprehensive tests for ScriptBuilder completeness

// Test Method: TestScriptBuilderConstruction()
TEST_F(ScriptBuilderAllMethodsTest, TestScriptBuilderConstruction) {
    // Test default constructor
    ScriptBuilder script;
    EXPECT_EQ(0, script.Length());
    EXPECT_TRUE(script.ToArray().empty());
    
    // Test after operations
    script.Emit(OpCode::NOP);
    EXPECT_EQ(1, script.Length());
    EXPECT_FALSE(script.ToArray().empty());
}

// Test Method: TestScriptBuilderReset()
TEST_F(ScriptBuilderAllMethodsTest, TestScriptBuilderReset) {
    ScriptBuilder script;
    script.Emit(OpCode::NOP);
    script.EmitPush(true);
    script.EmitPush(42);
    
    EXPECT_GT(script.Length(), 0);
    
    script.Clear(); // Reset the builder
    EXPECT_EQ(0, script.Length());
    EXPECT_TRUE(script.ToArray().empty());
}

// Test Method: TestComplexScript()
TEST_F(ScriptBuilderAllMethodsTest, TestComplexScript) {
    ScriptBuilder script;
    
    // Build a complex script with various operations
    script.EmitPush(42);                    // Push integer
    script.EmitPush("Hello World");         // Push string
    script.EmitPush(true);                  // Push boolean
    script.Emit(OpCode::ADD);               // Add operation
    script.EmitCall(100);                   // Call operation
    script.EmitJump(OpCode::JMP, 50);       // Jump operation
    script.EmitSysCall(0x12345678);         // System call
    
    auto result = script.ToArray();
    EXPECT_GT(result.size(), 10); // Should be substantial
    
    // Verify the script contains expected opcodes
    bool has_syscall = std::find(result.begin(), result.end(), 
                                static_cast<uint8_t>(OpCode::SYSCALL)) != result.end();
    bool has_add = std::find(result.begin(), result.end(), 
                            static_cast<uint8_t>(OpCode::ADD)) != result.end();
    bool has_call = std::find(result.begin(), result.end(), 
                             static_cast<uint8_t>(OpCode::CALL)) != result.end();
    
    EXPECT_TRUE(has_syscall);
    EXPECT_TRUE(has_add);
    EXPECT_TRUE(has_call);
}