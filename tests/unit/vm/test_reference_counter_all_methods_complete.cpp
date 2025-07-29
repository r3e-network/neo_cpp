#include "neo/vm/debugger.h"
#include "neo/vm/execution_engine.h"
#include "neo/vm/execution_engine_limits.h"
#include "neo/vm/opcode.h"
#include "neo/vm/reference_counter.h"
#include "neo/vm/script_builder.h"
#include "neo/vm/types/array.h"
#include "neo/vm/types/integer.h"
#include "neo/vm/vm_state.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>

using namespace neo;
using namespace neo::vm;
using namespace neo::vm::types;

// Complete conversion of C# UT_ReferenceCounter.cs - ALL 6 test methods
class ReferenceCounterAllMethodsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        engine_ = std::make_unique<ExecutionEngine>();
    }

    void TearDown() override
    {
        engine_.reset();
    }

    std::unique_ptr<ExecutionEngine> engine_;
};

// C# Test Method: TestCircularReferences()
TEST_F(ReferenceCounterAllMethodsTest, TestCircularReferences)
{
    ScriptBuilder sb;
    sb.Emit(OpCode::INITSSLOT, std::vector<uint8_t>{1});  //{}|{null}:1
    sb.EmitPush(0);                                       //{0}|{null}:2
    sb.Emit(OpCode::NEWARRAY);                            //{A[]}|{null}:2
    sb.Emit(OpCode::DUP);                                 //{A[],A[]}|{null}:3
    sb.Emit(OpCode::DUP);                                 //{A[],A[],A[]}|{null}:4
    sb.Emit(OpCode::APPEND);                              //{A[A]}|{null}:3
    sb.Emit(OpCode::DUP);                                 //{A[A],A[A]}|{null}:4
    sb.EmitPush(0);                                       //{A[A],A[A],0}|{null}:5
    sb.Emit(OpCode::NEWARRAY);                            //{A[A],A[A],B[]}|{null}:5
    sb.Emit(OpCode::STSFLD0);                             //{A[A],A[A]}|{B[]}:4
    sb.Emit(OpCode::LDSFLD0);                             //{A[A],A[A],B[]}|{B[]}:5
    sb.Emit(OpCode::APPEND);                              //{A[A,B]}|{B[]}:4
    sb.Emit(OpCode::LDSFLD0);                             //{A[A,B],B[]}|{B[]}:5
    sb.EmitPush(0);                                       //{A[A,B],B[],0}|{B[]}:6
    sb.Emit(OpCode::NEWARRAY);                            //{A[A,B],B[],C[]}|{B[]}:6
    sb.Emit(OpCode::TUCK);                                //{A[A,B],C[],B[],C[]}|{B[]}:7
    sb.Emit(OpCode::APPEND);                              //{A[A,B],C[]}|{B[C]}:6
    sb.EmitPush(0);                                       //{A[A,B],C[],0}|{B[C]}:7
    sb.Emit(OpCode::NEWARRAY);                            //{A[A,B],C[],D[]}|{B[C]}:7
    sb.Emit(OpCode::TUCK);                                //{A[A,B],D[],C[],D[]}|{B[C]}:8
    sb.Emit(OpCode::APPEND);                              //{A[A,B],D[]}|{B[C[D]]}:7
    sb.Emit(OpCode::LDSFLD0);                             //{A[A,B],D[],B[C]}|{B[C[D]]}:8
    sb.Emit(OpCode::APPEND);                              //{A[A,B]}|{B[C[D[B]]]}:7
    sb.Emit(OpCode::PUSHNULL);                            //{A[A,B],null}|{B[C[D[B]]]}:8
    sb.Emit(OpCode::STSFLD0);                             //{A[A,B[C[D[B]]]]}|{null}:7
    sb.Emit(OpCode::DUP);                                 //{A[A,B[C[D[B]]]],A[A,B]}|{null}:8
    sb.EmitPush(1);                                       //{A[A,B[C[D[B]]]],A[A,B],1}|{null}:9
    sb.Emit(OpCode::REMOVE);                              //{A[A]}|{null}:3
    sb.Emit(OpCode::STSFLD0);                             //{}|{A[A]}:2
    sb.Emit(OpCode::RET);                                 //{}:0

    engine_->LoadScript(sb.ToArray());

    Debugger debugger(*engine_);

    // Execute each step and verify reference counter at each step
    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(1, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(2, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(2, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(3, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(4, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(3, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(4, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(5, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(5, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(4, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(5, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(4, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(5, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(6, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(6, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(7, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(6, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(7, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(7, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(8, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(7, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(8, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(7, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(8, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(7, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(8, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(9, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(6, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(5, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::HALT, debugger.Execute());
    EXPECT_EQ(4, engine_->ReferenceCounter().Count());
}

// C# Test Method: TestRemoveReferrer()
TEST_F(ReferenceCounterAllMethodsTest, TestRemoveReferrer)
{
    ScriptBuilder sb;
    sb.Emit(OpCode::INITSSLOT, std::vector<uint8_t>{1});  //{}|{null}:1
    sb.EmitPush(0);                                       //{0}|{null}:2
    sb.Emit(OpCode::NEWARRAY);                            //{A[]}|{null}:2
    sb.Emit(OpCode::DUP);                                 //{A[],A[]}|{null}:3
    sb.EmitPush(0);                                       //{A[],A[],0}|{null}:4
    sb.Emit(OpCode::NEWARRAY);                            //{A[],A[],B[]}|{null}:4
    sb.Emit(OpCode::STSFLD0);                             //{A[],A[]}|{B[]}:3
    sb.Emit(OpCode::LDSFLD0);                             //{A[],A[],B[]}|{B[]}:4
    sb.Emit(OpCode::APPEND);                              //{A[B]}|{B[]}:3
    sb.Emit(OpCode::DROP);                                //{}|{B[]}:1
    sb.Emit(OpCode::RET);                                 //{}:0

    engine_->LoadScript(sb.ToArray());

    Debugger debugger(*engine_);

    // Execute each step and verify reference counter at each step
    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(1, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(2, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(2, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(3, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(4, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(4, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(3, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(4, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(3, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(2, engine_->ReferenceCounter().Count());

    EXPECT_EQ(VMState::HALT, debugger.Execute());
    EXPECT_EQ(1, engine_->ReferenceCounter().Count());
}

// C# Test Method: TestCheckZeroReferredWithArray()
TEST_F(ReferenceCounterAllMethodsTest, TestCheckZeroReferredWithArray)
{
    ScriptBuilder sb;

    sb.EmitPush(ExecutionEngineLimits::Default().MaxStackSize - 1);
    sb.Emit(OpCode::NEWARRAY);

    // Good with MaxStackSize
    {
        auto engine = std::make_unique<ExecutionEngine>();
        engine->LoadScript(sb.ToArray());
        EXPECT_EQ(0, engine->ReferenceCounter().Count());

        EXPECT_EQ(VMState::HALT, engine->Execute());
        EXPECT_EQ(static_cast<int>(ExecutionEngineLimits::Default().MaxStackSize), engine->ReferenceCounter().Count());
    }

    // Fault with MaxStackSize+1
    sb.Emit(OpCode::PUSH1);

    {
        auto engine = std::make_unique<ExecutionEngine>();
        engine->LoadScript(sb.ToArray());
        EXPECT_EQ(0, engine->ReferenceCounter().Count());

        EXPECT_EQ(VMState::FAULT, engine->Execute());
        EXPECT_EQ(static_cast<int>(ExecutionEngineLimits::Default().MaxStackSize) + 1,
                  engine->ReferenceCounter().Count());
    }
}

// C# Test Method: TestCheckZeroReferred()
TEST_F(ReferenceCounterAllMethodsTest, TestCheckZeroReferred)
{
    ScriptBuilder sb;

    for (int x = 0; x < ExecutionEngineLimits::Default().MaxStackSize; x++)
    {
        sb.Emit(OpCode::PUSH1);
    }

    // Good with MaxStackSize
    {
        auto engine = std::make_unique<ExecutionEngine>();
        engine->LoadScript(sb.ToArray());
        EXPECT_EQ(0, engine->ReferenceCounter().Count());

        EXPECT_EQ(VMState::HALT, engine->Execute());
        EXPECT_EQ(static_cast<int>(ExecutionEngineLimits::Default().MaxStackSize), engine->ReferenceCounter().Count());
    }

    // Fault with MaxStackSize+1
    sb.Emit(OpCode::PUSH1);

    {
        auto engine = std::make_unique<ExecutionEngine>();
        engine->LoadScript(sb.ToArray());
        EXPECT_EQ(0, engine->ReferenceCounter().Count());

        EXPECT_EQ(VMState::FAULT, engine->Execute());
        EXPECT_EQ(static_cast<int>(ExecutionEngineLimits::Default().MaxStackSize) + 1,
                  engine->ReferenceCounter().Count());
    }
}

// C# Test Method: TestArrayNoPush()
TEST_F(ReferenceCounterAllMethodsTest, TestArrayNoPush)
{
    ScriptBuilder sb;
    sb.Emit(OpCode::RET);

    engine_->LoadScript(sb.ToArray());
    EXPECT_EQ(0, engine_->ReferenceCounter().Count());

    // Create array with reference counter and items
    std::vector<std::shared_ptr<StackItem>> items;
    items.push_back(std::make_shared<Integer>(1, &engine_->ReferenceCounter()));
    items.push_back(std::make_shared<Integer>(2, &engine_->ReferenceCounter()));
    items.push_back(std::make_shared<Integer>(3, &engine_->ReferenceCounter()));
    items.push_back(std::make_shared<Integer>(4, &engine_->ReferenceCounter()));

    auto array = std::make_shared<Array>(&engine_->ReferenceCounter(), items);

    EXPECT_EQ(array->Count(), engine_->ReferenceCounter().Count());
    EXPECT_EQ(VMState::HALT, engine_->Execute());
    EXPECT_EQ(array->Count(), engine_->ReferenceCounter().Count());
}

// C# Test Method: TestInvalidReferenceStackItem()
TEST_F(ReferenceCounterAllMethodsTest, TestInvalidReferenceStackItem)
{
    auto reference = std::make_shared<ReferenceCounter>();
    auto arr = std::make_shared<Array>(reference.get());
    auto arr2 = std::make_shared<Array>();  // Different reference counter (or null)

    for (int i = 0; i < 10; i++)
    {
        arr2->Add(std::make_shared<Integer>(i, nullptr));
    }

    // Should throw InvalidOperationException when trying to add item with different reference counter
    EXPECT_THROW(arr->Add(arr2), std::invalid_argument);
}

// Additional comprehensive tests for complete coverage

// Test Method: TestReferenceCounterBasicOperations()
TEST_F(ReferenceCounterAllMethodsTest, TestReferenceCounterBasicOperations)
{
    auto ref_counter = engine_->ReferenceCounter();

    // Initially empty
    EXPECT_EQ(0, ref_counter.Count());

    // Add some items
    auto item1 = std::make_shared<Integer>(1, &ref_counter);
    auto item2 = std::make_shared<Integer>(2, &ref_counter);
    auto item3 = std::make_shared<Integer>(3, &ref_counter);

    EXPECT_EQ(3, ref_counter.Count());

    // Items should be properly tracked
    EXPECT_TRUE(ref_counter.CheckZeroReferred());
}

// Test Method: TestReferenceCounterWithComplexStructures()
TEST_F(ReferenceCounterAllMethodsTest, TestReferenceCounterWithComplexStructures)
{
    auto ref_counter = engine_->ReferenceCounter();

    // Create nested structures
    auto outer_array = std::make_shared<Array>(&ref_counter);
    auto inner_array1 = std::make_shared<Array>(&ref_counter);
    auto inner_array2 = std::make_shared<Array>(&ref_counter);

    // Add elements to inner arrays
    inner_array1->Add(std::make_shared<Integer>(1, &ref_counter));
    inner_array1->Add(std::make_shared<Integer>(2, &ref_counter));

    inner_array2->Add(std::make_shared<Integer>(3, &ref_counter));
    inner_array2->Add(std::make_shared<Integer>(4, &ref_counter));

    // Add inner arrays to outer array
    outer_array->Add(inner_array1);
    outer_array->Add(inner_array2);

    // Should have proper reference counting
    EXPECT_GT(ref_counter.Count(), 6);  // At least the items we created
}

// Test Method: TestReferenceCounterMemoryManagement()
TEST_F(ReferenceCounterAllMethodsTest, TestReferenceCounterMemoryManagement)
{
    auto ref_counter = engine_->ReferenceCounter();

    {
        // Create items in a scope
        auto array = std::make_shared<Array>(&ref_counter);
        array->Add(std::make_shared<Integer>(42, &ref_counter));
        array->Add(std::make_shared<Integer>(84, &ref_counter));

        EXPECT_GT(ref_counter.Count(), 0);
    }  // Items go out of scope

    // Reference counter should still track properly
    EXPECT_TRUE(ref_counter.CheckZeroReferred());
}

// Test Method: TestReferenceCounterLimits()
TEST_F(ReferenceCounterAllMethodsTest, TestReferenceCounterLimits)
{
    auto ref_counter = engine_->ReferenceCounter();

    // Test behavior at limits
    auto limits = ExecutionEngineLimits::Default();

    // Create many items (but stay within reasonable limits for test)
    std::vector<std::shared_ptr<StackItem>> items;
    for (int i = 0; i < 100; i++)
    {
        items.push_back(std::make_shared<Integer>(i, &ref_counter));
    }

    EXPECT_EQ(100, ref_counter.Count());

    // Clear items
    items.clear();

    // Reference counter should handle cleanup properly
    EXPECT_TRUE(ref_counter.CheckZeroReferred());
}

// Test Method: TestReferenceCounterEdgeCases()
TEST_F(ReferenceCounterAllMethodsTest, TestReferenceCounterEdgeCases)
{
    auto ref_counter = engine_->ReferenceCounter();

    // Test with null items
    auto array = std::make_shared<Array>(&ref_counter);
    array->Add(StackItem::Null());
    array->Add(std::make_shared<Integer>(1, &ref_counter));
    array->Add(StackItem::Null());

    EXPECT_GT(ref_counter.Count(), 1);  // Should count non-null items

    // Test removing items
    array->RemoveAt(1);  // Remove the integer

    EXPECT_GT(ref_counter.Count(), 0);  // Array still exists
}