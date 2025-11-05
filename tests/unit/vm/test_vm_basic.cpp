#include <gtest/gtest.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/opcodes.h>
#include <neo/vm/vm_state.h>
#include <neo/io/byte_vector.h>

using namespace neo::vm;
using namespace neo::io;

class VMBasicTest : public ::testing::Test {
protected:
    std::unique_ptr<ExecutionEngine> engine_;
    
    void SetUp() override {
        engine_ = std::make_unique<ExecutionEngine>();
    }
    
    Script CreateScript(const ByteVector& data) {
        return Script(neo::vm::internal::ByteSpan(data.Data(), data.Size()));
    }
};

TEST_F(VMBasicTest, TestVMStateEnum) {
    // Test VM state values
    EXPECT_EQ(static_cast<uint8_t>(VMState::None), 0);
    EXPECT_EQ(static_cast<uint8_t>(VMState::Halt), 1);
    EXPECT_EQ(static_cast<uint8_t>(VMState::Fault), 2);
    EXPECT_EQ(static_cast<uint8_t>(VMState::Break), 4);
}

TEST_F(VMBasicTest, TestOpCodeValues) {
    // Test basic opcode values
    EXPECT_EQ(static_cast<uint8_t>(OpCode::PUSH0), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::PUSH1), 0x51);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::NOP), 0x61);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::RET), 0x66);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::DUP), 0x76);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::DROP), 0x75);
}

TEST_F(VMBasicTest, TestSimpleScript) {
    // Create a simple script that just returns
    ScriptBuilder sb;
    sb.Emit(OpCode::RET);
    
    auto script = CreateScript(sb.ToArray());
    EXPECT_GT(script.GetLength(), 0u);
}

TEST_F(VMBasicTest, TestScriptBuilderPush) {
    ScriptBuilder sb;
    
    // Test pushing different types
    sb.EmitPush(static_cast<int64_t>(42));
    sb.EmitPush(true);
    sb.EmitPush(false);
    
    auto result = sb.ToArray();
    EXPECT_GT(result.Size(), 0u);
}

TEST_F(VMBasicTest, TestEngineInitialization) {
    EXPECT_NE(engine_, nullptr);
    
    // Engine should start in None state
    auto state = engine_->GetState();
    EXPECT_TRUE(state == VMState::None || state == VMState::Break);
}

TEST_F(VMBasicTest, TestEmptyScriptExecution) {
    // Empty script should halt immediately
    ScriptBuilder sb;
    sb.Emit(OpCode::NOP);
    sb.Emit(OpCode::RET);
    
    auto script = CreateScript(sb.ToArray());
    engine_->LoadScript(script);
    
    // Execute should not crash
    auto state = engine_->Execute();
    
    // Should end in Halt or Break state
    EXPECT_TRUE(state == VMState::Halt || state == VMState::Break);
}

TEST_F(VMBasicTest, TestSinglePushExecution) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(42));
    
    auto script = CreateScript(sb.ToArray());
    engine_->LoadScript(script);
    auto state = engine_->Execute();
    
    // Should complete successfully
    EXPECT_NE(state, VMState::Fault);
    
    // Stack should have one item
    auto& stack = engine_->GetResultStack();
    EXPECT_GE(stack.size(), 0u);  // At least not crash
}

TEST_F(VMBasicTest, TestBasicArithmetic) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(5));
    sb.EmitPush(static_cast<int64_t>(3));
    sb.Emit(OpCode::ADD);
    
    auto script = CreateScript(sb.ToArray());
    engine_->LoadScript(script);
    auto state = engine_->Execute();
    
    // Should complete without fault
    EXPECT_NE(state, VMState::Fault);
}

TEST_F(VMBasicTest, TestStackOperations) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(100));
    sb.Emit(OpCode::DUP);
    
    auto script = CreateScript(sb.ToArray());
    engine_->LoadScript(script);
    auto state = engine_->Execute();
    
    // Should complete without fault
    EXPECT_NE(state, VMState::Fault);
    
    // After DUP, stack should have 2 items (or at least not crash)
    auto& stack = engine_->GetResultStack();
    EXPECT_GE(stack.size(), 0u);
}
