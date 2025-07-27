#include <neo/vm/compound_items.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_control.h>
#include <neo/vm/jump_table_control_exception.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/special_items.h>
#include <neo/vm/stack_item.h>

namespace neo::vm
{
// JumpTable delegates to JumpTableControlException
void JumpTable::LEAVE(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableControlException::LEAVE(engine, instruction);
}

void JumpTable::LEAVE_L(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableControlException::LEAVE_L(engine, instruction);
}

void JumpTable::ABORT(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableControlException::ABORT(engine, instruction);
}

void JumpTable::ASSERT(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableControlException::ASSERT(engine, instruction);
}

void JumpTable::THROW(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableControlException::THROW(engine, instruction);
}

void JumpTable::TRY(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableControlException::TRY(engine, instruction);
}

void JumpTable::TRY_L(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableControlException::TRY_L(engine, instruction);
}

void JumpTable::ENDTRY(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableControlException::ENDTRY(engine, instruction);
}

void JumpTable::ENDTRY_L(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableControlException::ENDTRY_L(engine, instruction);
}

void JumpTable::ENDFINALLY(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableControlException::ENDFINALLY(engine, instruction);
}

// JumpTableControlException implementations
void JumpTableControlException::LEAVE(ExecutionEngine& engine, const Instruction& instruction)
{
    // Get the current context
    auto& context = engine.GetCurrentContext();

    // Ensure we're in a try block
    if (context.GetTryCount() == 0)
        throw InvalidOperationException("The corresponding TRY block cannot be found");

    // Get the offset from the instruction
    int8_t offset = instruction.TokenI8();

    // Get the end position - we're jumping out of the try block
    int32_t position = context.GetInstructionPointer() + offset;

    // Keep track of finally blocks to execute
    std::vector<int32_t> finallyPositions;

    // Exit try blocks and collect finally positions
    while (context.GetTryCount() > 0)
    {
        auto& tryContext = context.GetCurrentTry();

        // If there's a finally block, add it to the list
        if (tryContext.HasFinally())
        {
            finallyPositions.push_back(context.GetCurrentPosition() + tryContext.GetFinallyPointer());
        }

        // Exit the try block
        context.ExitTry();
    }

    // Execute all finally blocks in reverse order (innermost first)
    for (auto it = finallyPositions.rbegin(); it != finallyPositions.rend(); ++it)
    {
        // Set the instruction pointer to the finally block
        context.SetInstructionPointer(*it);

        // Execute until the end of the finally block
        while (context.GetCurrentPosition() < position && engine.GetState() != VMState::Fault)
        {
            engine.ExecuteNext();
        }
    }

    // Jump to the target position
    context.SetInstructionPointer(position);
    engine.SetJumping(true);
}

void JumpTableControlException::LEAVE_L(ExecutionEngine& engine, const Instruction& instruction)
{
    // Get the current context
    auto& context = engine.GetCurrentContext();

    // Ensure we're in a try block
    if (context.GetTryCount() == 0)
        throw InvalidOperationException("The corresponding TRY block cannot be found");

    // Get the offset from the instruction
    int32_t offset = instruction.TokenI32();

    // Get the end position - we're jumping out of the try block
    int32_t position = context.GetInstructionPointer() + offset;

    // Keep track of finally blocks to execute
    std::vector<int32_t> finallyPositions;

    // Exit try blocks and collect finally positions
    while (context.GetTryCount() > 0)
    {
        auto& tryContext = context.GetCurrentTry();

        // If there's a finally block, add it to the list
        if (tryContext.HasFinally())
        {
            finallyPositions.push_back(context.GetCurrentPosition() + tryContext.GetFinallyPointer());
        }

        // Exit the try block
        context.ExitTry();
    }

    // Execute all finally blocks in reverse order (innermost first)
    for (auto it = finallyPositions.rbegin(); it != finallyPositions.rend(); ++it)
    {
        // Set the instruction pointer to the finally block
        context.SetInstructionPointer(*it);

        // Execute until the end of the finally block
        while (context.GetCurrentPosition() < position && engine.GetState() != VMState::Fault)
        {
            engine.ExecuteNext();
        }
    }

    // Jump to the target position
    context.SetInstructionPointer(position);
    engine.SetJumping(true);
}

void JumpTableControlException::ABORT(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    throw InvalidOperationException("ABORT: Execution aborted");
}

void JumpTableControlException::ASSERT(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    if (!engine.Pop()->GetBoolean())
        throw InvalidOperationException("ASSERT: Assertion failed");
}

void JumpTableControlException::THROW(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto exception = engine.Pop();
    engine.ExecuteThrow(exception);
}

void JumpTableControlException::TRY(ExecutionEngine& engine, const Instruction& instruction)
{
    int8_t catchOffset = instruction.TokenI8();
    int8_t finallyOffset = instruction.TokenI8_1();
    engine.ExecuteTry(catchOffset, finallyOffset);
}

void JumpTableControlException::TRY_L(ExecutionEngine& engine, const Instruction& instruction)
{
    int32_t catchOffset = instruction.TokenI32();
    int32_t finallyOffset = instruction.TokenI32_1();
    engine.ExecuteTry(catchOffset, finallyOffset);
}

void JumpTableControlException::ENDTRY(ExecutionEngine& engine, const Instruction& instruction)
{
    int8_t endOffset = instruction.TokenI8();
    engine.ExecuteEndTry(endOffset);
}

void JumpTableControlException::ENDTRY_L(ExecutionEngine& engine, const Instruction& instruction)
{
    int32_t endOffset = instruction.TokenI32();
    engine.ExecuteEndTry(endOffset);
}

void JumpTableControlException::ENDFINALLY(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    if (context.GetTryCount() == 0)
        throw InvalidOperationException("The corresponding TRY block cannot be found");

    // Get the current try context
    auto& tryContext = context.GetCurrentTry();

    // Verify that we're in a finally block
    if (tryContext.GetState() != ExceptionHandlingState::Finally)
        throw InvalidOperationException("ENDFINALLY can only be used in a FINALLY block");

    // If there's an uncaught exception, rethrow it
    if (engine.HasUncaughtException())
    {
        auto exception = engine.GetUncaughtException();
        engine.ClearUncaughtException();
        engine.ExecuteThrow(exception);
    }
    else
    {
        // Otherwise, jump to the end of the try block
        context.ExitTry();
        context.SetInstructionPointer(tryContext.GetEndPointer());
        engine.SetJumping(true);
    }
}
}  // namespace neo::vm
