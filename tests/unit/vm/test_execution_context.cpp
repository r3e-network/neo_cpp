#include <gtest/gtest.h>
#include <memory>
#include <neo/vm/execution_context.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script.h>
#include <neo/vm/stack_item.h>
#include <typeindex>
#include <vector>

using namespace neo::vm;

/**
 * @brief Test fixture for ExecutionContext
 */
class ExecutionContextTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Create a simple test script
        std::vector<uint8_t> scriptData = {static_cast<uint8_t>(OpCode::PUSH1), static_cast<uint8_t>(OpCode::PUSH2),
                                           static_cast<uint8_t>(OpCode::ADD), static_cast<uint8_t>(OpCode::RET)};
        testScript = std::make_shared<Script>(scriptData);
    }

    std::shared_ptr<Script> testScript;
};

TEST_F(ExecutionContextTest, Constructor)
{
    // Test with default RVCount
    ExecutionContext context(*testScript);
    EXPECT_EQ(&context.GetScript(), testScript.get());
    EXPECT_EQ(0, context.GetInstructionPointer());
    EXPECT_EQ(-1, context.GetRVCount());

    // Test with specific RVCount
    ExecutionContext context2(*testScript, 5);
    EXPECT_EQ(5, context2.GetRVCount());
}

TEST_F(ExecutionContextTest, InstructionPointer)
{
    ExecutionContext context(*testScript);

    // Initial pointer should be 0
    EXPECT_EQ(0, context.GetInstructionPointer());
    EXPECT_EQ(0, context.GetCurrentPosition());

    // Set instruction pointer
    context.SetInstructionPointer(2);
    EXPECT_EQ(2, context.GetInstructionPointer());
    EXPECT_EQ(2, context.GetCurrentPosition());

    // Move next
    context.MoveNext();
    EXPECT_EQ(3, context.GetInstructionPointer());
}

TEST_F(ExecutionContextTest, GetNextInstructionOpCode)
{
    ExecutionContext context(*testScript);

    // Should be at PUSH1
    EXPECT_EQ(OpCode::PUSH1, context.GetNextInstructionOpCode());

    context.MoveNext();
    // Should be at PUSH2
    EXPECT_EQ(OpCode::PUSH2, context.GetNextInstructionOpCode());

    context.MoveNext();
    // Should be at ADD
    EXPECT_EQ(OpCode::ADD, context.GetNextInstructionOpCode());

    context.MoveNext();
    // Should be at RET
    EXPECT_EQ(OpCode::RET, context.GetNextInstructionOpCode());
}

TEST_F(ExecutionContextTest, GetInstructions)
{
    ExecutionContext context(*testScript);

    // Get next instruction object
    auto inst1 = context.GetNextInstructionObject();
    ASSERT_NE(nullptr, inst1);
    EXPECT_EQ(OpCode::PUSH1, inst1->GetOpCode());

    context.MoveNext();

    // Get current instruction
    auto current = context.GetCurrentInstruction();
    ASSERT_NE(nullptr, current);
    EXPECT_EQ(OpCode::PUSH1, current->GetOpCode());

    // Get next instruction
    auto inst2 = context.GetNextInstructionObject();
    ASSERT_NE(nullptr, inst2);
    EXPECT_EQ(OpCode::PUSH2, inst2->GetOpCode());
}

TEST_F(ExecutionContextTest, StaticFields)
{
    ExecutionContext context(*testScript);

    // Initially empty
    EXPECT_EQ(0u, context.GetStaticFields().size());

    // Initialize static fields
    context.InitializeStaticFields(3);
    EXPECT_EQ(3u, context.GetStaticFields().size());

    // Set and get static field
    context.SetStaticField(0, std::make_shared<IntegerStackItem>(100));
    auto field = context.GetStaticField(0);
    ASSERT_NE(nullptr, field);
    EXPECT_EQ(100, std::dynamic_pointer_cast<IntegerStackItem>(field)->GetValue());

    // Out of bounds should return null
    EXPECT_EQ(nullptr, context.GetStaticField(10));
}

TEST_F(ExecutionContextTest, LocalVariables)
{
    ExecutionContext context(*testScript);

    // Initially empty
    EXPECT_EQ(0u, context.GetLocalVariables().size());

    // Initialize local variables
    context.InitializeLocalVariables(2);
    EXPECT_EQ(2u, context.GetLocalVariables().size());

    // Set and get local variable
    context.SetLocalVariable(0, std::make_shared<IntegerStackItem>(42));
    auto local = context.GetLocalVariable(0);
    ASSERT_NE(nullptr, local);
    EXPECT_EQ(42, std::dynamic_pointer_cast<IntegerStackItem>(local)->GetValue());
}

TEST_F(ExecutionContextTest, Arguments)
{
    ExecutionContext context(*testScript);

    // Initially empty
    EXPECT_EQ(0u, context.GetArguments().size());

    // Initialize arguments
    context.InitializeArguments(2);
    EXPECT_EQ(2u, context.GetArguments().size());

    // Set and get argument
    context.SetArgument(1, std::make_shared<BooleanStackItem>(true));
    auto arg = context.GetArgument(1);
    ASSERT_NE(nullptr, arg);
    EXPECT_TRUE(std::dynamic_pointer_cast<BooleanStackItem>(arg)->GetValue());
}

TEST_F(ExecutionContextTest, EvaluationStack)
{
    ExecutionContext context(*testScript);

    // Initially empty
    EXPECT_EQ(0u, context.GetEvaluationStack().size());

    // Push items
    context.PushToEvaluationStack(std::make_shared<IntegerStackItem>(10));
    context.PushToEvaluationStack(std::make_shared<IntegerStackItem>(20));

    EXPECT_EQ(2u, context.GetEvaluationStack().size());

    // Pop items
    auto item1 = context.PopFromEvaluationStack();
    ASSERT_NE(nullptr, item1);
    EXPECT_EQ(20, std::dynamic_pointer_cast<IntegerStackItem>(item1)->GetValue());

    auto item2 = context.PopFromEvaluationStack();
    ASSERT_NE(nullptr, item2);
    EXPECT_EQ(10, std::dynamic_pointer_cast<IntegerStackItem>(item2)->GetValue());

    EXPECT_EQ(0u, context.GetEvaluationStack().size());
}

TEST_F(ExecutionContextTest, PeekEvaluationStack)
{
    ExecutionContext context(*testScript);

    // Push items
    context.PushToEvaluationStack(std::make_shared<IntegerStackItem>(1));
    context.PushToEvaluationStack(std::make_shared<IntegerStackItem>(2));
    context.PushToEvaluationStack(std::make_shared<IntegerStackItem>(3));

    // Peek at different positions
    auto top = context.PeekEvaluationStack(0);
    ASSERT_NE(nullptr, top);
    EXPECT_EQ(3, std::dynamic_pointer_cast<IntegerStackItem>(top)->GetValue());

    auto second = context.PeekEvaluationStack(1);
    ASSERT_NE(nullptr, second);
    EXPECT_EQ(2, std::dynamic_pointer_cast<IntegerStackItem>(second)->GetValue());

    auto third = context.PeekEvaluationStack(2);
    ASSERT_NE(nullptr, third);
    EXPECT_EQ(1, std::dynamic_pointer_cast<IntegerStackItem>(third)->GetValue());

    // Stack should still have 3 items
    EXPECT_EQ(3u, context.GetEvaluationStack().size());
}

TEST_F(ExecutionContextTest, State)
{
    ExecutionContext context(*testScript);

    // Define a test state class
    class TestState
    {
      public:
        int value = 0;
        TestState() = default;
        TestState(int v) : value(v) {}
    };

    // Get state with factory
    auto state1 = context.GetState<TestState>([]() { return std::make_shared<TestState>(42); });
    ASSERT_NE(nullptr, state1);
    EXPECT_EQ(42, state1->value);

    // Get same state again - should return existing
    auto state2 = context.GetState<TestState>([]() { return std::make_shared<TestState>(100); });
    ASSERT_NE(nullptr, state2);
    EXPECT_EQ(42, state2->value);  // Should still be 42, not 100
    EXPECT_EQ(state1, state2);     // Should be the same object

    // Get state with default constructor
    class DefaultState
    {
      public:
        std::string name = "default";
    };

    auto state3 = context.GetState<DefaultState>();
    ASSERT_NE(nullptr, state3);
    EXPECT_EQ("default", state3->name);
}

TEST_F(ExecutionContextTest, TryCount)
{
    ExecutionContext context(*testScript);

    // Initially should be 0
    EXPECT_EQ(0, context.GetTryCount());

    // Increment try count
    context.IncrementTryCount();
    EXPECT_EQ(1, context.GetTryCount());

    context.IncrementTryCount();
    EXPECT_EQ(2, context.GetTryCount());

    // Decrement try count
    context.DecrementTryCount();
    EXPECT_EQ(1, context.GetTryCount());
}

TEST_F(ExecutionContextTest, EmptyScript)
{
    Script emptyScript(std::vector<uint8_t>{});
    ExecutionContext context(emptyScript);

    EXPECT_EQ(0, context.GetInstructionPointer());
    EXPECT_THROW(context.GetNextInstructionOpCode(), std::runtime_error);
    EXPECT_EQ(nullptr, context.GetNextInstructionObject());
}

TEST_F(ExecutionContextTest, BoundaryConditions)
{
    ExecutionContext context(*testScript);

    // Test instruction pointer at end
    context.SetInstructionPointer(testScript->GetLength() - 1);
    context.MoveNext();
    EXPECT_EQ(testScript->GetLength(), context.GetInstructionPointer());

    // Try to read past end
    EXPECT_THROW(context.GetNextInstructionOpCode(), std::runtime_error);

    // Pop from empty evaluation stack
    EXPECT_THROW(context.PopFromEvaluationStack(), std::runtime_error);

    // Peek empty evaluation stack
    EXPECT_THROW(context.PeekEvaluationStack(0), std::runtime_error);
}
