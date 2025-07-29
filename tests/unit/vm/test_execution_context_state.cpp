#include <gtest/gtest.h>
#include <neo/io/byte_vector.h>
#include <neo/vm/execution_context.h>
#include <neo/vm/reference_counter.h>
#include <neo/vm/stack_item.h>
#include <stack>

using namespace neo::vm;
using namespace neo::io;

// Test state object similar to the C# TestState class
class TestState
{
  public:
    bool flag = false;
};

// Test ExecutionContext's GetState functionality
TEST(ExecutionContextStateTest, GetStateTest)
{
    ByteVector bytes = ByteVector::Parse("0102030405");
    // Convert io::ByteVector to internal::ByteVector
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i)
    {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    ReferenceCounter refCounter;
    ExecutionContext context(script, -1);

    // Test factory method - creating a new state
    auto state = context.GetState<TestState>(
        [&]()
        {
            auto newState = std::make_shared<TestState>();
            newState->flag = true;
            return newState;
        });

    ASSERT_TRUE(state != nullptr);
    EXPECT_TRUE(state->flag);

    // Modify the state
    state->flag = false;

    // Get the same state again - should return the existing one
    auto state2 = context.GetState<TestState>(
        [&]()
        {
            auto newState = std::make_shared<TestState>();
            newState->flag = true;
            return newState;
        });

    ASSERT_TRUE(state2 != nullptr);
    EXPECT_FALSE(state2->flag);  // Should be false from our previous modification

    // Test with a standard library type
    auto stack = context.GetState<std::stack<int>>([&]() { return std::make_shared<std::stack<int>>(); });

    ASSERT_TRUE(stack != nullptr);
    EXPECT_EQ(stack->size(), 0);

    // Modify the stack
    stack->push(100);

    // Get the same stack again
    auto stack2 = context.GetState<std::stack<int>>([&]() { return std::make_shared<std::stack<int>>(); });

    ASSERT_TRUE(stack2 != nullptr);
    EXPECT_EQ(stack2->size(), 1);
    EXPECT_EQ(stack2->top(), 100);

    // Test Clone
    auto clonedContext = context.Clone();
    auto clonedStack = clonedContext->GetState<std::stack<int>>([&]() { return std::make_shared<std::stack<int>>(); });

    ASSERT_TRUE(clonedStack != nullptr);
    EXPECT_EQ(clonedStack->size(), 1);
    EXPECT_EQ(clonedStack->top(), 100);

    // Modify the cloned stack
    clonedStack->push(200);

    // Check that the original stack was also modified (shared state)
    auto originalStackAfterClone =
        context.GetState<std::stack<int>>([&]() { return std::make_shared<std::stack<int>>(); });

    ASSERT_TRUE(originalStackAfterClone != nullptr);
    EXPECT_EQ(originalStackAfterClone->size(), 2);
    EXPECT_EQ(originalStackAfterClone->top(), 200);
}