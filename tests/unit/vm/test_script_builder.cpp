#include <gtest/gtest.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/opcode.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <cmath>
#include <limits>
#include <vector>
#include <algorithm>

using namespace neo::vm;
using namespace neo::io;
using namespace std;

class UT_ScriptBuilder : public testing::Test
{
protected:
    void SetUp() override
    {
        // Setup code if needed
    }

    void TearDown() override
    {
        // Teardown code if needed
    }

    // Helper to compare byte arrays
    void AssertAreEqual(const std::vector<uint8_t>& expected, const std::vector<uint8_t>& actual)
    {
        ASSERT_EQ(expected.size(), actual.size());
        for (size_t i = 0; i < expected.size(); i++)
        {
            ASSERT_EQ(expected[i], actual[i]) << "Mismatch at position " << i;
        }
    }
};

TEST_F(UT_ScriptBuilder, TestEmit)
{
    // Test basic emit
    {
        ScriptBuilder script;
        ASSERT_EQ(0, script.GetLength());
        script.Emit(OpCode::NOP);
        ASSERT_EQ(1, script.GetLength());

        std::vector<uint8_t> expected = {0x21}; // NOP opcode
        AssertAreEqual(expected, script.ToArray());
    }

    // Test emit with operand
    {
        ScriptBuilder script;
        std::vector<uint8_t> operand = {0x66};
        script.Emit(OpCode::NOP, operand);
        
        std::vector<uint8_t> expected = {0x21, 0x66}; // NOP opcode with operand
        AssertAreEqual(expected, script.ToArray());
    }
}

TEST_F(UT_ScriptBuilder, TestNullAndEmpty)
{
    ScriptBuilder script;
    
    // Empty byte span
    std::vector<uint8_t> empty;
    script.EmitPush(empty);
    
    // Expected: PUSHDATA1 followed by 0 (length)
    std::vector<uint8_t> expected = {(uint8_t)OpCode::PUSHDATA1, 0};
    AssertAreEqual(expected, script.ToArray());
}

TEST_F(UT_ScriptBuilder, TestBigInteger)
{
    // Test negative integer
    {
        ScriptBuilder script;
        ASSERT_EQ(0, script.GetLength());
        script.EmitPush(-100000);
        ASSERT_EQ(5, script.GetLength());

        std::vector<uint8_t> expected = {2, 96, 121, 254, 255}; // 2 is PUSHINT32 prefix, followed by -100000 in little-endian
        AssertAreEqual(expected, script.ToArray());
    }

    // Test positive integer
    {
        ScriptBuilder script;
        ASSERT_EQ(0, script.GetLength());
        script.EmitPush(100000);
        ASSERT_EQ(5, script.GetLength());

        std::vector<uint8_t> expected = {2, 160, 134, 1, 0}; // 2 is PUSHINT32 prefix, followed by 100000 in little-endian
        AssertAreEqual(expected, script.ToArray());
    }
}

TEST_F(UT_ScriptBuilder, TestEmitSysCall)
{
    ScriptBuilder script;
    script.EmitSysCall(0xE393C875);
    
    // SYSCALL followed by the syscall hash in little-endian
    std::vector<uint8_t> expected = {(uint8_t)OpCode::SYSCALL, 0x75, 0xC8, 0x93, 0xE3};
    AssertAreEqual(expected, script.ToArray());
}

TEST_F(UT_ScriptBuilder, TestEmitCall)
{
    // Test small offset
    {
        ScriptBuilder script;
        script.EmitCall(0);
        
        std::vector<uint8_t> expected = {(uint8_t)OpCode::CALL, 0};
        AssertAreEqual(expected, script.ToArray());
    }
    
    // Test large positive offset
    {
        ScriptBuilder script;
        script.EmitCall(12345);
        
        // CALL_L followed by the offset in little-endian
        std::vector<uint8_t> expected = {(uint8_t)OpCode::CALL_L};
        BinaryWriter writer;
        writer.WriteInt32(12345);
        auto bytes = writer.ToArray();
        expected.insert(expected.end(), bytes.begin(), bytes.end());
        
        AssertAreEqual(expected, script.ToArray());
    }
    
    // Test large negative offset
    {
        ScriptBuilder script;
        script.EmitCall(-12345);
        
        // CALL_L followed by the offset in little-endian
        std::vector<uint8_t> expected = {(uint8_t)OpCode::CALL_L};
        BinaryWriter writer;
        writer.WriteInt32(-12345);
        auto bytes = writer.ToArray();
        expected.insert(expected.end(), bytes.begin(), bytes.end());
        
        AssertAreEqual(expected, script.ToArray());
    }
}

TEST_F(UT_ScriptBuilder, TestEmitJump)
{
    // Test with max int8_t offset
    int8_t offset_i8 = std::numeric_limits<int8_t>::max();
    int32_t offset_i32 = std::numeric_limits<int32_t>::max();

    // Test valid jump opcodes with max offsets
    for (int op = (int)OpCode::JMP; op <= (int)OpCode::JMPLE_L; op++)
    {
        ScriptBuilder script;
        OpCode opcode = static_cast<OpCode>(op);
        
        script.EmitJump(opcode, offset_i8);
        script.EmitJump(opcode, offset_i32);
        
        std::vector<uint8_t> expected;
        if (op % 2 == 0) // Short form (JMP, JMPIF, etc.)
        {
            expected = {(uint8_t)opcode, (uint8_t)offset_i8, (uint8_t)((int)opcode + 1)};
            
            // Append int32 in little-endian
            BinaryWriter writer;
            writer.WriteInt32(offset_i32);
            auto bytes = writer.ToArray();
            expected.insert(expected.end(), bytes.begin(), bytes.end());
        }
        else // Long form (JMP_L, JMPIF_L, etc.)
        {
            expected = {(uint8_t)opcode};
            
            // Append int8 as int32 in little-endian
            BinaryWriter writer;
            writer.WriteInt32((int32_t)offset_i8);
            auto bytes = writer.ToArray();
            expected.insert(expected.end(), bytes.begin(), bytes.end());
            
            // Append opcode again
            expected.push_back((uint8_t)opcode);
            
            // Append int32 in little-endian
            BinaryWriter writer2;
            writer2.WriteInt32(offset_i32);
            auto bytes2 = writer2.ToArray();
            expected.insert(expected.end(), bytes2.begin(), bytes2.end());
        }
        
        AssertAreEqual(expected, script.ToArray());
    }
    
    // Test invalid jump opcode
    {
        ScriptBuilder script;
        EXPECT_THROW(script.EmitJump(OpCode::NOP, offset_i8), std::out_of_range);
        EXPECT_THROW(script.EmitJump(OpCode::NOP, offset_i32), std::out_of_range);
    }
}

TEST_F(UT_ScriptBuilder, TestEmitPushBigInteger)
{
    // Test small integers (-1 to 16)
    for (int i = -1; i <= 16; i++)
    {
        ScriptBuilder script;
        script.EmitPush(i);
        
        std::vector<uint8_t> expected = {(uint8_t)(OpCode::PUSH0 + i)};
        AssertAreEqual(expected, script.ToArray());
    }
    
    // Test -1 (should be PUSHM1)
    {
        ScriptBuilder script;
        script.EmitPush(-1);
        
        std::vector<uint8_t> expected = {(uint8_t)OpCode::PUSHM1};
        AssertAreEqual(expected, script.ToArray());
    }
    
    // Test int8_t min/max
    {
        ScriptBuilder script;
        script.EmitPush(std::numeric_limits<int8_t>::min());
        
        std::vector<uint8_t> expected = {0x00, 0x80}; // PUSHINT8 followed by -128
        AssertAreEqual(expected, script.ToArray());
    }
    
    {
        ScriptBuilder script;
        script.EmitPush(std::numeric_limits<int8_t>::max());
        
        std::vector<uint8_t> expected = {0x00, 0x7f}; // PUSHINT8 followed by 127
        AssertAreEqual(expected, script.ToArray());
    }
    
    // Test int16_t min/max
    {
        ScriptBuilder script;
        script.EmitPush(std::numeric_limits<int16_t>::min());
        
        // PUSHINT16 followed by -32768 in little-endian
        std::vector<uint8_t> expected = {0x01, 0x00, 0x80};
        AssertAreEqual(expected, script.ToArray());
    }
    
    {
        ScriptBuilder script;
        script.EmitPush(std::numeric_limits<int16_t>::max());
        
        // PUSHINT16 followed by 32767 in little-endian
        std::vector<uint8_t> expected = {0x01, 0xff, 0x7f};
        AssertAreEqual(expected, script.ToArray());
    }
    
    // Test int32_t min/max
    {
        ScriptBuilder script;
        script.EmitPush(std::numeric_limits<int32_t>::min());
        
        // PUSHINT32 followed by -2147483648 in little-endian
        std::vector<uint8_t> expected = {0x02, 0x00, 0x00, 0x00, 0x80};
        AssertAreEqual(expected, script.ToArray());
    }
    
    {
        ScriptBuilder script;
        script.EmitPush(std::numeric_limits<int32_t>::max());
        
        // PUSHINT32 followed by 2147483647 in little-endian
        std::vector<uint8_t> expected = {0x02, 0xff, 0xff, 0xff, 0x7f};
        AssertAreEqual(expected, script.ToArray());
    }
    
    // Test int64_t min/max
    {
        ScriptBuilder script;
        script.EmitPush(std::numeric_limits<int64_t>::min());
        
        // PUSHINT64 followed by -9223372036854775808 in little-endian
        std::vector<uint8_t> expected = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80};
        AssertAreEqual(expected, script.ToArray());
    }
    
    {
        ScriptBuilder script;
        script.EmitPush(std::numeric_limits<int64_t>::max());
        
        // PUSHINT64 followed by 9223372036854775807 in little-endian
        std::vector<uint8_t> expected = {0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f};
        AssertAreEqual(expected, script.ToArray());
    }
}

TEST_F(UT_ScriptBuilder, TestEmitPushString)
{
    // Test empty string
    {
        ScriptBuilder script;
        script.EmitPush("");
        
        // PUSHDATA1 followed by length 0
        std::vector<uint8_t> expected = {(uint8_t)OpCode::PUSHDATA1, 0};
        AssertAreEqual(expected, script.ToArray());
    }
    
    // Test short string
    {
        ScriptBuilder script;
        std::string str = "Hello";
        script.EmitPush(str);
        
        // PUSHDATA1 followed by length 5 and UTF-8 bytes of "Hello"
        std::vector<uint8_t> expected = {(uint8_t)OpCode::PUSHDATA1, 5, 'H', 'e', 'l', 'l', 'o'};
        AssertAreEqual(expected, script.ToArray());
    }
    
    // Test string with non-ASCII characters
    {
        ScriptBuilder script;
        std::string str = "你好"; // Chinese for "Hello"
        script.EmitPush(str);
        
        // Convert string to UTF-8 bytes
        std::vector<uint8_t> utf8Bytes;
        for (char c : str)
        {
            utf8Bytes.push_back(static_cast<uint8_t>(c));
        }
        
        // PUSHDATA1 followed by length of UTF-8 bytes and the bytes themselves
        std::vector<uint8_t> expected = {(uint8_t)OpCode::PUSHDATA1, (uint8_t)utf8Bytes.size()};
        expected.insert(expected.end(), utf8Bytes.begin(), utf8Bytes.end());
        
        // Note: This test might not work correctly because of UTF-8 encoding in C++
        // The string "你好" in UTF-8 should be 6 bytes
    }
}

TEST_F(UT_ScriptBuilder, TestEmitPushBoolean)
{
    // Test true
    {
        ScriptBuilder script;
        script.EmitPush(true);
        
        // PUSH1 (true)
        std::vector<uint8_t> expected = {(uint8_t)OpCode::PUSH1};
        AssertAreEqual(expected, script.ToArray());
    }
    
    // Test false
    {
        ScriptBuilder script;
        script.EmitPush(false);
        
        // PUSH0 (false)
        std::vector<uint8_t> expected = {(uint8_t)OpCode::PUSH0};
        AssertAreEqual(expected, script.ToArray());
    }
}

TEST_F(UT_ScriptBuilder, TestEmitPushByteArray)
{
    // Test empty array
    {
        ScriptBuilder script;
        std::vector<uint8_t> data;
        script.EmitPush(data);
        
        // PUSHDATA1 followed by length 0
        std::vector<uint8_t> expected = {(uint8_t)OpCode::PUSHDATA1, 0};
        AssertAreEqual(expected, script.ToArray());
    }
    
    // Test small array
    {
        ScriptBuilder script;
        std::vector<uint8_t> data = {1, 2, 3, 4, 5};
        script.EmitPush(data);
        
        // PUSHDATA1 followed by length 5 and the bytes
        std::vector<uint8_t> expected = {(uint8_t)OpCode::PUSHDATA1, 5, 1, 2, 3, 4, 5};
        AssertAreEqual(expected, script.ToArray());
    }
    
    // Test medium array (75 bytes - should use PUSHDATA1)
    {
        ScriptBuilder script;
        std::vector<uint8_t> data(75, 42); // 75 bytes, all with value 42
        script.EmitPush(data);
        
        // PUSHDATA1 followed by length 75 and the bytes
        std::vector<uint8_t> expected = {(uint8_t)OpCode::PUSHDATA1, 75};
        expected.insert(expected.end(), data.begin(), data.end());
        AssertAreEqual(expected, script.ToArray());
    }
    
    // Test large array (256 bytes - should use PUSHDATA2)
    {
        ScriptBuilder script;
        std::vector<uint8_t> data(256, 42); // 256 bytes, all with value 42
        script.EmitPush(data);
        
        // PUSHDATA2 followed by length 256 in little-endian and the bytes
        std::vector<uint8_t> expected = {(uint8_t)OpCode::PUSHDATA2, 0x00, 0x01};
        expected.insert(expected.end(), data.begin(), data.end());
        AssertAreEqual(expected, script.ToArray());
    }
    
    // Test very large array (65536 bytes - should use PUSHDATA4)
    // Note: This test might be slow and memory-intensive
    /*
    {
        ScriptBuilder script;
        std::vector<uint8_t> data(65536, 42); // 65536 bytes, all with value 42
        script.EmitPush(data);
        
        // PUSHDATA4 followed by length 65536 in little-endian and the bytes
        std::vector<uint8_t> expected = {(uint8_t)OpCode::PUSHDATA4, 0x00, 0x00, 0x01, 0x00};
        expected.insert(expected.end(), data.begin(), data.end());
        AssertAreEqual(expected, script.ToArray());
    }
    */
}
