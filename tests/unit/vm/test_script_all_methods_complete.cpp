#include "neo/vm/bad_script_exception.h"
#include "neo/vm/instruction.h"
#include "neo/vm/opcode.h"
#include "neo/vm/script.h"
#include "neo/vm/script_builder.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace neo;
using namespace neo::vm;

// Complete conversion of C# UT_Script.cs - ALL 3 test methods
class ScriptAllMethodsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Setup code if needed
    }

    void TearDown() override
    {
        // Cleanup code if needed
    }
};

// C# Test Method: TestConversion()
TEST_F(ScriptAllMethodsTest, TestConversion)
{
    std::vector<uint8_t> raw_script;

    {
        ScriptBuilder builder;
        builder.Emit(OpCode::PUSH0);
        builder.Emit(OpCode::CALL, std::vector<uint8_t>{0x00, 0x01});
        builder.EmitSysCall(123);

        raw_script = builder.ToArray();
    }

    auto script = std::make_shared<Script>(raw_script);

    // Test implicit conversion to span/memory
    auto script_conversion = script->GetSpan();
    std::vector<uint8_t> result(script_conversion.begin(), script_conversion.end());

    EXPECT_EQ(raw_script, result);
}

// C# Test Method: TestStrictMode()
TEST_F(ScriptAllMethodsTest, TestStrictMode)
{
    // Test with invalid script in strict mode
    {
        std::vector<uint8_t> raw_script = {static_cast<uint8_t>(OpCode::PUSH0), 0xFF};
        EXPECT_THROW(auto script = std::make_shared<Script>(raw_script, true), BadScriptException);

        // Should work in non-strict mode
        auto script = std::make_shared<Script>(raw_script, false);
        EXPECT_EQ(2, script->Length());
    }

    // Test PUSHDATA1 without enough data in strict mode
    {
        std::vector<uint8_t> raw_script = {static_cast<uint8_t>(OpCode::PUSHDATA1)};
        EXPECT_THROW(auto script = std::make_shared<Script>(raw_script, true), BadScriptException);
    }

    // Test PUSHDATA2 without enough data in strict mode
    {
        std::vector<uint8_t> raw_script = {static_cast<uint8_t>(OpCode::PUSHDATA2)};
        EXPECT_THROW(auto script = std::make_shared<Script>(raw_script, true), BadScriptException);
    }

    // Test PUSHDATA4 without enough data in strict mode
    {
        std::vector<uint8_t> raw_script = {static_cast<uint8_t>(OpCode::PUSHDATA4)};
        EXPECT_THROW(auto script = std::make_shared<Script>(raw_script, true), BadScriptException);
    }
}

// C# Test Method: TestParse()
TEST_F(ScriptAllMethodsTest, TestParse)
{
    std::shared_ptr<Script> script;

    {
        ScriptBuilder builder;
        builder.Emit(OpCode::PUSH0);
        builder.Emit(OpCode::CALL_L, std::vector<uint8_t>{0x00, 0x01, 0x00, 0x00});
        builder.EmitSysCall(123);

        script = std::make_shared<Script>(builder.ToArray());
    }

    EXPECT_EQ(11, script->Length());

    // Test first instruction (PUSH0)
    {
        auto ins = script->GetInstruction(0);

        EXPECT_EQ(OpCode::PUSH0, ins.OpCode());
        EXPECT_TRUE(ins.Operand().empty());
        EXPECT_EQ(1, ins.Size());

        // Should throw when accessing tokens that don't exist for this instruction
        EXPECT_THROW(auto x = ins.TokenI16(), std::out_of_range);
        EXPECT_THROW(auto x = ins.TokenU32(), std::out_of_range);
    }

    // Test second instruction (CALL_L)
    {
        auto ins = script->GetInstruction(1);

        EXPECT_EQ(OpCode::CALL_L, ins.OpCode());

        std::vector<uint8_t> expected_operand = {0x00, 0x01, 0x00, 0x00};
        auto operand = ins.Operand();
        std::vector<uint8_t> actual_operand(operand.begin(), operand.end());
        EXPECT_EQ(expected_operand, actual_operand);

        EXPECT_EQ(5, ins.Size());
        EXPECT_EQ(256, ins.TokenI32());

        // Test token string conversion
        std::string expected_token_string(reinterpret_cast<const char*>(expected_operand.data()),
                                          expected_operand.size());
        EXPECT_EQ(expected_token_string, ins.TokenString());
    }

    // Test third instruction (SYSCALL)
    {
        auto ins = script->GetInstruction(6);

        EXPECT_EQ(OpCode::SYSCALL, ins.OpCode());

        std::vector<uint8_t> expected_operand = {123, 0x00, 0x00, 0x00};
        auto operand = ins.Operand();
        std::vector<uint8_t> actual_operand(operand.begin(), operand.end());
        EXPECT_EQ(expected_operand, actual_operand);

        EXPECT_EQ(5, ins.Size());
        EXPECT_EQ(123, ins.TokenI16());

        // Test token string conversion
        std::string expected_token_string(reinterpret_cast<const char*>(expected_operand.data()),
                                          expected_operand.size());
        EXPECT_EQ(expected_token_string, ins.TokenString());

        EXPECT_EQ(123U, ins.TokenU32());
    }

    // Test out of range instruction access
    EXPECT_THROW(auto ins = script->GetInstruction(100), std::out_of_range);
}

// Additional comprehensive tests for complete coverage

// Test Method: TestScriptConstruction()
TEST_F(ScriptAllMethodsTest, TestScriptConstruction)
{
    // Test empty script
    {
        std::vector<uint8_t> empty_script;
        auto script = std::make_shared<Script>(empty_script);
        EXPECT_EQ(0, script->Length());
    }

    // Test script with single instruction
    {
        std::vector<uint8_t> single_instruction = {static_cast<uint8_t>(OpCode::RET)};
        auto script = std::make_shared<Script>(single_instruction);
        EXPECT_EQ(1, script->Length());

        auto ins = script->GetInstruction(0);
        EXPECT_EQ(OpCode::RET, ins.OpCode());
    }

    // Test script copy constructor behavior
    {
        std::vector<uint8_t> original_data = {static_cast<uint8_t>(OpCode::PUSH1), static_cast<uint8_t>(OpCode::PUSH2)};
        auto script1 = std::make_shared<Script>(original_data);
        auto script2 = std::make_shared<Script>(*script1);

        EXPECT_EQ(script1->Length(), script2->Length());
        EXPECT_EQ(script1->GetSpan().size(), script2->GetSpan().size());
    }
}

// Test Method: TestScriptInstructionParsing()
TEST_F(ScriptAllMethodsTest, TestScriptInstructionParsing)
{
    ScriptBuilder builder;

    // Build a complex script with various instruction types
    builder.Emit(OpCode::PUSH1);                                                 // Simple opcode
    builder.EmitPush(42);                                                        // Push with data
    builder.Emit(OpCode::JMP, std::vector<uint8_t>{0x05});                       // Jump with 1-byte operand
    builder.Emit(OpCode::CALL_L, std::vector<uint8_t>{0x10, 0x00, 0x00, 0x00});  // Call with 4-byte operand
    builder.EmitSysCall(0x627D5B52);                                             // SYSCALL with 4-byte operand
    builder.Emit(OpCode::RET);                                                   // Simple terminator

    auto script = std::make_shared<Script>(builder.ToArray());

    int position = 0;

    // Test PUSH1
    {
        auto ins = script->GetInstruction(position);
        EXPECT_EQ(OpCode::PUSH1, ins.OpCode());
        EXPECT_EQ(1, ins.Size());
        position += ins.Size();
    }

    // Test PUSH with data (actual opcode depends on value 42)
    {
        auto ins = script->GetInstruction(position);
        EXPECT_GT(ins.Size(), 1);  // Should have operand data
        position += ins.Size();
    }

    // Test JMP
    {
        auto ins = script->GetInstruction(position);
        EXPECT_EQ(OpCode::JMP, ins.OpCode());
        EXPECT_EQ(2, ins.Size());  // 1 byte opcode + 1 byte operand
        position += ins.Size();
    }

    // Test CALL_L
    {
        auto ins = script->GetInstruction(position);
        EXPECT_EQ(OpCode::CALL_L, ins.OpCode());
        EXPECT_EQ(5, ins.Size());  // 1 byte opcode + 4 byte operand
        position += ins.Size();
    }

    // Test SYSCALL
    {
        auto ins = script->GetInstruction(position);
        EXPECT_EQ(OpCode::SYSCALL, ins.OpCode());
        EXPECT_EQ(5, ins.Size());  // 1 byte opcode + 4 byte operand
        position += ins.Size();
    }

    // Test RET
    {
        auto ins = script->GetInstruction(position);
        EXPECT_EQ(OpCode::RET, ins.OpCode());
        EXPECT_EQ(1, ins.Size());
    }
}

// Test Method: TestScriptValidation()
TEST_F(ScriptAllMethodsTest, TestScriptValidation)
{
    // Test valid scripts
    {
        ScriptBuilder builder;
        builder.Emit(OpCode::PUSH0);
        builder.Emit(OpCode::PUSH1);
        builder.Emit(OpCode::ADD);
        builder.Emit(OpCode::RET);

        EXPECT_NO_THROW(auto script = std::make_shared<Script>(builder.ToArray(), true));
    }

    // Test invalid PUSHDATA operations in strict mode
    {
        // PUSHDATA1 with insufficient data
        std::vector<uint8_t> invalid_script = {
            static_cast<uint8_t>(OpCode::PUSHDATA1),
            0x05,       // Claims 5 bytes of data
            0x01, 0x02  // But only provides 2 bytes
        };
        EXPECT_THROW(auto script = std::make_shared<Script>(invalid_script, true), BadScriptException);
    }

    {
        // PUSHDATA2 with insufficient data
        std::vector<uint8_t> invalid_script = {
            static_cast<uint8_t>(OpCode::PUSHDATA2), 0x05, 0x00,  // Claims 5 bytes of data
            0x01, 0x02                                            // But only provides 2 bytes
        };
        EXPECT_THROW(auto script = std::make_shared<Script>(invalid_script, true), BadScriptException);
    }

    {
        // PUSHDATA4 with insufficient data
        std::vector<uint8_t> invalid_script = {
            static_cast<uint8_t>(OpCode::PUSHDATA4),
            0x05,
            0x00,
            0x00,
            0x00,  // Claims 5 bytes of data
            0x01,
            0x02  // But only provides 2 bytes
        };
        EXPECT_THROW(auto script = std::make_shared<Script>(invalid_script, true), BadScriptException);
    }
}

// Test Method: TestScriptIteration()
TEST_F(ScriptAllMethodsTest, TestScriptIteration)
{
    ScriptBuilder builder;
    builder.Emit(OpCode::PUSH0);
    builder.Emit(OpCode::PUSH1);
    builder.Emit(OpCode::PUSH2);
    builder.Emit(OpCode::ADD);
    builder.Emit(OpCode::ADD);
    builder.Emit(OpCode::RET);

    auto script = std::make_shared<Script>(builder.ToArray());

    std::vector<OpCode> expected_opcodes = {OpCode::PUSH0, OpCode::PUSH1, OpCode::PUSH2,
                                            OpCode::ADD,   OpCode::ADD,   OpCode::RET};

    // Iterate through all instructions
    std::vector<OpCode> actual_opcodes;
    int position = 0;

    while (position < script->Length())
    {
        auto ins = script->GetInstruction(position);
        actual_opcodes.push_back(ins.OpCode());
        position += ins.Size();
    }

    EXPECT_EQ(expected_opcodes, actual_opcodes);
}

// Test Method: TestScriptOperandExtraction()
TEST_F(ScriptAllMethodsTest, TestScriptOperandExtraction)
{
    ScriptBuilder builder;

    // Test different operand sizes
    builder.EmitPush(0x12);                                                     // 1-byte push
    builder.EmitPush(0x1234);                                                   // 2-byte push
    builder.EmitPush(0x12345678);                                               // 4-byte push
    builder.Emit(OpCode::JMP_L, std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04});  // 4-byte operand

    auto script = std::make_shared<Script>(builder.ToArray());

    int position = 0;

    // Test 1-byte push operand
    {
        auto ins = script->GetInstruction(position);
        auto operand = ins.Operand();
        EXPECT_FALSE(operand.empty());
        EXPECT_EQ(0x12, operand[0]);
        position += ins.Size();
    }

    // Test 2-byte push operand
    {
        auto ins = script->GetInstruction(position);
        auto operand = ins.Operand();
        EXPECT_GE(operand.size(), 2);
        position += ins.Size();
    }

    // Test 4-byte push operand
    {
        auto ins = script->GetInstruction(position);
        auto operand = ins.Operand();
        EXPECT_GE(operand.size(), 4);
        position += ins.Size();
    }

    // Test JMP_L operand
    {
        auto ins = script->GetInstruction(position);
        EXPECT_EQ(OpCode::JMP_L, ins.OpCode());
        auto operand = ins.Operand();
        EXPECT_EQ(4, operand.size());
        EXPECT_EQ(0x01, operand[0]);
        EXPECT_EQ(0x02, operand[1]);
        EXPECT_EQ(0x03, operand[2]);
        EXPECT_EQ(0x04, operand[3]);
    }
}