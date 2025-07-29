#include <gtest/gtest.h>
#include <neo/vm/execution_context.h>
#include <neo/vm/reference_counter.h>
#include <neo/vm/script.h>
#include <stack>

using namespace neo::vm;

class ExecutionContextTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        script_ = std::make_shared<Script>(std::vector<uint8_t>{});
        ref_counter_ = std::make_shared<ReferenceCounter>();
    }

    std::shared_ptr<Script> script_;
    std::shared_ptr<ReferenceCounter> ref_counter_;
};

TEST_F(ExecutionContextTest, StateManagement)
{
    ExecutionContext context(script_, 0, ref_counter_);

    // Test state management
    struct TestState
    {
        bool flag = false;
    };

    // First get should create the state
    auto& state1 = context.GetState<TestState>();
    state1.flag = true;

    // Second get should return the same instance
    auto& state2 = context.GetState<TestState>();
    EXPECT_TRUE(state2.flag);

    // Test with different type
    auto& stack = context.GetState<std::stack<int>>();
    EXPECT_EQ(0, stack.size());
    stack.push(42);
    EXPECT_EQ(1, stack.size());

    // Test clone
    auto clone = context.Clone();
    auto& clonedStack = clone->GetState<std::stack<int>>();
    EXPECT_EQ(1, clonedStack.size());
    EXPECT_EQ(42, clonedStack.top());

    // Modify clone shouldn't affect original
    clonedStack.pop();
    clonedStack.push(100);
    EXPECT_EQ(1, clone->GetState<std::stack<int>>().size());
    EXPECT_EQ(100, clone->GetState<std::stack<int>>().top());

    // Original should remain unchanged
    EXPECT_EQ(1, context.GetState<std::stack<int>>().size());
    EXPECT_EQ(42, context.GetState<std::stack<int>>().top());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
