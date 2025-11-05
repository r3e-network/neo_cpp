#include <gtest/gtest.h>

#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script_builder.h>

#include <limits>
#include <vector>

using namespace neo;
using namespace neo::vm;

namespace
{
std::vector<uint8_t> ToVec(const io::ByteVector& bytes) { return std::vector<uint8_t>(bytes.begin(), bytes.end()); }

std::vector<uint8_t> Concat(const std::vector<uint8_t>& first, const io::ByteVector& second)
{
    std::vector<uint8_t> merged = first;
    merged.insert(merged.end(), second.begin(), second.end());
    return merged;
}

template <typename T>
io::ByteVector LittleEndianBytes(T value)
{
    io::ByteVector buffer(sizeof(T));
    std::memcpy(buffer.Data(), &value, sizeof(T));
    return buffer;
}
}  // namespace

class ScriptBuilderTest : public ::testing::Test
{
  protected:
    std::vector<uint8_t> ToArray(ScriptBuilder& builder) { return ToVec(builder.ToArray()); }
};

TEST_F(ScriptBuilderTest, EmitWritesOpcodeAndOperand)
{
    ScriptBuilder builder;
    EXPECT_TRUE(builder.ToArray().IsEmpty());

    builder.Emit(OpCode::NOP);
    EXPECT_EQ(ToArray(builder), std::vector<uint8_t>{static_cast<uint8_t>(OpCode::NOP)});

    ScriptBuilder builderWithOperand;
    const std::vector<uint8_t> operand{0x66};
    builderWithOperand.Emit(OpCode::NOP, io::ByteSpan(operand));
    EXPECT_EQ(ToArray(builderWithOperand),
              (std::vector<uint8_t>{static_cast<uint8_t>(OpCode::NOP), 0x66}));
}

TEST_F(ScriptBuilderTest, EmitPushHandlesNullAndEmptySpans)
{
    ScriptBuilder builder;
    builder.EmitPush(io::ByteSpan());          // null span semantics
    builder.EmitPush(io::ByteSpan(nullptr, 0));  // explicit empty span

    std::vector<uint8_t> expected = {static_cast<uint8_t>(OpCode::PUSHDATA1), 0x00,
                                     static_cast<uint8_t>(OpCode::PUSHDATA1), 0x00};
    EXPECT_EQ(ToArray(builder), expected);
}

TEST_F(ScriptBuilderTest, EmitPushBigIntegerMatchesCSharpBehaviour)
{
    {
        ScriptBuilder builder;
        builder.EmitPush(static_cast<int64_t>(-100000));
        std::vector<uint8_t> expected = {static_cast<uint8_t>(OpCode::PUSHINT32), 0x60, 0x79, 0xFE, 0xFF};
        EXPECT_EQ(ToArray(builder), expected);
    }
    {
        ScriptBuilder builder;
        builder.EmitPush(static_cast<int64_t>(100000));
        std::vector<uint8_t> expected = {static_cast<uint8_t>(OpCode::PUSHINT32), 0xA0, 0x86, 0x01, 0x00};
        EXPECT_EQ(ToArray(builder), expected);
    }
}

TEST_F(ScriptBuilderTest, EmitSysCallWritesHashLittleEndian)
{
    ScriptBuilder builder;
    builder.EmitSysCall(0xE393C875);
    std::vector<uint8_t> expected = {static_cast<uint8_t>(OpCode::SYSCALL), 0x75, 0xC8, 0x93, 0xE3};
    EXPECT_EQ(ToArray(builder), expected);
}

TEST_F(ScriptBuilderTest, EmitCallChoosesShortOrLongForm)
{
    {
        ScriptBuilder builder;
        builder.EmitCall(0);
        std::vector<uint8_t> expected = {static_cast<uint8_t>(OpCode::CALL), 0x00};
        EXPECT_EQ(ToArray(builder), expected);
    }
    {
        ScriptBuilder builder;
        builder.EmitCall(12345);
        std::vector<uint8_t> expected = Concat({static_cast<uint8_t>(OpCode::CALL_L)},
                                               LittleEndianBytes<int32_t>(12345));
        EXPECT_EQ(ToArray(builder), expected);
    }
    {
        ScriptBuilder builder;
        builder.EmitCall(-12345);
        std::vector<uint8_t> expected = Concat({static_cast<uint8_t>(OpCode::CALL_L)},
                                               LittleEndianBytes<int32_t>(-12345));
        EXPECT_EQ(ToArray(builder), expected);
    }
}

TEST_F(ScriptBuilderTest, EmitJumpValidatesOpcodeAndEncodesOffsets)
{
    const int8_t offsetI8 = std::numeric_limits<int8_t>::max();
    const int32_t offsetI32 = std::numeric_limits<int32_t>::max();

    for (int op = static_cast<int>(OpCode::JMP); op <= static_cast<int>(OpCode::JMPLE_L); ++op)
    {
        ScriptBuilder builder;
        const OpCode opcode = static_cast<OpCode>(op);

        if (opcode < OpCode::JMP || opcode > OpCode::JMPLE_L)
        {
            EXPECT_THROW(builder.EmitJump(opcode, offsetI8), std::invalid_argument);
            EXPECT_THROW(builder.EmitJump(opcode, offsetI32), std::invalid_argument);
            continue;
        }

        builder.EmitJump(opcode, offsetI8);
        builder.EmitJump(opcode, offsetI32);

        std::vector<uint8_t> expected;
        if (op % 2 == 0)
        {
            expected.push_back(static_cast<uint8_t>(opcode));
            expected.push_back(static_cast<uint8_t>(offsetI8));
            expected.push_back(static_cast<uint8_t>(static_cast<int>(opcode) + 1));
            expected = Concat(expected, LittleEndianBytes<int32_t>(offsetI32));
        }
        else
        {
            expected.push_back(static_cast<uint8_t>(opcode));
            expected = Concat(expected, LittleEndianBytes<int32_t>(static_cast<int32_t>(offsetI8)));
            expected.push_back(static_cast<uint8_t>(opcode));
            expected = Concat(expected, LittleEndianBytes<int32_t>(offsetI32));
        }
        EXPECT_EQ(ToArray(builder), expected);
    }
}

TEST_F(ScriptBuilderTest, EmitJumpThrowsForInvalidOpCodes)
{
    ScriptBuilder builder;
    EXPECT_THROW(builder.EmitJump(OpCode::PUSH0, 0), std::invalid_argument);
    EXPECT_THROW(builder.EmitJump(static_cast<OpCode>(static_cast<int>(OpCode::JMPLE_L) + 1), 0),
                 std::invalid_argument);
}
