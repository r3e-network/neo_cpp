#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "neo/vm/execution_context.h"
#include "neo/vm/reference_counter.h"
#include <memory>
#include <vector>
#include <stack>

using namespace neo;
using namespace neo::vm;

// Test state class equivalent to C# TestState
class TestState {
public:
    bool Flag = false;
    
    TestState() = default;
    explicit TestState(bool flag) : Flag(flag) {}
};

// Complete conversion of C# UT_ExecutionContext.cs - ALL 1 test method
class ExecutionContextAllMethodsTest : public ::testing::Test {
protected:
    void SetUp() override {
        reference_counter_ = std::make_shared<ReferenceCounter>();
    }
    
    void TearDown() override {
        reference_counter_.reset();
    }
    
    std::shared_ptr<ReferenceCounter> reference_counter_;
};

// C# Test Method: TestStateTest()
TEST_F(ExecutionContextAllMethodsTest, TestStateTest) {
    std::vector<uint8_t> empty_script;
    auto context = std::make_shared<ExecutionContext>(empty_script, -1, reference_counter_.get());
    
    // Test factory equivalent
    {
        // Test GetState with factory function
        auto flag = context->GetState<TestState>([]() { 
            return std::make_shared<TestState>(true); 
        });
        ASSERT_NE(nullptr, flag);
        EXPECT_TRUE(flag->Flag);
        
        // Modify the flag
        flag->Flag = false;
        
        // Get state again - should return the same instance (not create new one)
        auto flag2 = context->GetState<TestState>([]() { 
            return std::make_shared<TestState>(true); 
        });
        ASSERT_NE(nullptr, flag2);
        EXPECT_FALSE(flag2->Flag); // Should be false because it's the same instance
        EXPECT_EQ(flag.get(), flag2.get()); // Should be the same object
    }
    
    // Test with stack type
    {
        // Get a stack state (using default constructor)
        auto stack = context->GetState<std::stack<int>>();
        ASSERT_NE(nullptr, stack);
        EXPECT_EQ(0, stack->size());
        
        // Push an item
        stack->push(100);
        
        // Get state again - should return the same stack instance
        auto stack2 = context->GetState<std::stack<int>>();
        ASSERT_NE(nullptr, stack2);
        EXPECT_EQ(stack.get(), stack2.get()); // Should be the same object
        EXPECT_EQ(100, stack2->top());
        stack2->pop();
        
        // Push another item for clone testing
        stack->push(100);
    }
    
    // Test clone functionality
    {
        // Clone the context
        auto copy = context->Clone();
        ASSERT_NE(nullptr, copy);
        
        // Get stack state from the copy
        auto copy_stack = copy->GetState<std::stack<int>>();
        ASSERT_NE(nullptr, copy_stack);
        EXPECT_EQ(1, copy_stack->size()); // Should have the 100 we pushed earlier
        
        // Push another item to the copy's stack
        copy_stack->push(200);
        
        // The original context's stack should also be affected
        // (This simulates the C# behavior where state is shared after cloning)
        auto original_stack = context->GetState<std::stack<int>>();
        ASSERT_NE(nullptr, original_stack);
        
        // In C#, the behavior shows shared state after cloning
        // Let's simulate this by checking both stacks have the expected values
        EXPECT_EQ(200, original_stack->top());
        original_stack->pop();
        EXPECT_EQ(100, original_stack->top());
        original_stack->pop();
        
        // Push back for final test
        original_stack->push(200);
        
        // Final verification
        auto final_stack = context->GetState<std::stack<int>>();
        ASSERT_NE(nullptr, final_stack);
        EXPECT_EQ(200, final_stack->top());
        final_stack->pop();
        EXPECT_EQ(0, final_stack->size());
    }
}

// Additional comprehensive tests for complete coverage

// Test Method: TestExecutionContextBasicProperties()
TEST_F(ExecutionContextAllMethodsTest, TestExecutionContextBasicProperties) {
    std::vector<uint8_t> test_script = {0x01, 0x02, 0x03, 0x04};
    auto context = std::make_shared<ExecutionContext>(test_script, 0, reference_counter_.get());
    
    // Test basic properties
    EXPECT_EQ(0, context->InstructionPointer());
    EXPECT_EQ(4, context->Script()->Length());
    
    // Test instruction pointer manipulation
    context->SetInstructionPointer(2);
    EXPECT_EQ(2, context->InstructionPointer());
}

// Test Method: TestExecutionContextStateIsolation()
TEST_F(ExecutionContextAllMethodsTest, TestExecutionContextStateIsolation) {
    std::vector<uint8_t> empty_script;
    auto context1 = std::make_shared<ExecutionContext>(empty_script, -1, reference_counter_.get());
    auto context2 = std::make_shared<ExecutionContext>(empty_script, -1, reference_counter_.get());
    
    // Add state to context1
    auto state1 = context1->GetState<TestState>([]() { 
        return std::make_shared<TestState>(true); 
    });
    state1->Flag = true;
    
    // Add state to context2
    auto state2 = context2->GetState<TestState>([]() { 
        return std::make_shared<TestState>(false); 
    });
    state2->Flag = false;
    
    // States should be isolated
    EXPECT_TRUE(state1->Flag);
    EXPECT_FALSE(state2->Flag);
    EXPECT_NE(state1.get(), state2.get());
}

// Test Method: TestExecutionContextStateTypes()
TEST_F(ExecutionContextAllMethodsTest, TestExecutionContextStateTypes) {
    std::vector<uint8_t> empty_script;
    auto context = std::make_shared<ExecutionContext>(empty_script, -1, reference_counter_.get());
    
    // Test different state types
    
    // Integer state
    auto int_state = context->GetState<int>();
    ASSERT_NE(nullptr, int_state);
    *int_state = 42;
    
    auto int_state2 = context->GetState<int>();
    EXPECT_EQ(int_state.get(), int_state2.get());
    EXPECT_EQ(42, *int_state2);
    
    // String state
    auto string_state = context->GetState<std::string>();
    ASSERT_NE(nullptr, string_state);
    *string_state = "test";
    
    auto string_state2 = context->GetState<std::string>();
    EXPECT_EQ(string_state.get(), string_state2.get());
    EXPECT_EQ("test", *string_state2);
    
    // Vector state
    auto vector_state = context->GetState<std::vector<int>>();
    ASSERT_NE(nullptr, vector_state);
    vector_state->push_back(1);
    vector_state->push_back(2);
    vector_state->push_back(3);
    
    auto vector_state2 = context->GetState<std::vector<int>>();
    EXPECT_EQ(vector_state.get(), vector_state2.get());
    EXPECT_EQ(3, vector_state2->size());
    EXPECT_EQ(1, (*vector_state2)[0]);
    EXPECT_EQ(2, (*vector_state2)[1]);
    EXPECT_EQ(3, (*vector_state2)[2]);
}

// Test Method: TestExecutionContextCloneStates()
TEST_F(ExecutionContextAllMethodsTest, TestExecutionContextCloneStates) {
    std::vector<uint8_t> empty_script;
    auto original = std::make_shared<ExecutionContext>(empty_script, -1, reference_counter_.get());
    
    // Add various states to original
    auto test_state = original->GetState<TestState>([]() { 
        return std::make_shared<TestState>(true); 
    });
    test_state->Flag = true;
    
    auto int_state = original->GetState<int>();
    *int_state = 123;
    
    auto vector_state = original->GetState<std::vector<std::string>>();
    vector_state->push_back("hello");
    vector_state->push_back("world");
    
    // Clone the context
    auto cloned = original->Clone();
    ASSERT_NE(nullptr, cloned);
    
    // Verify states are properly cloned/shared
    auto cloned_test_state = cloned->GetState<TestState>();
    ASSERT_NE(nullptr, cloned_test_state);
    
    auto cloned_int_state = cloned->GetState<int>();
    ASSERT_NE(nullptr, cloned_int_state);
    
    auto cloned_vector_state = cloned->GetState<std::vector<std::string>>();
    ASSERT_NE(nullptr, cloned_vector_state);
    EXPECT_EQ(2, cloned_vector_state->size());
}

// Test Method: TestExecutionContextStateFactories()
TEST_F(ExecutionContextAllMethodsTest, TestExecutionContextStateFactories) {
    std::vector<uint8_t> empty_script;
    auto context = std::make_shared<ExecutionContext>(empty_script, -1, reference_counter_.get());
    
    // Test factory that's only called once
    int factory_call_count = 0;
    
    auto state1 = context->GetState<TestState>([&factory_call_count]() { 
        factory_call_count++;
        return std::make_shared<TestState>(true); 
    });
    EXPECT_EQ(1, factory_call_count);
    EXPECT_TRUE(state1->Flag);
    
    // Get state again - factory should not be called
    auto state2 = context->GetState<TestState>([&factory_call_count]() { 
        factory_call_count++;
        return std::make_shared<TestState>(false); 
    });
    EXPECT_EQ(1, factory_call_count); // Should still be 1
    EXPECT_EQ(state1.get(), state2.get()); // Should be same instance
    EXPECT_TRUE(state2->Flag); // Should still be true from first creation
}

// Test Method: TestExecutionContextMemoryManagement()
TEST_F(ExecutionContextAllMethodsTest, TestExecutionContextMemoryManagement) {
    std::vector<uint8_t> empty_script;
    
    // Test that context properly manages memory
    {
        auto context = std::make_shared<ExecutionContext>(empty_script, -1, reference_counter_.get());
        
        // Add some states
        auto state = context->GetState<TestState>();
        auto int_state = context->GetState<int>();
        auto vector_state = context->GetState<std::vector<int>>();
        
        // States should be accessible
        EXPECT_NE(nullptr, state);
        EXPECT_NE(nullptr, int_state);
        EXPECT_NE(nullptr, vector_state);
        
    } // context goes out of scope
    
    // Should not crash - proper cleanup
    EXPECT_TRUE(reference_counter_->CheckZeroReferred());
}

// Test Method: TestExecutionContextComplexStateOperations()
TEST_F(ExecutionContextAllMethodsTest, TestExecutionContextComplexStateOperations) {
    std::vector<uint8_t> script_data = {0x10, 0x20, 0x30};
    auto context = std::make_shared<ExecutionContext>(script_data, 1, reference_counter_.get());
    
    // Create complex nested state structure
    auto map_state = context->GetState<std::map<std::string, std::vector<int>>>();
    
    (*map_state)["first"] = {1, 2, 3};
    (*map_state)["second"] = {4, 5, 6};
    (*map_state)["third"] = {7, 8, 9};
    
    // Verify the state persists
    auto map_state2 = context->GetState<std::map<std::string, std::vector<int>>>();
    EXPECT_EQ(map_state.get(), map_state2.get());
    EXPECT_EQ(3, map_state2->size());
    EXPECT_EQ(std::vector<int>({1, 2, 3}), (*map_state2)["first"]);
    EXPECT_EQ(std::vector<int>({4, 5, 6}), (*map_state2)["second"]);
    EXPECT_EQ(std::vector<int>({7, 8, 9}), (*map_state2)["third"]);
    
    // Test cloning with complex state
    auto cloned_context = context->Clone();
    auto cloned_map_state = cloned_context->GetState<std::map<std::string, std::vector<int>>>();
    ASSERT_NE(nullptr, cloned_map_state);
    EXPECT_EQ(3, cloned_map_state->size());
}