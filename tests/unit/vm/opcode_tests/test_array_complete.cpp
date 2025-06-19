#include <gtest/gtest.h>
#include "neo/vm/execution_engine.h"
#include "neo/vm/script_builder.h"
#include "neo/vm/stack_item.h"
#include "neo/vm/opcode.h"
#include "neo/vm/types/array.h"
#include "neo/vm/types/struct.h"
#include "neo/vm/types/map.h"

using namespace neo;
using namespace neo::vm;

class ArrayOpcodeTest : public ::testing::Test {
protected:
    std::unique_ptr<ExecutionEngine> engine;
    
    void SetUp() override {
        engine = std::make_unique<ExecutionEngine>();
    }
    
    void ExecuteScript(const ByteVector& script) {
        engine->LoadScript(script);
        engine->Execute();
    }
    
    void CheckState(VMState expected) {
        EXPECT_EQ(engine->State(), expected);
    }
    
    void CheckStackSize(size_t expected) {
        ASSERT_EQ(engine->EvaluationStack().size(), expected);
    }
};

// PACK Tests
TEST_F(ArrayOpcodeTest, PACK_EmptyArray) {
    ScriptBuilder sb;
    sb.EmitPush(0);
    sb.EmitOpCode(OpCode::PACK);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto array = engine->EvaluationStack().Pop()->GetArray();
    EXPECT_EQ(array->Count(), 0);
}

TEST_F(ArrayOpcodeTest, PACK_SingleElement) {
    ScriptBuilder sb;
    sb.EmitPush(42);
    sb.EmitPush(1);
    sb.EmitOpCode(OpCode::PACK);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto array = engine->EvaluationStack().Pop()->GetArray();
    ASSERT_EQ(array->Count(), 1);
    EXPECT_EQ(array->Get(0)->GetInteger(), 42);
}

TEST_F(ArrayOpcodeTest, PACK_MultipleElements) {
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::PACK);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto array = engine->EvaluationStack().Pop()->GetArray();
    ASSERT_EQ(array->Count(), 3);
    EXPECT_EQ(array->Get(0)->GetInteger(), 3); // Last pushed is first in array
    EXPECT_EQ(array->Get(1)->GetInteger(), 2);
    EXPECT_EQ(array->Get(2)->GetInteger(), 1);
}

// UNPACK Tests
TEST_F(ArrayOpcodeTest, UNPACK_EmptyArray) {
    ScriptBuilder sb;
    sb.EmitOpCode(OpCode::NEWARRAY0);
    sb.EmitOpCode(OpCode::UNPACK);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Peek()->GetInteger(), 0); // Count
}

TEST_F(ArrayOpcodeTest, UNPACK_SingleElement) {
    ScriptBuilder sb;
    sb.EmitPush(42);
    sb.EmitPush(1);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitOpCode(OpCode::UNPACK);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(2); // Element + count
    
    auto count = engine->EvaluationStack().Pop()->GetInteger();
    auto element = engine->EvaluationStack().Pop()->GetInteger();
    EXPECT_EQ(count, 1);
    EXPECT_EQ(element, 42);
}

TEST_F(ArrayOpcodeTest, UNPACK_MultipleElements) {
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitOpCode(OpCode::UNPACK);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(4); // 3 elements + count
    
    auto count = engine->EvaluationStack().Pop()->GetInteger();
    EXPECT_EQ(count, 3);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 3);
}

// NEWARRAY Tests
TEST_F(ArrayOpcodeTest, NEWARRAY_Zero) {
    ScriptBuilder sb;
    sb.EmitPush(0);
    sb.EmitOpCode(OpCode::NEWARRAY);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto array = engine->EvaluationStack().Pop()->GetArray();
    EXPECT_EQ(array->Count(), 0);
}

TEST_F(ArrayOpcodeTest, NEWARRAY_PositiveSize) {
    ScriptBuilder sb;
    sb.EmitPush(5);
    sb.EmitOpCode(OpCode::NEWARRAY);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto array = engine->EvaluationStack().Pop()->GetArray();
    ASSERT_EQ(array->Count(), 5);
    
    // All elements should be null
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(array->Get(i)->IsNull());
    }
}

TEST_F(ArrayOpcodeTest, NEWARRAY_NegativeSize) {
    ScriptBuilder sb;
    sb.EmitPush(-1);
    sb.EmitOpCode(OpCode::NEWARRAY);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

// NEWARRAY_T Tests
TEST_F(ArrayOpcodeTest, NEWARRAY_T_Boolean) {
    ScriptBuilder sb;
    sb.EmitPush(3);
    sb.EmitPush(static_cast<uint8_t>(StackItemType::Boolean));
    sb.EmitOpCode(OpCode::NEWARRAY_T);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto array = engine->EvaluationStack().Pop()->GetArray();
    ASSERT_EQ(array->Count(), 3);
    
    // All elements should be false
    for (int i = 0; i < 3; i++) {
        EXPECT_FALSE(array->Get(i)->GetBoolean());
    }
}

// NEWSTRUCT Tests
TEST_F(ArrayOpcodeTest, NEWSTRUCT_Zero) {
    ScriptBuilder sb;
    sb.EmitPush(0);
    sb.EmitOpCode(OpCode::NEWSTRUCT);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto struct_item = engine->EvaluationStack().Pop()->GetStruct();
    EXPECT_EQ(struct_item->Count(), 0);
}

TEST_F(ArrayOpcodeTest, NEWSTRUCT_WithSize) {
    ScriptBuilder sb;
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::NEWSTRUCT);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto struct_item = engine->EvaluationStack().Pop()->GetStruct();
    ASSERT_EQ(struct_item->Count(), 3);
    
    // All elements should be null
    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(struct_item->Get(i)->IsNull());
    }
}

// APPEND Tests
TEST_F(ArrayOpcodeTest, APPEND_ToEmptyArray) {
    ScriptBuilder sb;
    sb.EmitOpCode(OpCode::NEWARRAY0);
    sb.EmitPush(42);
    sb.EmitOpCode(OpCode::APPEND);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto array = engine->EvaluationStack().Pop()->GetArray();
    ASSERT_EQ(array->Count(), 1);
    EXPECT_EQ(array->Get(0)->GetInteger(), 42);
}

TEST_F(ArrayOpcodeTest, APPEND_ToExistingArray) {
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(2);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::APPEND);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto array = engine->EvaluationStack().Pop()->GetArray();
    ASSERT_EQ(array->Count(), 3);
    EXPECT_EQ(array->Get(2)->GetInteger(), 3);
}

// REVERSE Tests
TEST_F(ArrayOpcodeTest, REVERSE_EmptyArray) {
    ScriptBuilder sb;
    sb.EmitOpCode(OpCode::NEWARRAY0);
    sb.EmitOpCode(OpCode::REVERSE);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto array = engine->EvaluationStack().Pop()->GetArray();
    EXPECT_EQ(array->Count(), 0);
}

TEST_F(ArrayOpcodeTest, REVERSE_SingleElement) {
    ScriptBuilder sb;
    sb.EmitPush(42);
    sb.EmitPush(1);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitOpCode(OpCode::REVERSE);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto array = engine->EvaluationStack().Pop()->GetArray();
    ASSERT_EQ(array->Count(), 1);
    EXPECT_EQ(array->Get(0)->GetInteger(), 42);
}

TEST_F(ArrayOpcodeTest, REVERSE_MultipleElements) {
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitPush(4);
    sb.EmitPush(4);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitOpCode(OpCode::REVERSE);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto array = engine->EvaluationStack().Pop()->GetArray();
    ASSERT_EQ(array->Count(), 4);
    EXPECT_EQ(array->Get(0)->GetInteger(), 1);
    EXPECT_EQ(array->Get(1)->GetInteger(), 2);
    EXPECT_EQ(array->Get(2)->GetInteger(), 3);
    EXPECT_EQ(array->Get(3)->GetInteger(), 4);
}

// REMOVE Tests
TEST_F(ArrayOpcodeTest, REMOVE_FirstElement) {
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitPush(0); // Index
    sb.EmitOpCode(OpCode::REMOVE);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto array = engine->EvaluationStack().Pop()->GetArray();
    ASSERT_EQ(array->Count(), 2);
    EXPECT_EQ(array->Get(0)->GetInteger(), 2);
    EXPECT_EQ(array->Get(1)->GetInteger(), 1);
}

TEST_F(ArrayOpcodeTest, REMOVE_LastElement) {
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitPush(2); // Index
    sb.EmitOpCode(OpCode::REMOVE);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto array = engine->EvaluationStack().Pop()->GetArray();
    ASSERT_EQ(array->Count(), 2);
    EXPECT_EQ(array->Get(0)->GetInteger(), 3);
    EXPECT_EQ(array->Get(1)->GetInteger(), 2);
}

TEST_F(ArrayOpcodeTest, REMOVE_InvalidIndex) {
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(2);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitPush(5); // Out of bounds
    sb.EmitOpCode(OpCode::REMOVE);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

// CLEARITEMS Tests
TEST_F(ArrayOpcodeTest, CLEARITEMS_Array) {
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitOpCode(OpCode::DUP); // Keep reference
    sb.EmitOpCode(OpCode::CLEARITEMS);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(2);
    
    engine->EvaluationStack().Pop(); // Remove the cleared array
    auto array = engine->EvaluationStack().Pop()->GetArray();
    EXPECT_EQ(array->Count(), 0);
}

// POPITEM Tests
TEST_F(ArrayOpcodeTest, POPITEM_Array) {
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitOpCode(OpCode::POPITEM);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(2);
    
    auto popped = engine->EvaluationStack().Pop()->GetInteger();
    auto array = engine->EvaluationStack().Pop()->GetArray();
    
    EXPECT_EQ(popped, 1); // Last element
    EXPECT_EQ(array->Count(), 2);
}

// SIZE Tests
TEST_F(ArrayOpcodeTest, SIZE_EmptyArray) {
    ScriptBuilder sb;
    sb.EmitOpCode(OpCode::NEWARRAY0);
    sb.EmitOpCode(OpCode::SIZE);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 0);
}

TEST_F(ArrayOpcodeTest, SIZE_NonEmptyArray) {
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitOpCode(OpCode::SIZE);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 3);
}

// PICKITEM Tests
TEST_F(ArrayOpcodeTest, PICKITEM_ValidIndex) {
    ScriptBuilder sb;
    sb.EmitPush(10);
    sb.EmitPush(20);
    sb.EmitPush(30);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitPush(1); // Index
    sb.EmitOpCode(OpCode::PICKITEM);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 20);
}

TEST_F(ArrayOpcodeTest, PICKITEM_NegativeIndex) {
    ScriptBuilder sb;
    sb.EmitPush(10);
    sb.EmitPush(20);
    sb.EmitPush(30);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitPush(-1); // Negative index (last element)
    sb.EmitOpCode(OpCode::PICKITEM);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 10);
}

// SETITEM Tests
TEST_F(ArrayOpcodeTest, SETITEM_ValidIndex) {
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitPush(1);   // Index
    sb.EmitPush(99);  // New value
    sb.EmitOpCode(OpCode::SETITEM);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto array = engine->EvaluationStack().Pop()->GetArray();
    EXPECT_EQ(array->Get(1)->GetInteger(), 99);
}

// NEWMAP Tests
TEST_F(ArrayOpcodeTest, NEWMAP_Empty) {
    ScriptBuilder sb;
    sb.EmitOpCode(OpCode::NEWMAP);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto map = engine->EvaluationStack().Pop()->GetMap();
    EXPECT_EQ(map->Count(), 0);
}

// HASKEY Tests
TEST_F(ArrayOpcodeTest, HASKEY_ArrayValidIndex) {
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitPush(1);
    sb.EmitOpCode(OpCode::HASKEY);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    EXPECT_TRUE(engine->EvaluationStack().Pop()->GetBoolean());
}

TEST_F(ArrayOpcodeTest, HASKEY_ArrayInvalidIndex) {
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(2);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitPush(5);
    sb.EmitOpCode(OpCode::HASKEY);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    EXPECT_FALSE(engine->EvaluationStack().Pop()->GetBoolean());
}

// KEYS Tests
TEST_F(ArrayOpcodeTest, KEYS_Map) {
    ScriptBuilder sb;
    sb.EmitOpCode(OpCode::NEWMAP);
    sb.EmitOpCode(OpCode::DUP);
    sb.EmitPush("key1");
    sb.EmitPush("value1");
    sb.EmitOpCode(OpCode::SETITEM);
    sb.EmitOpCode(OpCode::DUP);
    sb.EmitPush("key2");
    sb.EmitPush("value2");
    sb.EmitOpCode(OpCode::SETITEM);
    sb.EmitOpCode(OpCode::KEYS);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto keys = engine->EvaluationStack().Pop()->GetArray();
    EXPECT_EQ(keys->Count(), 2);
}

// VALUES Tests
TEST_F(ArrayOpcodeTest, VALUES_Array) {
    ScriptBuilder sb;
    sb.EmitPush(10);
    sb.EmitPush(20);
    sb.EmitPush(30);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::PACK);
    sb.EmitOpCode(OpCode::VALUES);
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    auto values = engine->EvaluationStack().Pop()->GetArray();
    ASSERT_EQ(values->Count(), 3);
    EXPECT_EQ(values->Get(0)->GetInteger(), 30);
    EXPECT_EQ(values->Get(1)->GetInteger(), 20);
    EXPECT_EQ(values->Get(2)->GetInteger(), 10);
}

// Complex array operation tests
TEST_F(ArrayOpcodeTest, NestedArrays) {
    ScriptBuilder sb;
    // Create inner array [1, 2]
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(2);
    sb.EmitOpCode(OpCode::PACK);
    
    // Create outer array [[1, 2], 3]
    sb.EmitPush(3);
    sb.EmitPush(2);
    sb.EmitOpCode(OpCode::PACK);
    
    // Access nested element
    sb.EmitPush(0);
    sb.EmitOpCode(OpCode::PICKITEM); // Get inner array
    sb.EmitPush(1);
    sb.EmitOpCode(OpCode::PICKITEM); // Get element from inner array
    
    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
}

TEST_F(ArrayOpcodeTest, ArraySlicing) {
    // Create array and get a slice
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.EmitPush(4);
    sb.EmitPush(5);
    sb.EmitPush(5);
    sb.EmitOpCode(OpCode::PACK);
    
    // Use SUBSTR opcode for slicing (if supported)
    sb.EmitPush(1); // Start index
    sb.EmitPush(3); // Count
    sb.EmitOpCode(OpCode::SUBSTR);
    
    ExecuteScript(sb.ToByteArray());
    // Check result based on implementation
}