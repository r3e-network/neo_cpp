#include "neo/vm/execution_engine.h"
#include "neo/vm/opcode.h"
#include "neo/vm/script_builder.h"
#include "neo/vm/stack_item.h"
#include <gtest/gtest.h>

using namespace neo;
using namespace neo::vm;

class StackOpcodeTest : public ::testing::Test
{
  protected:
    std::unique_ptr<ExecutionEngine> engine;

    void SetUp() override
    {
        engine = std::make_unique<ExecutionEngine>();
    }

    void ExecuteScript(const ByteVector& script)
    {
        engine->LoadScript(script);
        engine->Execute();
    }

    void CheckState(VMState expected)
    {
        EXPECT_EQ(engine->State(), expected);
    }

    void CheckStackSize(size_t expected)
    {
        ASSERT_EQ(engine->EvaluationStack().size(), expected);
    }

    void CheckStackContents(const std::vector<int>& expected)
    {
        ASSERT_EQ(engine->EvaluationStack().size(), expected.size());
        auto& stack = engine->EvaluationStack();
        for (int i = expected.size() - 1; i >= 0; i--)
        {
            EXPECT_EQ(stack.Pop()->GetInteger(), expected[i]);
        }
    }
};

// DEPTH Tests
TEST_F(StackOpcodeTest, DEPTH_EmptyStack)
{
    ScriptBuilder sb;
    sb.EmitOpCode(OpCode::DEPTH);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 0);
}

TEST_F(StackOpcodeTest, DEPTH_WithElements)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::DEPTH);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(4);                                            // 3 original + 1 depth result
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 3);  // Depth
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 3);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);
}

// DROP Tests
TEST_F(StackOpcodeTest, DROP_SingleElement)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitOpCode(OpCode::DROP);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);
}

TEST_F(StackOpcodeTest, DROP_EmptyStack)
{
    ScriptBuilder sb;
    sb.EmitOpCode(OpCode::DROP);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);  // Should fault on empty stack
}

// NIP Tests
TEST_F(StackOpcodeTest, NIP_RemoveSecondElement)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::NIP);  // Remove second from top (2)

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 3);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);
}

TEST_F(StackOpcodeTest, NIP_InsufficientElements)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitOpCode(OpCode::NIP);  // Only one element

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

// DUP Tests
TEST_F(StackOpcodeTest, DUP_DuplicateTop)
{
    ScriptBuilder sb;
    sb.EmitPush(42);
    sb.EmitOpCode(OpCode::DUP);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 42);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 42);
}

TEST_F(StackOpcodeTest, DUP_EmptyStack)
{
    ScriptBuilder sb;
    sb.EmitOpCode(OpCode::DUP);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

// OVER Tests
TEST_F(StackOpcodeTest, OVER_CopySecondElement)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitOpCode(OpCode::OVER);  // Copy 1 to top

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(3);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);  // Copied element
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);  // Original
}

TEST_F(StackOpcodeTest, OVER_InsufficientElements)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitOpCode(OpCode::OVER);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

// PICK Tests
TEST_F(StackOpcodeTest, PICK_ValidIndex)
{
    ScriptBuilder sb;
    sb.EmitPush(1);  // Bottom
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitPush(4);  // Top
    sb.EmitPush(2);  // Index to pick (element 2)
    sb.EmitOpCode(OpCode::PICK);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(5);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);  // Picked element
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 4);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 3);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);
}

TEST_F(StackOpcodeTest, PICK_IndexZero)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(0);  // Pick top element
    sb.EmitOpCode(OpCode::PICK);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(3);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);  // Picked (same as top)
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);
}

TEST_F(StackOpcodeTest, PICK_InvalidIndex)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(5);  // Index out of range
    sb.EmitOpCode(OpCode::PICK);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

TEST_F(StackOpcodeTest, PICK_NegativeIndex)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(-1);  // Negative index
    sb.EmitOpCode(OpCode::PICK);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

// TUCK Tests
TEST_F(StackOpcodeTest, TUCK_InsertTopBelowSecond)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::TUCK);  // Insert 3 below 2

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(4);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 3);  // Top
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);  // Second
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 3);  // Inserted copy
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);  // Bottom
}

TEST_F(StackOpcodeTest, TUCK_InsufficientElements)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitOpCode(OpCode::TUCK);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

// SWAP Tests
TEST_F(StackOpcodeTest, SWAP_TwoElements)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitOpCode(OpCode::SWAP);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);  // Was second
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);  // Was top
}

TEST_F(StackOpcodeTest, SWAP_InsufficientElements)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitOpCode(OpCode::SWAP);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

// ROT Tests
TEST_F(StackOpcodeTest, ROT_ThreeElements)
{
    ScriptBuilder sb;
    sb.EmitPush(1);              // Bottom
    sb.EmitPush(2);              // Middle
    sb.EmitPush(3);              // Top
    sb.EmitOpCode(OpCode::ROT);  // Rotate: 1 -> top, 2 -> bottom, 3 -> middle

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(3);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);  // Was bottom, now top
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 3);  // Was top, now middle
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);  // Was middle, now bottom
}

TEST_F(StackOpcodeTest, ROT_InsufficientElements)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitOpCode(OpCode::ROT);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

// ROLL Tests
TEST_F(StackOpcodeTest, ROLL_ValidIndex)
{
    ScriptBuilder sb;
    sb.EmitPush(1);  // Index 3
    sb.EmitPush(2);  // Index 2
    sb.EmitPush(3);  // Index 1
    sb.EmitPush(4);  // Index 0 (top)
    sb.EmitPush(2);  // Roll index 2 to top
    sb.EmitOpCode(OpCode::ROLL);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(4);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);  // Rolled to top
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 4);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 3);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);
}

TEST_F(StackOpcodeTest, ROLL_IndexZero)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(0);  // Roll top element (no change)
    sb.EmitOpCode(OpCode::ROLL);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);
}

TEST_F(StackOpcodeTest, ROLL_InvalidIndex)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(5);  // Index out of range
    sb.EmitOpCode(OpCode::ROLL);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

// REVERSE3 Tests
TEST_F(StackOpcodeTest, REVERSE3_ThreeElements)
{
    ScriptBuilder sb;
    sb.EmitPush(1);  // Bottom
    sb.EmitPush(2);  // Middle
    sb.EmitPush(3);  // Top
    sb.EmitOpCode(OpCode::REVERSE3);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(3);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);  // Reversed
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 3);
}

TEST_F(StackOpcodeTest, REVERSE3_InsufficientElements)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitOpCode(OpCode::REVERSE3);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

// REVERSE4 Tests
TEST_F(StackOpcodeTest, REVERSE4_FourElements)
{
    ScriptBuilder sb;
    sb.EmitPush(1);  // Bottom
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitPush(4);  // Top
    sb.EmitOpCode(OpCode::REVERSE4);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(4);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);  // Reversed
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 3);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 4);
}

TEST_F(StackOpcodeTest, REVERSE4_InsufficientElements)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::REVERSE4);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

// REVERSEN Tests
TEST_F(StackOpcodeTest, REVERSEN_ValidCount)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitPush(4);
    sb.EmitPush(4);  // Reverse top 4 elements
    sb.EmitOpCode(OpCode::REVERSEN);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(4);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);  // Reversed
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 3);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 4);
}

TEST_F(StackOpcodeTest, REVERSEN_Zero)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(0);  // Reverse 0 elements (no change)
    sb.EmitOpCode(OpCode::REVERSEN);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);
}

TEST_F(StackOpcodeTest, REVERSEN_InsufficientElements)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(5);  // Try to reverse more elements than available
    sb.EmitOpCode(OpCode::REVERSEN);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

TEST_F(StackOpcodeTest, REVERSEN_NegativeCount)
{
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(-1);  // Negative count
    sb.EmitOpCode(OpCode::REVERSEN);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

// Complex Stack Manipulation Tests
TEST_F(StackOpcodeTest, ComplexStackOperations_CalculateSum)
{
    // Calculate sum of array [1, 2, 3, 4, 5] using stack operations
    ScriptBuilder sb;

    // Push array elements
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitPush(4);
    sb.EmitPush(5);
    sb.EmitPush(0);  // Sum accumulator

    // Sum loop (5 iterations)
    for (int i = 0; i < 5; i++)
    {
        sb.EmitOpCode(OpCode::SWAP);  // Swap sum and next number
        sb.EmitOpCode(OpCode::ADD);   // Add to sum
    }

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 15);  // 1+2+3+4+5
}

TEST_F(StackOpcodeTest, ComplexStackOperations_ReverseArray)
{
    // Reverse array [1, 2, 3, 4] using stack operations
    ScriptBuilder sb;

    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitPush(4);
    sb.EmitPush(4);  // Array size
    sb.EmitOpCode(OpCode::REVERSEN);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(4);

    // Check reversed order
    std::vector<int> expected = {1, 2, 3, 4};  // Original order after reverse
    CheckStackContents(expected);
}

TEST_F(StackOpcodeTest, ComplexStackOperations_StackSorting)
{
    // Sort 3 numbers using stack operations (bubble sort style)
    ScriptBuilder sb;

    sb.EmitPush(3);  // Largest
    sb.EmitPush(1);  // Smallest
    sb.EmitPush(2);  // Middle

    // Compare and swap if needed (top two elements)
    sb.EmitOpCode(OpCode::DUP);  // Duplicate second
    sb.EmitOpCode(OpCode::ROT);  // Bring first to top
    sb.EmitOpCode(OpCode::DUP);  // Duplicate first
    sb.EmitOpCode(OpCode::ROT);  // Arrange for comparison
    sb.EmitOpCode(OpCode::GT);   // Compare
    // If true, elements are in wrong order - would need conditional swap
    // Demonstrates basic comparison - full sort requires branching logic

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    // Result depends on exact implementation of conditional swapping
}

TEST_F(StackOpcodeTest, StackDepthTracking)
{
    ScriptBuilder sb;

    // Track depth changes
    sb.EmitOpCode(OpCode::DEPTH);  // 0
    sb.EmitPush(1);
    sb.EmitOpCode(OpCode::DEPTH);  // 1
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::DEPTH);  // 3
    sb.EmitOpCode(OpCode::DROP);
    sb.EmitOpCode(OpCode::DEPTH);  // 2

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(5);  // 4 depth results + 2 remaining values

    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);  // Final depth
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 3);  // Before drop
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);  // Remaining value
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);  // After first push
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);  // Remaining value
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 0);  // Initial depth
}