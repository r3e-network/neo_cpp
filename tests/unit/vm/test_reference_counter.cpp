#include <gtest/gtest.h>
#include <neo/vm/reference_counter.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/script.h>
#include <neo/vm/debugger.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/opcode.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/compound_items.h>
#include <neo/io/byte_span.h>

using namespace neo::vm;
using namespace neo::io;

class UT_ReferenceCounter : public testing::Test
{
protected:
    void SetUp() override
    {
        // Setup code if needed
    }

    void TearDown() override
    {
        // Teardown code if needed
    }
};

TEST_F(UT_ReferenceCounter, TestCircularReferences)
{
    // First test a simple INITSSLOT to ensure it works
    {
        ScriptBuilder sb_test;
        uint8_t slotCount = 1;
        sb_test.Emit(OpCode::INITSSLOT, ByteSpan(&slotCount, 1));
        sb_test.Emit(OpCode::RET);
        
        ExecutionEngine engine_test;
        auto bytes_test = sb_test.ToArray();
        neo::vm::internal::ByteVector internalBytes_test;
        internalBytes_test.Reserve(bytes_test.Size());
        for (size_t i = 0; i < bytes_test.Size(); ++i)
        {
            internalBytes_test.Push(bytes_test[i]);
        }
        Script script_test(internalBytes_test);
        engine_test.LoadScript(script_test);
        
        std::cout << "Script bytes: ";
        for (size_t i = 0; i < internalBytes_test.Size(); ++i) {
            std::cout << std::hex << (int)internalBytes_test[i] << " ";
        }
        std::cout << std::endl;
        
        auto state_test = engine_test.Execute();
        if (state_test == VMState::Fault) {
            std::cout << "Simple INITSSLOT test failed!" << std::endl;
            // Don't fail, just continue to see more details
        }
    }
    
    ScriptBuilder sb;
    uint8_t slotCount = 1;
    sb.Emit(OpCode::INITSSLOT, ByteSpan(&slotCount, 1)); //{}|{null}:1
    sb.EmitPush(static_cast<int64_t>(0)); //{0}|{null}:2
    sb.Emit(OpCode::NEWARRAY); //{A[]}|{null}:2
    sb.Emit(OpCode::DUP); //{A[],A[]}|{null}:3
    sb.Emit(OpCode::DUP); //{A[],A[],A[]}|{null}:4
    sb.Emit(OpCode::APPEND); //{A[A]}|{null}:3
    sb.Emit(OpCode::DUP); //{A[A],A[A]}|{null}:4
    sb.EmitPush(static_cast<int64_t>(0)); //{A[A],A[A],0}|{null}:5
    sb.Emit(OpCode::NEWARRAY); //{A[A],A[A],B[]}|{null}:5
    sb.Emit(OpCode::STSFLD0); //{A[A],A[A]}|{B[]}:4
    sb.Emit(OpCode::LDSFLD0); //{A[A],A[A],B[]}|{B[]}:5
    sb.Emit(OpCode::APPEND); //{A[A,B]}|{B[]}:4
    sb.Emit(OpCode::LDSFLD0); //{A[A,B],B[]}|{B[]}:5
    sb.EmitPush(static_cast<int64_t>(0)); //{A[A,B],B[],0}|{B[]}:6
    sb.Emit(OpCode::NEWARRAY); //{A[A,B],B[],C[]}|{B[]}:6
    sb.Emit(OpCode::TUCK); //{A[A,B],C[],B[],C[]}|{B[]}:7
    sb.Emit(OpCode::APPEND); //{A[A,B],C[]}|{B[C]}:6
    sb.EmitPush(static_cast<int64_t>(0)); //{A[A,B],C[],0}|{B[C]}:7
    sb.Emit(OpCode::NEWARRAY); //{A[A,B],C[],D[]}|{B[C]}:7
    sb.Emit(OpCode::TUCK); //{A[A,B],D[],C[],D[]}|{B[C]}:8
    sb.Emit(OpCode::APPEND); //{A[A,B],D[]}|{B[C[D]]}:7
    sb.Emit(OpCode::LDSFLD0); //{A[A,B],D[],B[C]}|{B[C[D]]}:8
    sb.Emit(OpCode::APPEND); //{A[A,B]}|{B[C[D[B]]]}:7
    sb.Emit(OpCode::PUSHNULL); //{A[A,B],null}|{B[C[D[B]]]}:8
    sb.Emit(OpCode::STSFLD0); //{A[A,B[C[D[B]]]]}|{null}:7
    sb.Emit(OpCode::DUP); //{A[A,B[C[D[B]]]],A[A,B]}|{null}:8
    sb.EmitPush(static_cast<int64_t>(1)); //{A[A,B[C[D[B]]]],A[A,B],1}|{null}:9
    sb.Emit(OpCode::REMOVE); //{A[A]}|{null}:3
    sb.Emit(OpCode::STSFLD0); //{}|{A[A]}:2
    sb.Emit(OpCode::RET); //{}:0

    ExecutionEngine engine;
    Debugger debugger(engine);
    // Convert io::ByteVector to internal::ByteVector
    auto bytes = sb.ToArray();
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i)
    {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    engine.LoadScript(script);
    
    auto state = debugger.StepInto();
    if (state == VMState::Fault) {
        std::cout << "Faulted on first instruction (INITSSLOT)!" << std::endl;
    }
    ASSERT_EQ(VMState::Break, state);
    ASSERT_EQ(1, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(2, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(2, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(3, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(4, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(3, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(4, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(5, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(5, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(4, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(5, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(4, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(5, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(6, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(6, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(7, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(6, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(7, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(7, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(8, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(7, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(8, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(7, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(8, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(7, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(8, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(9, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(6, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(5, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Halt, debugger.Execute());
    ASSERT_EQ(4, engine.GetReferenceCounter()->Count());
}

TEST_F(UT_ReferenceCounter, TestRemoveReferrer)
{
    ScriptBuilder sb;
    uint8_t slotCount = 1;
    sb.Emit(OpCode::INITSSLOT, ByteSpan(&slotCount, 1)); //{}|{null}:1
    sb.EmitPush(static_cast<int64_t>(0)); //{0}|{null}:2
    sb.Emit(OpCode::NEWARRAY); //{A[]}|{null}:2
    sb.Emit(OpCode::DUP); //{A[],A[]}|{null}:3
    sb.EmitPush(static_cast<int64_t>(0)); //{A[],A[],0}|{null}:4
    sb.Emit(OpCode::NEWARRAY); //{A[],A[],B[]}|{null}:4
    sb.Emit(OpCode::STSFLD0); //{A[],A[]}|{B[]}:3
    sb.Emit(OpCode::LDSFLD0); //{A[],A[],B[]}|{B[]}:4
    sb.Emit(OpCode::APPEND); //{A[B]}|{B[]}:3
    sb.Emit(OpCode::DROP); //{}|{B[]}:1
    sb.Emit(OpCode::RET); //{}:0

    ExecutionEngine engine;
    Debugger debugger(engine);
    // Convert io::ByteVector to internal::ByteVector
    auto bytes = sb.ToArray();
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i)
    {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    engine.LoadScript(script);
    
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(1, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(2, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(2, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(3, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(4, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(4, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(3, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(4, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(3, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Break, debugger.StepInto());
    ASSERT_EQ(2, engine.GetReferenceCounter()->Count());
    ASSERT_EQ(VMState::Halt, debugger.Execute());
    ASSERT_EQ(1, engine.GetReferenceCounter()->Count());
}

TEST_F(UT_ReferenceCounter, TestCheckZeroReferredWithArray)
{
    ScriptBuilder sb;

    sb.EmitPush(static_cast<int64_t>(ExecutionEngineLimits::Default.MaxStackSize - 1));
    sb.Emit(OpCode::NEWARRAY);

    // Good with MaxStackSize
    {
        ExecutionEngine engine;
        // Convert io::ByteVector to internal::ByteVector
        auto bytes = sb.ToArray();
        neo::vm::internal::ByteVector internalBytes;
        internalBytes.Reserve(bytes.Size());
        for (size_t i = 0; i < bytes.Size(); ++i)
        {
            internalBytes.Push(bytes[i]);
        }
        Script script(internalBytes);
        engine.LoadScript(script);
        ASSERT_EQ(0, engine.GetReferenceCounter()->Count());

        ASSERT_EQ(VMState::Halt, engine.Execute());
        ASSERT_EQ(static_cast<int>(ExecutionEngineLimits::Default.MaxStackSize), engine.GetReferenceCounter()->Count());
    }

    // Fault with MaxStackSize+1
    sb.Emit(OpCode::PUSH1);

    {
        ExecutionEngine engine;
        // Convert io::ByteVector to internal::ByteVector
        auto bytes = sb.ToArray();
        neo::vm::internal::ByteVector internalBytes;
        internalBytes.Reserve(bytes.Size());
        for (size_t i = 0; i < bytes.Size(); ++i)
        {
            internalBytes.Push(bytes[i]);
        }
        Script script(internalBytes);
        engine.LoadScript(script);
        ASSERT_EQ(0, engine.GetReferenceCounter()->Count());

        ASSERT_EQ(VMState::Fault, engine.Execute());
        ASSERT_EQ(static_cast<int>(ExecutionEngineLimits::Default.MaxStackSize) + 1, engine.GetReferenceCounter()->Count());
    }
}

TEST_F(UT_ReferenceCounter, TestCheckZeroReferred)
{
    // Create a scenario with circular references to test CheckZeroReferred
    auto referenceCounter = std::make_shared<ReferenceCounter>();
    
    std::vector<StackItem> empty1, empty2, empty3;
    auto array1 = std::make_shared<ArrayItem>(std::vector<std::shared_ptr<StackItem>>(), referenceCounter.get());
    auto array2 = std::make_shared<ArrayItem>(std::vector<std::shared_ptr<StackItem>>(), referenceCounter.get());
    auto array3 = std::make_shared<ArrayItem>(std::vector<std::shared_ptr<StackItem>>(), referenceCounter.get());
    
    // Create circular references between arrays
    array1->Add(array2);
    array2->Add(array3);
    array3->Add(array1);
    
    // Manual testing of reference counter behavior
    ASSERT_EQ(3, referenceCounter->Count());
    
    // Remove a reference and ensure the circular reference is handled correctly
    array1->Clear();
    
    // Reference counter should detect and handle this properly
    ASSERT_EQ(0, referenceCounter->Count());
}
