#include <gtest/gtest.h>
#include <neo/vm/script.h>
#include <neo/vm/internal/byte_vector.h>
#include <neo/vm/internal/byte_span.h>
#include <sstream>

using namespace neo::vm;
using namespace neo::vm::internal;

TEST(ScriptTest, Constructor)
{
    // Default constructor
    Script script1;
    EXPECT_EQ(script1.GetScript().Size(), 0);
    
    // ByteVector constructor
    ByteVector bytes = ByteVector::Parse("0102030405");
    Script script2(bytes);
    EXPECT_EQ(script2.GetScript(), bytes);
    
    // ByteSpan constructor
    Script script3(bytes.AsSpan());
    EXPECT_EQ(script3.GetScript(), bytes);
}

TEST(ScriptTest, GetLength)
{
    ByteVector bytes = ByteVector::Parse("0102030405");
    Script script(bytes);
    EXPECT_EQ(script.GetLength(), 5);
}

TEST(ScriptTest, GetInstruction)
{
    // Simple instruction
    ByteVector bytes1 = ByteVector::Parse("10"); // PUSH0 (0x10)
    Script script1(bytes1);
    auto instruction1 = script1.GetInstruction(0);
    EXPECT_TRUE(instruction1 != nullptr);
    if (instruction1) {
        EXPECT_EQ(instruction1->opcode, OpCode::PUSH0);
        EXPECT_EQ(instruction1->Operand.Size(), 0);
    }
    
    // Instruction with operand (PUSHINT16)
    ByteVector bytes2 = ByteVector::Parse("010100"); // PUSHINT16 0x0001 (little-endian)
    Script script2(bytes2);
    auto instruction2 = script2.GetInstruction(0);
    EXPECT_TRUE(instruction2 != nullptr);
    if (instruction2) {
        EXPECT_EQ(instruction2->opcode, OpCode::PUSHINT16);
        EXPECT_EQ(instruction2->Operand.Size(), 2);
        EXPECT_EQ(instruction2->Operand[0], 0x01);
        EXPECT_EQ(instruction2->Operand[1], 0x00);
    }
    
    // PUSHDATA1
    ByteVector bytes3 = ByteVector::Parse("0C0401020304"); // PUSHDATA1 0x04 0x01 0x02 0x03 0x04
    Script script3(bytes3);
    auto instruction3 = script3.GetInstruction(0);
    EXPECT_TRUE(instruction3 != nullptr);
    if (instruction3) {
        EXPECT_EQ(instruction3->opcode, OpCode::PUSHDATA1);
        EXPECT_EQ(instruction3->Operand.Size(), 4);
        EXPECT_EQ(instruction3->Operand[0], 0x01);
        EXPECT_EQ(instruction3->Operand[1], 0x02);
        EXPECT_EQ(instruction3->Operand[2], 0x03);
        EXPECT_EQ(instruction3->Operand[3], 0x04);
    }
    
    // PUSHDATA2
    ByteVector bytes4 = ByteVector::Parse("0D040001020304"); // PUSHDATA2 0x0400 0x01 0x02 0x03 0x04
    Script script4(bytes4);
    auto instruction4 = script4.GetInstruction(0);
    EXPECT_TRUE(instruction4 != nullptr);
    if (instruction4) {
        EXPECT_EQ(instruction4->opcode, OpCode::PUSHDATA2);
        EXPECT_EQ(instruction4->Operand.Size(), 4);
        EXPECT_EQ(instruction4->Operand[0], 0x01);
        EXPECT_EQ(instruction4->Operand[1], 0x02);
        EXPECT_EQ(instruction4->Operand[2], 0x03);
        EXPECT_EQ(instruction4->Operand[3], 0x04);
    }
    
    // PUSHDATA4
    ByteVector bytes5 = ByteVector::Parse("0E0400000001020304"); // PUSHDATA4 0x04000000 0x01 0x02 0x03 0x04
    Script script5(bytes5);
    auto instruction5 = script5.GetInstruction(0);
    EXPECT_TRUE(instruction5 != nullptr);
    if (instruction5) {
        EXPECT_EQ(instruction5->opcode, OpCode::PUSHDATA4);
        EXPECT_EQ(instruction5->Operand.Size(), 4);
        EXPECT_EQ(instruction5->Operand[0], 0x01);
        EXPECT_EQ(instruction5->Operand[1], 0x02);
        EXPECT_EQ(instruction5->Operand[2], 0x03);
        EXPECT_EQ(instruction5->Operand[3], 0x04);
    }
    
    // Invalid position
    auto instruction6 = script1.GetInstruction(1);
    EXPECT_TRUE(instruction6 == nullptr);
    
    // Invalid instruction
    ByteVector bytes6 = ByteVector::Parse("01"); // PUSHINT16 (missing 2-byte operand)
    Script script6(bytes6);
    auto instruction7 = script6.GetInstruction(0);
    EXPECT_TRUE(instruction7 == nullptr);
}

TEST(ScriptTest, GetNextInstruction)
{
    // Multiple instructions
    ByteVector bytes = ByteVector::Parse("10000211"); // PUSH0, PUSHINT8 0x02, PUSH1
    Script script(bytes);
    
    int32_t position = 0;
    auto instruction1 = script.GetNextInstruction(position);
    EXPECT_TRUE(instruction1 != nullptr);
    EXPECT_EQ(instruction1->opcode, OpCode::PUSH0);
    EXPECT_EQ(instruction1->Operand.Size(), 0);
    EXPECT_EQ(position, 1);
    
    auto instruction2 = script.GetNextInstruction(position);
    EXPECT_TRUE(instruction2 != nullptr);
    EXPECT_EQ(instruction2->opcode, OpCode::PUSHINT8);
    EXPECT_EQ(instruction2->Operand.Size(), 1);
    EXPECT_EQ(instruction2->Operand[0], 0x02);
    EXPECT_EQ(position, 3);
    
    auto instruction3 = script.GetNextInstruction(position);
    EXPECT_TRUE(instruction3 != nullptr);
    EXPECT_EQ(instruction3->opcode, OpCode::PUSH1);
    EXPECT_EQ(instruction3->Operand.Size(), 0);
    EXPECT_EQ(position, 4);
    
    auto instruction4 = script.GetNextInstruction(position);
    EXPECT_FALSE(instruction4 != nullptr);
}

TEST(ScriptTest, GetJumpDestination)
{
    Script script;
    
    // Positive offset
    EXPECT_EQ(script.GetJumpDestination(10, 5), 15);
    
    // Negative offset
    EXPECT_EQ(script.GetJumpDestination(10, -5), 5);
    
    // Zero offset
    EXPECT_EQ(script.GetJumpDestination(10, 0), 10);
}

// Script doesn't implement ISerializable, so this test is not applicable
// TEST(ScriptTest, Serialization)
// {
//     // Create a script
//     ByteVector bytes = ByteVector::Parse("0102030405");
//     Script script(bytes);
//     
//     // Serialize
//     std::stringstream stream;
//     BinaryWriter writer(stream);
//     script.Serialize(writer);
//     
//     // Deserialize
//     stream.seekg(0);
//     BinaryReader reader(stream);
//     Script script2;
//     script2.Deserialize(reader);
//     
//     // Check
//     EXPECT_EQ(script2.GetScript(), bytes);
// }

TEST(ScriptTest, Equality)
{
    ByteVector bytes1 = ByteVector::Parse("0102030405");
    ByteVector bytes2 = ByteVector::Parse("0102030405");
    ByteVector bytes3 = ByteVector::Parse("0607080910");
    
    Script script1(bytes1);
    Script script2(bytes2);
    Script script3(bytes3);
    
    EXPECT_TRUE(script1 == script2);
    EXPECT_FALSE(script1 == script3);
    
    EXPECT_FALSE(script1 != script2);
    EXPECT_TRUE(script1 != script3);
}

TEST(ScriptTest, GetOperandSize)
{
    // No operand
    EXPECT_EQ(Script::GetOperandSize(OpCode::PUSH0), 0);
    EXPECT_EQ(Script::GetOperandSize(OpCode::PUSH1), 0);
    EXPECT_EQ(Script::GetOperandSize(OpCode::ADD), 0);
    
    // 1-byte operand
    EXPECT_EQ(Script::GetOperandSize(OpCode::JMP), 1);
    EXPECT_EQ(Script::GetOperandSize(OpCode::JMPIF), 1);
    EXPECT_EQ(Script::GetOperandSize(OpCode::CALL), 1);
    
    // 4-byte operand
    EXPECT_EQ(Script::GetOperandSize(OpCode::JMP_L), 4);
    EXPECT_EQ(Script::GetOperandSize(OpCode::JMPIF_L), 4);
    EXPECT_EQ(Script::GetOperandSize(OpCode::CALL_L), 4);
    EXPECT_EQ(Script::GetOperandSize(OpCode::SYSCALL), 4);
    
    // Variable-length operand
    EXPECT_EQ(Script::GetOperandSize(OpCode::PUSHDATA1), 1);
    EXPECT_EQ(Script::GetOperandSize(OpCode::PUSHDATA2), 2);
    EXPECT_EQ(Script::GetOperandSize(OpCode::PUSHDATA4), 4);
}

TEST(ScriptTest, GetPrice)
{
    // Free operations
    EXPECT_EQ(Script::GetPrice(OpCode::NOP), 0);
    
    // Standard operations
    EXPECT_EQ(Script::GetPrice(OpCode::PUSH0), 1);
    EXPECT_EQ(Script::GetPrice(OpCode::PUSH1), 1);
    EXPECT_EQ(Script::GetPrice(OpCode::ADD), 1);
    
    // Push operations
    EXPECT_EQ(Script::GetPrice(OpCode::PUSHDATA1), 1);
    EXPECT_EQ(Script::GetPrice(OpCode::PUSHDATA2), 1);
    EXPECT_EQ(Script::GetPrice(OpCode::PUSHDATA4), 1);
    EXPECT_EQ(Script::GetPrice(OpCode::PUSH1), 1);
    EXPECT_EQ(Script::GetPrice(OpCode::PUSH16), 1);
}

TEST(ScriptTest, GetOpCodeName)
{
    // Check a few opcodes
    EXPECT_EQ(Script::GetOpCodeName(OpCode::PUSH0), "PUSH0");
    EXPECT_EQ(Script::GetOpCodeName(OpCode::PUSH1), "PUSH1");
    EXPECT_EQ(Script::GetOpCodeName(OpCode::ADD), "ADD");
    EXPECT_EQ(Script::GetOpCodeName(OpCode::SUB), "SUB");
    EXPECT_EQ(Script::GetOpCodeName(OpCode::MUL), "MUL");
    EXPECT_EQ(Script::GetOpCodeName(OpCode::DIV), "DIV");
    EXPECT_EQ(Script::GetOpCodeName(OpCode::JMP), "JMP");
    EXPECT_EQ(Script::GetOpCodeName(OpCode::CALL), "CALL");
    EXPECT_EQ(Script::GetOpCodeName(OpCode::RET), "RET");
    EXPECT_EQ(Script::GetOpCodeName(OpCode::SYSCALL), "SYSCALL");
    
    // Unknown opcode
    EXPECT_EQ(Script::GetOpCodeName(static_cast<OpCode>(0xFF)), "CONVERT");
}
