#pragma once

#include <neo/vm/stack_item.h>
#include <neo/vm/script.h>
#include <neo/vm/jump_table.h>
#include <neo/vm/reference_counter.h>
#include <neo/vm/execution_engine_limits.h>
#include <neo/vm/execution_context.h>
#include <neo/vm/internal/byte_vector.h>
#include <neo/vm/vm_state.h>
#include <functional>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <optional>

namespace neo::vm
{
    // Forward declaration
    class ExecutionContext;

    /**
     * @brief Represents a system call.
     */
    class SystemCall
    {
    public:
        /**
         * @brief Default constructor.
         */
        SystemCall() = default;

        /**
         * @brief Constructs a SystemCall.
         * @param name The name.
         * @param handler The handler.
         */
        SystemCall(const std::string& name, std::function<bool(ExecutionEngine&)> handler);

        /**
         * @brief Gets the name.
         * @return The name.
         */
        const std::string& GetName() const;

        /**
         * @brief Gets the handler.
         * @return The handler.
         */
        const std::function<bool(ExecutionEngine&)>& GetHandler() const;

    private:
        std::string name_;
        std::function<bool(ExecutionEngine&)> handler_;
    };

    /**
     * @brief Represents an execution engine.
     */
    class ExecutionEngine
    {
    public:
        /**
         * @brief Constructs an ExecutionEngine with default settings.
         */
        ExecutionEngine();

        /**
         * @brief Constructs an ExecutionEngine with a custom jump table.
         * @param jumpTable The jump table.
         */
        explicit ExecutionEngine(const JumpTable& jumpTable);

        /**
         * @brief Constructs an ExecutionEngine with custom limits.
         * @param limits The execution engine limits.
         */
        explicit ExecutionEngine(const ExecutionEngineLimits& limits);

        /**
         * @brief Constructs an ExecutionEngine with a custom jump table and limits.
         * @param jumpTable The jump table.
         * @param limits The execution engine limits.
         */
        ExecutionEngine(const JumpTable& jumpTable, const ExecutionEngineLimits& limits);

        /**
         * @brief Virtual destructor to properly clean up resources.
         */
        virtual ~ExecutionEngine();

        /**
         * @brief Gets the limits of the execution engine.
         * @return The limits.
         */
        const ExecutionEngineLimits& GetLimits() const;

        /**
         * @brief Gets the current context.
         * @return The current context.
         */
        ExecutionContext& GetCurrentContext();

        /**
         * @brief Gets the current context.
         * @return The current context.
         */
        const ExecutionContext& GetCurrentContext() const;

        /**
         * @brief Gets the entry context.
         * @return The entry context.
         */
        std::shared_ptr<ExecutionContext> GetEntryContext() const;

        /**
         * @brief Gets the invocation stack.
         * @return The invocation stack.
         */
        const std::vector<std::shared_ptr<ExecutionContext>>& GetInvocationStack() const;

        /**
         * @brief Gets the state.
         * @return The state.
         */
        VMState GetState() const;

        /**
         * @brief Sets the VM state.
         * @param state The state.
         */
        void SetState(VMState state);

        /**
         * @brief Gets the result stack.
         * @return The result stack.
         */
        const std::vector<std::shared_ptr<StackItem>>& GetResultStack() const;

        /**
         * @brief Loads a script.
         * @param script The script.
         * @param initialPosition The initial position.
         * @param configureContext The configure context function.
         * @return The loaded execution context.
         */
        std::shared_ptr<ExecutionContext> LoadScript(const Script& script, int32_t initialPosition = 0, std::function<void(ExecutionContext&)> configureContext = nullptr);

        /**
         * @brief Loads a context.
         * @param context The context.
         */
        void LoadContext(ExecutionContext& context);

        /**
         * @brief Executes the script.
         * @param gasLimit The gas limit.
         * @return The state.
         */
        VMState Execute(int64_t gasLimit = -1);

        /**
         * @brief Executes the next instruction.
         */
        void ExecuteNext();

        /**
         * @brief Registers a system call.
         * @param name The name.
         * @param handler The handler.
         */
        void RegisterSystemCall(const std::string& name, std::function<bool(ExecutionEngine&)> handler);

        /**
         * @brief Gets the system call.
         * @param hash The hash.
         * @return The system call.
         */
        const SystemCall& GetSystemCall(uint32_t hash) const;

        /**
         * @brief Pushes an item onto the evaluation stack.
         * @param item The item.
         */
        void Push(std::shared_ptr<StackItem> item);

        /**
         * @brief Pops an item from the evaluation stack.
         * @return The item.
         */
        std::shared_ptr<StackItem> Pop();

        /**
         * @brief Pops an item from the evaluation stack and converts it to the specified type.
         * @tparam T The type to convert to.
         * @return The item as the specified type.
         */
        template<typename T>
        std::shared_ptr<T> Pop()
        {
            auto item = Pop();
            auto typed = std::dynamic_pointer_cast<T>(item);
            if (!typed)
                throw std::runtime_error("Invalid cast");
            return typed;
        }

        /**
         * @brief Peeks at an item on the evaluation stack.
         * @param index The index.
         * @return The item.
         */
        std::shared_ptr<StackItem> Peek(int32_t index = 0) const;

        /**
         * @brief Sets the jumping flag.
         * @param jumping The jumping flag.
         */
        void SetJumping(bool jumping);

        /**
         * @brief Gets the jumping flag.
         * @return The jumping flag.
         */
        bool IsJumping() const;

        /**
         * @brief Gets the reference counter.
         * @return The reference counter.
         */
        ReferenceCounter* GetReferenceCounter();

        /**
         * @brief Checks if there is an uncaught exception.
         * @return True if there is an uncaught exception, false otherwise.
         */
        bool HasUncaughtException() const;

        /**
         * @brief Gets the uncaught exception.
         * @return The uncaught exception.
         */
        std::shared_ptr<StackItem> GetUncaughtException() const;

        /**
         * @brief Clears the uncaught exception.
         */
        void ClearUncaughtException();

        /**
         * @brief Sets the uncaught exception.
         * @param exception The exception.
         */
        void SetUncaughtException(std::shared_ptr<StackItem> exception);

        /**
         * @brief Executes a return operation.
         * @return True if successful, false otherwise.
         */
        bool ExecuteRet();

        /**
         * @brief Executes a system call.
         * @param hash The hash of the system call.
         * @return True if successful, false otherwise.
         */
        bool ExecuteSysCall(uint32_t hash);

        /**
         * @brief Executes a call operation.
         * @param position The position to call.
         * @return True if successful, false otherwise.
         */
        bool ExecuteCall(int32_t position);

        /**
         * @brief Executes a jump operation.
         * @param position The position to jump to.
         * @return True if successful, false otherwise.
         */
        bool ExecuteJump(int32_t position);

        /**
         * @brief Executes a throw operation.
         * @param exception The exception to throw.
         * @return True if successful, false otherwise.
         */
        bool ExecuteThrow(std::shared_ptr<StackItem> exception);

        /**
         * @brief Executes a try operation.
         * @param catchPosition The catch position.
         * @param finallyPosition The finally position.
         * @return True if successful, false otherwise.
         */
        bool ExecuteTry(int32_t catchPosition, int32_t finallyPosition);

        /**
         * @brief Executes an end try operation.
         * @param position The position.
         * @return True if successful, false otherwise.
         */
        bool ExecuteEndTry(int32_t position);

        /**
         * @brief Unloads a context from the invocation stack.
         * @param context The context to unload.
         */
        void UnloadContext(ExecutionContext& context);

    protected:
        /**
         * @brief Creates a new context with the specified script without loading.
         * @param script The script.
         * @param rvcount The number of values that the context should return when it is unloaded.
         * @param initialPosition The initial position.
         * @return The created context.
         */
        std::shared_ptr<ExecutionContext> CreateContext(const Script& script, int32_t rvcount, int32_t initialPosition);

        /**
         * @brief Called before an instruction is executed.
         * @param instruction The instruction.
         */
        virtual void PreExecuteInstruction(const Instruction& instruction);

        /**
         * @brief Called after an instruction is executed.
         * @param instruction The instruction.
         */
        virtual void PostExecuteInstruction(const Instruction& instruction);

        /**
         * @brief Called when a fault occurs.
         * @param ex The exception pointer.
         */
        virtual void OnFault(std::exception_ptr ex);

        /**
         * @brief Called when a fault occurs with the current exception.
         */
        virtual void OnFault();

        /**
         * @brief Called when the state changes.
         */
        virtual void OnStateChanged();

    private:
        std::vector<std::shared_ptr<ExecutionContext>> invocationStack_;
        VMState state_;
        std::vector<std::shared_ptr<StackItem>> resultStack_;
        std::unordered_map<uint32_t, SystemCall> systemCalls_;
        const JumpTable& jumpTable_;
        bool jumping_;
        ReferenceCounter referenceCounter_;
        std::shared_ptr<StackItem> uncaughtException_;
        ExecutionEngineLimits limits_;
        std::shared_ptr<ExecutionContext> entryContext_;
    };
}
