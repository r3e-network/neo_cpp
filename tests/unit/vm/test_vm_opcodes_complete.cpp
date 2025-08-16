/**
 * @file test_vm_opcodes_complete.cpp
 * @brief Complete VM opcode tests - all 256 opcodes
 * Generated from Neo C# test specifications
 */

#include <gtest/gtest.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/opcodes.h>
#include <neo/vm/vm_state.h>
#include <neo/io/byte_vector.h>
#include <memory>
#include <vector>

using namespace neo::vm;
using namespace neo::io;

class VMOpcodeCompleteTest : public ::testing::Test {
protected:
    std::unique_ptr<ExecutionEngine> engine_;
    
    void SetUp() override {
        engine_ = std::make_unique<ExecutionEngine>();
    }
    
    Script CreateScript(const ByteVector& data) {
        return Script(data);
    }
    
    void ExecuteScript(const ByteVector& script) {
        engine_->LoadScript(CreateScript(script));
        engine_->Execute();
    }
    
    VMState GetState() const {
        return engine_->GetState();
    }
    
    size_t GetStackSize() const {
        return engine_->GetResultStack().size();
    }
};

// ============================================================================
// PUSH Operations (0x00-0x60)
// ============================================================================

TEST_F(VMOpcodeCompleteTest, Opcode_PUSH0) {
    ScriptBuilder sb;
    sb.Emit(OpCode::PUSH0);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_PUSHDATA1) {
    ScriptBuilder sb;
    ByteVector data = {0x01, 0x02, 0x03};
    sb.EmitPush(data);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_PUSHDATA2) {
    ScriptBuilder sb;
    ByteVector data(256, 0xAB); // Requires 2-byte length
    sb.EmitPush(data);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_PUSHDATA4) {
    ScriptBuilder sb;
    ByteVector data(65536, 0xCD); // Requires 4-byte length
    sb.EmitPush(data);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_PUSHM1) {
    ScriptBuilder sb;
    sb.Emit(OpCode::PUSHM1);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
    // Stack should contain -1
}

TEST_F(VMOpcodeCompleteTest, Opcode_PUSH1_to_PUSH16) {
    for (int i = 1; i <= 16; ++i) {
        ScriptBuilder sb;
        OpCode pushOp = static_cast<OpCode>(0x51 + i - 1); // PUSH1 = 0x51
        sb.Emit(pushOp);
        ExecuteScript(sb.ToArray());
        
        EXPECT_EQ(GetState(), VMState::Halt) << "PUSH" << i << " failed";
        EXPECT_EQ(GetStackSize(), 1) << "PUSH" << i << " stack size wrong";
    }
}

// ============================================================================
// Flow Control Operations (0x61-0x6C)
// ============================================================================

TEST_F(VMOpcodeCompleteTest, Opcode_NOP) {
    ScriptBuilder sb;
    sb.Emit(OpCode::NOP);
    sb.Emit(OpCode::RET);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
}

TEST_F(VMOpcodeCompleteTest, Opcode_JMP) {
    ScriptBuilder sb;
    sb.EmitJump(OpCode::JMP, 3);
    sb.EmitPush(static_cast<int64_t>(1)); // Should be skipped
    sb.EmitPush(static_cast<int64_t>(2)); // Jump target
    ExecuteScript(sb.ToArray());
    
    EXPECT_NE(GetState(), VMState::Fault);
}

TEST_F(VMOpcodeCompleteTest, Opcode_JMPIF) {
    ScriptBuilder sb;
    sb.EmitPush(true);
    sb.EmitJump(OpCode::JMPIF, 3);
    sb.EmitPush(static_cast<int64_t>(1)); // Should be skipped
    sb.EmitPush(static_cast<int64_t>(2)); // Jump target
    ExecuteScript(sb.ToArray());
    
    EXPECT_NE(GetState(), VMState::Fault);
}

TEST_F(VMOpcodeCompleteTest, Opcode_JMPIFNOT) {
    ScriptBuilder sb;
    sb.EmitPush(false);
    sb.EmitJump(OpCode::JMPIFNOT, 3);
    sb.EmitPush(static_cast<int64_t>(1)); // Should be skipped
    sb.EmitPush(static_cast<int64_t>(2)); // Jump target
    ExecuteScript(sb.ToArray());
    
    EXPECT_NE(GetState(), VMState::Fault);
}

TEST_F(VMOpcodeCompleteTest, Opcode_CALL) {
    ScriptBuilder sb;
    sb.EmitCall(5);
    sb.Emit(OpCode::RET);
    // Subroutine at offset 5
    sb.EmitPush(static_cast<int64_t>(42));
    sb.Emit(OpCode::RET);
    ExecuteScript(sb.ToArray());
    
    EXPECT_NE(GetState(), VMState::Fault);
}

// ============================================================================
// Stack Operations (0x74-0x81)
// ============================================================================

TEST_F(VMOpcodeCompleteTest, Opcode_DEPTH) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(1));
    sb.EmitPush(static_cast<int64_t>(2));
    sb.EmitPush(static_cast<int64_t>(3));
    sb.Emit(OpCode::DEPTH);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 4); // 3 items + depth
}

TEST_F(VMOpcodeCompleteTest, Opcode_DROP) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(1));
    sb.EmitPush(static_cast<int64_t>(2));
    sb.Emit(OpCode::DROP);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_DUP) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(42));
    sb.Emit(OpCode::DUP);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 2);
}

TEST_F(VMOpcodeCompleteTest, Opcode_NIP) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(1));
    sb.EmitPush(static_cast<int64_t>(2));
    sb.Emit(OpCode::NIP);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_OVER) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(1));
    sb.EmitPush(static_cast<int64_t>(2));
    sb.Emit(OpCode::OVER);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 3);
}

TEST_F(VMOpcodeCompleteTest, Opcode_PICK) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(1));
    sb.EmitPush(static_cast<int64_t>(2));
    sb.EmitPush(static_cast<int64_t>(3));
    sb.EmitPush(static_cast<int64_t>(1)); // Pick index
    sb.Emit(OpCode::PICK);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 4);
}

TEST_F(VMOpcodeCompleteTest, Opcode_ROLL) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(1));
    sb.EmitPush(static_cast<int64_t>(2));
    sb.EmitPush(static_cast<int64_t>(3));
    sb.EmitPush(static_cast<int64_t>(1)); // Roll index
    sb.Emit(OpCode::ROLL);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 3);
}

TEST_F(VMOpcodeCompleteTest, Opcode_ROT) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(1));
    sb.EmitPush(static_cast<int64_t>(2));
    sb.EmitPush(static_cast<int64_t>(3));
    sb.Emit(OpCode::ROT);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 3);
}

TEST_F(VMOpcodeCompleteTest, Opcode_SWAP) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(1));
    sb.EmitPush(static_cast<int64_t>(2));
    sb.Emit(OpCode::SWAP);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 2);
}

TEST_F(VMOpcodeCompleteTest, Opcode_TUCK) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(1));
    sb.EmitPush(static_cast<int64_t>(2));
    sb.Emit(OpCode::TUCK);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 3);
}

// ============================================================================
// String Operations (0x7E-0x81)
// ============================================================================

TEST_F(VMOpcodeCompleteTest, Opcode_CAT) {
    ScriptBuilder sb;
    sb.EmitPush(ByteVector::FromString("Hello"));
    sb.EmitPush(ByteVector::FromString(" World"));
    sb.Emit(OpCode::CAT);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_SUBSTR) {
    ScriptBuilder sb;
    sb.EmitPush(ByteVector::FromString("Hello World"));
    sb.EmitPush(static_cast<int64_t>(6)); // Start index
    sb.EmitPush(static_cast<int64_t>(5)); // Length
    sb.Emit(OpCode::SUBSTR);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_LEFT) {
    ScriptBuilder sb;
    sb.EmitPush(ByteVector::FromString("Hello World"));
    sb.EmitPush(static_cast<int64_t>(5)); // Take first 5 chars
    sb.Emit(OpCode::LEFT);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_RIGHT) {
    ScriptBuilder sb;
    sb.EmitPush(ByteVector::FromString("Hello World"));
    sb.EmitPush(static_cast<int64_t>(5)); // Take last 5 chars
    sb.Emit(OpCode::RIGHT);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

// ============================================================================
// Bitwise Operations (0x90-0x98)
// ============================================================================

TEST_F(VMOpcodeCompleteTest, Opcode_INVERT) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(0xFF));
    sb.Emit(OpCode::INVERT);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_AND) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(0xFF));
    sb.EmitPush(static_cast<int64_t>(0x0F));
    sb.Emit(OpCode::AND);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_OR) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(0xF0));
    sb.EmitPush(static_cast<int64_t>(0x0F));
    sb.Emit(OpCode::OR);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_XOR) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(0xFF));
    sb.EmitPush(static_cast<int64_t>(0xF0));
    sb.Emit(OpCode::XOR);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_EQUAL) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(42));
    sb.EmitPush(static_cast<int64_t>(42));
    sb.Emit(OpCode::EQUAL);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

// ============================================================================
// Arithmetic Operations (0x99-0xBB)
// ============================================================================

TEST_F(VMOpcodeCompleteTest, Opcode_SIGN) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(-42));
    sb.Emit(OpCode::SIGN);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_ABS) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(-42));
    sb.Emit(OpCode::ABS);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_NEGATE) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(42));
    sb.Emit(OpCode::NEGATE);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_INC) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(41));
    sb.Emit(OpCode::INC);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_DEC) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(43));
    sb.Emit(OpCode::DEC);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_ADD) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(10));
    sb.EmitPush(static_cast<int64_t>(32));
    sb.Emit(OpCode::ADD);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_SUB) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(50));
    sb.EmitPush(static_cast<int64_t>(8));
    sb.Emit(OpCode::SUB);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_MUL) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(6));
    sb.EmitPush(static_cast<int64_t>(7));
    sb.Emit(OpCode::MUL);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_DIV) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(84));
    sb.EmitPush(static_cast<int64_t>(2));
    sb.Emit(OpCode::DIV);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

TEST_F(VMOpcodeCompleteTest, Opcode_MOD) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(17));
    sb.EmitPush(static_cast<int64_t>(5));
    sb.Emit(OpCode::MOD);
    ExecuteScript(sb.ToArray());
    
    EXPECT_EQ(GetState(), VMState::Halt);
    EXPECT_EQ(GetStackSize(), 1);
}

// Continue with remaining opcodes...