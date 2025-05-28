#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_exception.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/primitive_items.h>

namespace neo::vm
{
    // JumpTable delegates to JumpTableException
    void JumpTable::ExecuteThrow(ExecutionEngine& engine, const std::string& message)
    {
        JumpTableException::ExecuteThrow(engine, message);
    }

    void JumpTable::ExecuteThrow(ExecutionEngine& engine, std::shared_ptr<StackItem> exception)
    {
        JumpTableException::ExecuteThrow(engine, exception);
    }

    // JumpTableException implementations
    void JumpTableException::ExecuteThrow(ExecutionEngine& engine, const std::string& message)
    {
        auto& context = engine.GetCurrentContext();
        if (context.GetTryCount() == 0)
        {
            // No try block to catch the exception
            throw CatchableException(message);
        }

        // Create a string stack item for the exception
        auto exception = StackItem::Create(message);
        ExecuteThrow(engine, exception);
    }

    void JumpTableException::ExecuteThrow(ExecutionEngine& engine, std::shared_ptr<StackItem> exception)
    {
        auto& context = engine.GetCurrentContext();
        if (context.GetTryCount() == 0)
        {
            // No try block to catch the exception
            throw CatchableException("Uncaught exception");
        }

        // Get the catch offset
        auto catchOffset = context.GetCatchOffset();
        auto finallyOffset = context.GetFinallyOffset();

        // Push the exception to the stack
        engine.Push(exception);

        // Jump to the catch block if it exists
        if (catchOffset.has_value())
        {
            // Set the state of the try context to Catch
            auto& tryContext = context.GetCurrentTry();
            tryContext.SetState(ExceptionHandlingState::Catch);

            context.SetInstructionPointer(catchOffset.value());
            engine.SetJumping(true);
        }
        // Jump to the finally block if it exists
        else if (finallyOffset.has_value())
        {
            // Set the state of the try context to Finally
            auto& tryContext = context.GetCurrentTry();
            tryContext.SetState(ExceptionHandlingState::Finally);

            // Store the exception as an uncaught exception
            engine.SetUncaughtException(exception);

            context.SetInstructionPointer(finallyOffset.value());
            engine.SetJumping(true);
        }
        // No catch or finally block, rethrow the exception
        else
        {
            context.ExitTry();
            throw CatchableException("Uncaught exception");
        }
    }
}
