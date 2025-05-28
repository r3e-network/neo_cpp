#include <gtest/gtest.h>
#include <neo/vm/execution_engine.h>

using namespace neo::vm;
using namespace neo::io;

TEST(ExecutionContextTest, Constructor)
{
    ByteVector bytes = ByteVector::Parse("0102030405");
    Script script(bytes);
    ExecutionContext context(script);

    EXPECT_EQ(context.GetScript().GetScript(), bytes);
    EXPECT_EQ(context.GetInstructionPointer(), 0);
    EXPECT_EQ(context.GetCurrentPosition(), 0);
    EXPECT_EQ(context.GetStaticFields().size(), 0);
    EXPECT_EQ(context.GetLocalVariables().size(), 0);
    EXPECT_EQ(context.GetArguments().size(), 0);
    EXPECT_EQ(context.GetEvaluationStack().size(), 0);
    EXPECT_EQ(context.GetTryCount(), 0);
}

TEST(ExecutionContextTest, InstructionPointer)
{
    ByteVector bytes = ByteVector::Parse("0102030405");
    Script script(bytes);
    ExecutionContext context(script);

    EXPECT_EQ(context.GetInstructionPointer(), 0);

    context.SetInstructionPointer(3);
    EXPECT_EQ(context.GetInstructionPointer(), 3);
    EXPECT_EQ(context.GetCurrentPosition(), 3);
}

TEST(ExecutionContextTest, GetNextInstruction)
{
    ByteVector bytes = ByteVector::Parse("0051"); // PUSH0, PUSH1
    Script script(bytes);
    ExecutionContext context(script);

    EXPECT_EQ(context.GetNextInstructionOpCode(), OpCode::PUSH0);

    context.SetInstructionPointer(1);
    EXPECT_EQ(context.GetNextInstructionOpCode(), OpCode::PUSH1);

    context.SetInstructionPointer(2);
    EXPECT_EQ(context.GetNextInstructionOpCode(), OpCode::RET);
}

TEST(ExecutionContextTest, StaticFields)
{
    ByteVector bytes = ByteVector::Parse("0102030405");
    Script script(bytes);
    ExecutionContext context(script);

    // Initialize static fields
    context.InitializeStaticFields(3);
    EXPECT_EQ(context.GetStaticFields().size(), 3);

    // Load/store static field
    auto item = StackItem::Create(123);
    context.StoreStaticField(1, item);
    EXPECT_EQ(context.LoadStaticField(1), item);

    // Out of range
    EXPECT_THROW(context.LoadStaticField(3), std::out_of_range);
    EXPECT_THROW(context.StoreStaticField(3, item), std::out_of_range);
}

TEST(ExecutionContextTest, LocalVariables)
{
    ByteVector bytes = ByteVector::Parse("0102030405");
    Script script(bytes);
    ExecutionContext context(script);

    // Initialize local variables
    context.InitializeLocalVariables(3, 2);
    EXPECT_EQ(context.GetLocalVariables().size(), 3);
    EXPECT_EQ(context.GetArguments().size(), 2);

    // Load/store local variable
    auto item = StackItem::Create(123);
    context.StoreLocalVariable(1, item);
    EXPECT_EQ(context.LoadLocalVariable(1), item);

    // Out of range
    EXPECT_THROW(context.LoadLocalVariable(3), std::out_of_range);
    EXPECT_THROW(context.StoreLocalVariable(3, item), std::out_of_range);
}

TEST(ExecutionContextTest, Arguments)
{
    ByteVector bytes = ByteVector::Parse("0102030405");
    Script script(bytes);
    ExecutionContext context(script);

    // Initialize arguments
    context.InitializeLocalVariables(3, 2);
    EXPECT_EQ(context.GetLocalVariables().size(), 3);
    EXPECT_EQ(context.GetArguments().size(), 2);

    // Load/store argument
    auto item = StackItem::Create(123);
    context.StoreArgument(1, item);
    EXPECT_EQ(context.LoadArgument(1), item);

    // Out of range
    EXPECT_THROW(context.LoadArgument(2), std::out_of_range);
    EXPECT_THROW(context.StoreArgument(2, item), std::out_of_range);
}

TEST(ExecutionContextTest, EvaluationStack)
{
    ByteVector bytes = ByteVector::Parse("0102030405");
    Script script(bytes);
    ExecutionContext context(script);

    // Push
    auto item1 = StackItem::Create(123);
    auto item2 = StackItem::Create(456);
    context.Push(item1);
    context.Push(item2);
    EXPECT_EQ(context.GetStackSize(), 2);

    // Peek
    EXPECT_EQ(context.Peek(), item2);
    EXPECT_EQ(context.Peek(1), item1);
    EXPECT_THROW(context.Peek(2), std::out_of_range);

    // Pop
    EXPECT_EQ(context.Pop(), item2);
    EXPECT_EQ(context.GetStackSize(), 1);
    EXPECT_EQ(context.Pop(), item1);
    EXPECT_EQ(context.GetStackSize(), 0);
    EXPECT_THROW(context.Pop(), std::runtime_error);

    // Clear
    context.Push(item1);
    context.Push(item2);
    EXPECT_EQ(context.GetStackSize(), 2);
    context.ClearStack();
    EXPECT_EQ(context.GetStackSize(), 0);
}

TEST(ExecutionContextTest, TryBlock)
{
    ByteVector bytes = ByteVector::Parse("0102030405");
    Script script(bytes);
    ExecutionContext context(script);

    // Enter try block
    context.EnterTry(10, 20, 30);
    EXPECT_EQ(context.GetTryCount(), 1);
    EXPECT_EQ(context.GetCatchOffset(), 10);
    EXPECT_EQ(context.GetFinallyOffset(), 20);
    EXPECT_EQ(context.GetEndOffset(), 30);

    // Enter nested try block
    context.EnterTry(40, 50, 60);
    EXPECT_EQ(context.GetTryCount(), 2);
    EXPECT_EQ(context.GetCatchOffset(), 40);
    EXPECT_EQ(context.GetFinallyOffset(), 50);
    EXPECT_EQ(context.GetEndOffset(), 60);

    // Exit try block
    context.ExitTry();
    EXPECT_EQ(context.GetTryCount(), 1);
    EXPECT_EQ(context.GetCatchOffset(), 10);
    EXPECT_EQ(context.GetFinallyOffset(), 20);
    EXPECT_EQ(context.GetEndOffset(), 30);

    // Exit try block
    context.ExitTry();
    EXPECT_EQ(context.GetTryCount(), 0);
    EXPECT_FALSE(context.GetCatchOffset().has_value());
    EXPECT_FALSE(context.GetFinallyOffset().has_value());
    EXPECT_FALSE(context.GetEndOffset().has_value());

    // Exit try block when not in a try block
    EXPECT_THROW(context.ExitTry(), std::runtime_error);
}

TEST(ExceptionHandlingContextTest, Constructor)
{
    ExceptionHandlingContext context(10, 20);

    EXPECT_EQ(context.GetCatchPointer(), 10);
    EXPECT_EQ(context.GetFinallyPointer(), 20);
    EXPECT_EQ(context.GetEndPointer(), -1);
    EXPECT_EQ(context.GetState(), ExceptionHandlingState::Try);
    EXPECT_TRUE(context.HasCatch());
    EXPECT_TRUE(context.HasFinally());
}

TEST(ExceptionHandlingContextTest, SetEndPointer)
{
    ExceptionHandlingContext context(10, 20);

    context.SetEndPointer(30);
    EXPECT_EQ(context.GetEndPointer(), 30);
}

TEST(ExceptionHandlingContextTest, SetState)
{
    ExceptionHandlingContext context(10, 20);

    context.SetState(ExceptionHandlingState::Catch);
    EXPECT_EQ(context.GetState(), ExceptionHandlingState::Catch);

    context.SetState(ExceptionHandlingState::Finally);
    EXPECT_EQ(context.GetState(), ExceptionHandlingState::Finally);
}

TEST(ExceptionHandlingContextTest, HasCatchAndFinally)
{
    ExceptionHandlingContext context1(10, 20);
    EXPECT_TRUE(context1.HasCatch());
    EXPECT_TRUE(context1.HasFinally());

    ExceptionHandlingContext context2(10, -1);
    EXPECT_TRUE(context2.HasCatch());
    EXPECT_FALSE(context2.HasFinally());

    ExceptionHandlingContext context3(-1, 20);
    EXPECT_FALSE(context3.HasCatch());
    EXPECT_TRUE(context3.HasFinally());

    ExceptionHandlingContext context4(-1, -1);
    EXPECT_FALSE(context4.HasCatch());
    EXPECT_FALSE(context4.HasFinally());
}

TEST(ExecutionContextTest, GetCurrentTry)
{
    ByteVector bytes = ByteVector::Parse("0102030405");
    Script script(bytes);
    ExecutionContext context(script);

    // Try to get current try when there is none
    EXPECT_THROW(context.GetCurrentTry(), InvalidOperationException);

    // Enter try block
    context.EnterTry(10, 20, 30);
    EXPECT_NO_THROW({
        const auto& tryContext = context.GetCurrentTry();
        EXPECT_EQ(tryContext.GetCatchPointer(), 10);
        EXPECT_EQ(tryContext.GetFinallyPointer(), 20);
        EXPECT_EQ(tryContext.GetEndPointer(), 30);
        EXPECT_EQ(tryContext.GetState(), ExceptionHandlingState::Try);
    });

    // Modify the try context
    auto& tryContext = context.GetCurrentTry();
    tryContext.SetState(ExceptionHandlingState::Catch);
    EXPECT_EQ(context.GetCurrentTry().GetState(), ExceptionHandlingState::Catch);
}

TEST(ExecutionEngineTest, UncaughtException)
{
    ExecutionEngine engine;

    // Initially, there should be no uncaught exception
    EXPECT_FALSE(engine.HasUncaughtException());
    EXPECT_EQ(engine.GetUncaughtException(), nullptr);

    // Set an uncaught exception
    auto exception = StackItem::Create("Test exception");
    engine.SetUncaughtException(exception);
    EXPECT_TRUE(engine.HasUncaughtException());
    EXPECT_EQ(engine.GetUncaughtException(), exception);

    // Clear the uncaught exception
    engine.ClearUncaughtException();
    EXPECT_FALSE(engine.HasUncaughtException());
    EXPECT_EQ(engine.GetUncaughtException(), nullptr);
}

TEST(ExecutionEngineTest, TryCatchFinally)
{
    // Create a script with a try-catch-finally block
    // TRY
    //   PUSH0
    //   THROW
    // CATCH
    //   PUSH1
    // FINALLY
    //   PUSH2
    // ENDFINALLY
    // PUSH3
    ByteVector bytes = ByteVector::Parse("0C0A0F0050F30D0551520F3F53");
    Script script(bytes);

    // Create an execution engine with a custom jump table
    JumpTable jumpTable;
    ExecutionEngine engine(jumpTable);

    // Load the script
    engine.LoadScript(script);

    // Execute the script
    VMState state = engine.Execute();

    // The script should halt successfully
    EXPECT_EQ(state, VMState::Halt);

    // The result stack should contain [3, 2, 1, 0]
    EXPECT_EQ(engine.GetResultStack().size(), 4);
    EXPECT_EQ(engine.GetResultStack()[0]->GetInteger(), 3);
    EXPECT_EQ(engine.GetResultStack()[1]->GetInteger(), 2);
    EXPECT_EQ(engine.GetResultStack()[2]->GetInteger(), 1);
    EXPECT_EQ(engine.GetResultStack()[3]->GetInteger(), 0);
}

TEST(ExecutionEngineTest, TryFinally)
{
    // Create a script with a try-finally block (no catch)
    // TRY
    //   PUSH0
    // FINALLY
    //   PUSH1
    // ENDFINALLY
    // PUSH2
    ByteVector bytes = ByteVector::Parse("0C000A50520F3F52");
    Script script(bytes);

    // Create an execution engine with a custom jump table
    JumpTable jumpTable;
    ExecutionEngine engine(jumpTable);

    // Load the script
    engine.LoadScript(script);

    // Execute the script
    VMState state = engine.Execute();

    // The script should halt successfully
    EXPECT_EQ(state, VMState::Halt);

    // The result stack should contain [2, 1, 0]
    EXPECT_EQ(engine.GetResultStack().size(), 3);
    EXPECT_EQ(engine.GetResultStack()[0]->GetInteger(), 2);
    EXPECT_EQ(engine.GetResultStack()[1]->GetInteger(), 1);
    EXPECT_EQ(engine.GetResultStack()[2]->GetInteger(), 0);
}

TEST(ExecutionEngineTest, TryCatch)
{
    // Create a script with a try-catch block (no finally)
    // TRY
    //   PUSH0
    //   THROW
    // CATCH
    //   PUSH1
    // ENDTRY
    // PUSH2
    ByteVector bytes = ByteVector::Parse("0C0A0050F30D0551520D0252");
    Script script(bytes);

    // Create an execution engine with a custom jump table
    JumpTable jumpTable;
    ExecutionEngine engine(jumpTable);

    // Load the script
    engine.LoadScript(script);

    // Execute the script
    VMState state = engine.Execute();

    // The script should halt successfully
    EXPECT_EQ(state, VMState::Halt);

    // The result stack should contain [2, 1, 0]
    EXPECT_EQ(engine.GetResultStack().size(), 3);
    EXPECT_EQ(engine.GetResultStack()[0]->GetInteger(), 2);
    EXPECT_EQ(engine.GetResultStack()[1]->GetInteger(), 1);
    EXPECT_EQ(engine.GetResultStack()[2]->GetInteger(), 0);
}

TEST(ExecutionEngineTest, NestedTryCatchFinally)
{
    // Create a script with nested try-catch-finally blocks
    // TRY
    //   PUSH0
    //   TRY
    //     PUSH1
    //     THROW
    //   CATCH
    //     PUSH2
    //   FINALLY
    //     PUSH3
    //   ENDFINALLY
    // CATCH
    //   PUSH4
    // FINALLY
    //   PUSH5
    // ENDFINALLY
    // PUSH6
    ByteVector bytes = ByteVector::Parse("0C1A2A5051F30D0552530F3F54550F3F56");
    Script script(bytes);

    // Create an execution engine with a custom jump table
    JumpTable jumpTable;
    ExecutionEngine engine(jumpTable);

    // Load the script
    engine.LoadScript(script);

    // Execute the script
    VMState state = engine.Execute();

    // The script should halt successfully
    EXPECT_EQ(state, VMState::Halt);

    // The result stack should contain [6, 5, 3, 2, 1, 0]
    EXPECT_EQ(engine.GetResultStack().size(), 6);
    EXPECT_EQ(engine.GetResultStack()[0]->GetInteger(), 6);
    EXPECT_EQ(engine.GetResultStack()[1]->GetInteger(), 5);
    EXPECT_EQ(engine.GetResultStack()[2]->GetInteger(), 3);
    EXPECT_EQ(engine.GetResultStack()[3]->GetInteger(), 2);
    EXPECT_EQ(engine.GetResultStack()[4]->GetInteger(), 1);
    EXPECT_EQ(engine.GetResultStack()[5]->GetInteger(), 0);
}

TEST(ExecutionEngineTest, UncaughtExceptionNoHandler)
{
    // Create a script that throws an exception without a try-catch-finally block
    // PUSH0
    // THROW
    ByteVector bytes = ByteVector::Parse("50F3");
    Script script(bytes);

    // Create an execution engine with a custom jump table
    JumpTable jumpTable;
    ExecutionEngine engine(jumpTable);

    // Load the script
    engine.LoadScript(script);

    // Execute the script
    VMState state = engine.Execute();

    // The script should fault due to the uncaught exception
    EXPECT_EQ(state, VMState::Fault);

    // The result stack should be empty
    EXPECT_EQ(engine.GetResultStack().size(), 0);
}

TEST(ExecutionEngineTest, ExceptionWithFinallyNoHandler)
{
    // Create a script with a try-finally block (no catch) that throws an exception
    // TRY
    //   PUSH0
    //   THROW
    // FINALLY
    //   PUSH1
    // ENDFINALLY
    // PUSH2
    ByteVector bytes = ByteVector::Parse("0C000A50F3520F3F52");
    Script script(bytes);

    // Create an execution engine with a custom jump table
    JumpTable jumpTable;
    ExecutionEngine engine(jumpTable);

    // Load the script
    engine.LoadScript(script);

    // Execute the script
    VMState state = engine.Execute();

    // The script should fault due to the uncaught exception
    EXPECT_EQ(state, VMState::Fault);

    // The result stack should contain only the value pushed in the finally block
    EXPECT_EQ(engine.GetResultStack().size(), 1);
    EXPECT_EQ(engine.GetResultStack()[0]->GetInteger(), 1);
}

TEST(ExecutionEngineTest, ExceptionInFinally)
{
    // Create a script with a try-finally block where an exception is thrown in the finally block
    // TRY
    //   PUSH0
    // FINALLY
    //   PUSH1
    //   THROW
    // ENDFINALLY
    // PUSH2
    ByteVector bytes = ByteVector::Parse("0C000A50520F51F3F352");
    Script script(bytes);

    // Create an execution engine with a custom jump table
    JumpTable jumpTable;
    ExecutionEngine engine(jumpTable);

    // Load the script
    engine.LoadScript(script);

    // Execute the script
    VMState state = engine.Execute();

    // The script should fault due to the uncaught exception
    EXPECT_EQ(state, VMState::Fault);

    // The result stack should contain the values pushed before the exception
    EXPECT_EQ(engine.GetResultStack().size(), 2);
    EXPECT_EQ(engine.GetResultStack()[0]->GetInteger(), 1);
    EXPECT_EQ(engine.GetResultStack()[1]->GetInteger(), 0);
}

TEST(ExecutionEngineTest, ExceptionInCatch)
{
    // Create a script with a try-catch-finally block where an exception is thrown in the catch block
    // TRY
    //   PUSH0
    //   THROW
    // CATCH
    //   PUSH1
    //   THROW
    // FINALLY
    //   PUSH2
    // ENDFINALLY
    // PUSH3
    ByteVector bytes = ByteVector::Parse("0C0A0F50F30D0551F3520F3F53");
    Script script(bytes);

    // Create an execution engine with a custom jump table
    JumpTable jumpTable;
    ExecutionEngine engine(jumpTable);

    // Load the script
    engine.LoadScript(script);

    // Execute the script
    VMState state = engine.Execute();

    // The script should fault due to the uncaught exception
    EXPECT_EQ(state, VMState::Fault);

    // The result stack should contain the values pushed before the exception
    EXPECT_EQ(engine.GetResultStack().size(), 2);
    EXPECT_EQ(engine.GetResultStack()[0]->GetInteger(), 2);
    EXPECT_EQ(engine.GetResultStack()[1]->GetInteger(), 1);
}

TEST(SystemCallTest, Constructor)
{
    std::string name = "System.Runtime.Log";
    auto handler = [](ExecutionEngine& engine) { return true; };
    SystemCall syscall(name, handler);

    EXPECT_EQ(syscall.GetName(), name);
    EXPECT_TRUE(syscall.GetHandler()(ExecutionEngine()));
}

TEST(ExecutionEngineTest, Constructor)
{
    ExecutionEngine engine;

    EXPECT_EQ(engine.GetState(), VMState::None);
    EXPECT_EQ(engine.GetResultStack().size(), 0);
    EXPECT_EQ(engine.GetInvocationStack().size(), 0);
    EXPECT_THROW(engine.GetCurrentContext(), std::runtime_error);
}

TEST(ExecutionEngineTest, LoadScript)
{
    ExecutionEngine engine;

    // Load script
    ByteVector bytes = ByteVector::Parse("0102030405");
    Script script(bytes);
    engine.LoadScript(script);

    EXPECT_EQ(engine.GetInvocationStack().size(), 1);
    EXPECT_EQ(engine.GetCurrentContext().GetScript().GetScript(), bytes);
    EXPECT_EQ(engine.GetCurrentContext().GetInstructionPointer(), 0);

    // Load script with initial position
    engine = ExecutionEngine();
    engine.LoadScript(script, 3);

    EXPECT_EQ(engine.GetInvocationStack().size(), 1);
    EXPECT_EQ(engine.GetCurrentContext().GetScript().GetScript(), bytes);
    EXPECT_EQ(engine.GetCurrentContext().GetInstructionPointer(), 3);

    // Load script with configure context
    engine = ExecutionEngine();
    engine.LoadScript(script, 0, [](ExecutionContext& context) {
        context.InitializeStaticFields(3);
        context.InitializeLocalVariables(2, 1);
        context.Push(StackItem::Create(123));
    });

    EXPECT_EQ(engine.GetInvocationStack().size(), 1);
    EXPECT_EQ(engine.GetCurrentContext().GetScript().GetScript(), bytes);
    EXPECT_EQ(engine.GetCurrentContext().GetInstructionPointer(), 0);
    EXPECT_EQ(engine.GetCurrentContext().GetStaticFields().size(), 3);
    EXPECT_EQ(engine.GetCurrentContext().GetLocalVariables().size(), 2);
    EXPECT_EQ(engine.GetCurrentContext().GetArguments().size(), 1);
    EXPECT_EQ(engine.GetCurrentContext().GetStackSize(), 1);
    EXPECT_EQ(engine.GetCurrentContext().Peek()->GetInteger(), 123);
}

TEST(ExecutionEngineTest, Execute)
{
    ExecutionEngine engine;

    // Empty invocation stack
    EXPECT_EQ(engine.Execute(), VMState::None);

    // Simple script
    ByteVector bytes = ByteVector::Parse("0051"); // PUSH0, PUSH1
    Script script(bytes);
    engine.LoadScript(script);

    EXPECT_EQ(engine.Execute(), VMState::Halt);
    EXPECT_EQ(engine.GetResultStack().size(), 2);
    EXPECT_EQ(engine.GetResultStack()[0]->GetBoolean(), false);
    EXPECT_EQ(engine.GetResultStack()[1]->GetInteger(), 1);

    // Script with arithmetic operations
    bytes = ByteVector::Parse("515293"); // PUSH1, PUSH2, ADD
    script = Script(bytes);
    engine = ExecutionEngine();
    engine.LoadScript(script);

    EXPECT_EQ(engine.Execute(), VMState::Halt);
    EXPECT_EQ(engine.GetResultStack().size(), 1);
    EXPECT_EQ(engine.GetResultStack()[0]->GetInteger(), 3);

    // Script with comparison operations
    bytes = ByteVector::Parse("515294"); // PUSH1, PUSH2, LT
    script = Script(bytes);
    engine = ExecutionEngine();
    engine.LoadScript(script);

    EXPECT_EQ(engine.Execute(), VMState::Halt);
    EXPECT_EQ(engine.GetResultStack().size(), 1);
    EXPECT_EQ(engine.GetResultStack()[0]->GetBoolean(), true);

    // Script with logical operations
    bytes = ByteVector::Parse("5152DF"); // PUSH1, PUSH2, BOOLAND
    script = Script(bytes);
    engine = ExecutionEngine();
    engine.LoadScript(script);

    EXPECT_EQ(engine.Execute(), VMState::Halt);
    EXPECT_EQ(engine.GetResultStack().size(), 1);
    EXPECT_EQ(engine.GetResultStack()[0]->GetBoolean(), true);

    // Script with array operations
    bytes = ByteVector::Parse("51EC5152F8"); // PUSH1, NEWARRAY0, PUSH1, PUSH2, SETITEM
    script = Script(bytes);
    engine = ExecutionEngine();
    engine.LoadScript(script);

    EXPECT_EQ(engine.Execute(), VMState::Halt);
    EXPECT_EQ(engine.GetResultStack().size(), 0);
}

TEST(ExecutionEngineTest, SystemCall)
{
    ExecutionEngine engine;

    // Register system call
    engine.RegisterSystemCall("System.Runtime.Log", [](ExecutionEngine& engine) {
        auto message = engine.GetCurrentContext().Pop();
        // Log the message (in tests, we just verify it doesn't crash)
        return true;
    });

    // Script with system call
    ByteVector bytes = ByteVector::Parse("0051800000000000000000000000000000"); // PUSH0, PUSH1, SYSCALL (hash)
    Script script(bytes);
    engine.LoadScript(script);

    // Execute
    EXPECT_EQ(engine.Execute(), VMState::Halt);
}
