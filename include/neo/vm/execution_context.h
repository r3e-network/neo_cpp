/**
 * @file execution_context.h
 * @brief Execution Context
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/vm/exception_handling_context.h>
#include <neo/vm/script.h>
#include <neo/vm/stack_item.h>

#include <any>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace neo::vm
{

/**
 * @brief Represents a frame in the VM execution stack.
 */
class ExecutionContext
{
   public:
    /**
     * @brief Constructs a new ExecutionContext.
     * @param script The script.
     * @param rvcount The number of values that the context should return when it is unloaded.
     */
    ExecutionContext(const Script& script, int32_t rvcount = -1);

    /**
     * @brief Gets the script.
     * @return The script.
     */
    const Script& GetScript() const;

    /**
     * @brief Gets the instruction pointer.
     * @return The instruction pointer.
     */
    int32_t GetInstructionPointer() const;

    /**
     * @brief Sets the instruction pointer.
     * @param instructionPointer The instruction pointer.
     */
    void SetInstructionPointer(int32_t instructionPointer);

    /**
     * @brief Gets the current position.
     * @return The current position.
     */
    int32_t GetCurrentPosition() const;

    /**
     * @brief Gets the next instruction opcode.
     * @return The next instruction opcode.
     */
    OpCode GetNextInstructionOpCode() const;

    /**
     * @brief Gets the next instruction object.
     * @return The next instruction object.
     */
    std::shared_ptr<Instruction> GetNextInstructionObject() const;

    /**
     * @brief Moves to the next instruction.
     */
    void MoveNext();

    /**
     * @brief Gets the current instruction.
     * @return The current instruction.
     */
    std::shared_ptr<Instruction> GetCurrentInstruction() const;

    /**
     * @brief Gets the RVCount.
     * @return The RVCount.
     */
    int32_t GetRVCount() const;

    /**
     * @brief Gets the static fields.
     * @return The static fields.
     */
    const std::vector<std::shared_ptr<StackItem>>& GetStaticFields() const;

    /**
     * @brief Gets the local variables.
     * @return The local variables.
     */
    const std::vector<std::shared_ptr<StackItem>>& GetLocalVariables() const;

    /**
     * @brief Gets the arguments.
     * @return The arguments.
     */
    const std::vector<std::shared_ptr<StackItem>>& GetArguments() const;

    /**
     * @brief Gets the evaluation stack.
     * @return The evaluation stack.
     */
    const std::vector<std::shared_ptr<StackItem>>& GetEvaluationStack() const;

    /**
     * @brief Provides mutable access to the evaluation stack.
     * @return Reference to the underlying evaluation stack.
     */
    std::vector<std::shared_ptr<StackItem>>& GetEvaluationStackMutable();

    /**
     * @brief Gets the try count.
     * @return The try count.
     */
    int32_t GetTryCount() const;

    /**
     * @brief Gets a state object of the specified type.
     * @tparam T The type of the state object.
     * @param factory A factory function to create the state object if it doesn't exist.
     * @return The state object.
     */
    template <typename T>
    std::shared_ptr<T> GetState(std::function<std::shared_ptr<T>()> factory)
    {
        std::type_index type = std::type_index(typeid(T));

        // Check if state exists for this type
        auto it = states_.find(type);
        if (it != states_.end())
        {
            // Return existing state
            return std::any_cast<std::shared_ptr<T>>(it->second);
        }

        // Create new state
        auto state = factory();
        states_[type] = state;
        return state;
    }

    /**
     * @brief Gets a state object of the specified type.
     * @tparam T The type of the state object.
     * @return The state object.
     */
    template <typename T>
    std::shared_ptr<T> GetState()
    {
        return GetState<T>([]() { return std::make_shared<T>(); });
    }

    /**
     * @brief Initializes the static fields.
     * @param count The count.
     */
    void InitializeStaticFields(int32_t count);

    /**
     * @brief Gets a static field.
     * @param index The index.
     * @return The static field.
     */
    std::shared_ptr<StackItem> GetStaticField(int32_t index) const;

    /**
     * @brief Sets a static field.
     * @param index The index.
     * @param item The item.
     */
    void SetStaticField(int32_t index, std::shared_ptr<StackItem> item);

    /**
     * @brief Initializes the local variables.
     * @param count The count.
     */
    void InitializeLocalVariables(int32_t count);

    /**
     * @brief Sets the local variables.
     * @param variables The local variables.
     */
    void SetLocalVariables(const std::vector<std::shared_ptr<StackItem>>& variables);

    /**
     * @brief Sets the arguments.
     * @param arguments The arguments.
     */
    void SetArguments(const std::vector<std::shared_ptr<StackItem>>& arguments);

    /**
     * @brief Initializes the local variables and arguments.
     * @param localCount The local variable count.
     * @param argumentCount The argument count.
     */
    void InitializeLocalVariables(int32_t localCount, int32_t argumentCount);

    /**
     * @brief Initializes the arguments.
     * @param count The count.
     */
    void InitializeArguments(int32_t count);

    /**
     * @brief Loads a local variable.
     * @param index The index.
     * @return The local variable.
     */
    std::shared_ptr<StackItem> LoadLocalVariable(int32_t index) const;

    /**
     * @brief Stores a local variable.
     * @param index The index.
     * @param item The item.
     */
    void StoreLocalVariable(int32_t index, std::shared_ptr<StackItem> item);

    /**
     * @brief Loads an argument.
     * @param index The index.
     * @return The argument.
     */
    std::shared_ptr<StackItem> LoadArgument(int32_t index) const;

    /**
     * @brief Stores an argument.
     * @param index The index.
     * @param item The item.
     */
    void StoreArgument(int32_t index, std::shared_ptr<StackItem> item);

    /**
     * @brief Loads a static field.
     * @param index The index.
     * @return The static field.
     */
    std::shared_ptr<StackItem> LoadStaticField(int32_t index) const;

    /**
     * @brief Stores a static field.
     * @param index The index.
     * @param item The item.
     */
    void StoreStaticField(int32_t index, std::shared_ptr<StackItem> item);

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
     * @brief Peeks at an item on the evaluation stack.
     * @param index The index.
     * @return The item.
     */
    std::shared_ptr<StackItem> Peek(int32_t index = 0) const;

    /**
     * @brief Clears the evaluation stack.
     */
    void ClearStack();

    /**
     * @brief Gets the stack size.
     * @return The stack size.
     */
    int32_t GetStackSize() const;

    /**
     * @brief Enters a try block.
     * @param catchOffset The catch offset.
     * @param finallyOffset The finally offset.
     * @param endOffset The end offset.
     */
    void EnterTry(int32_t catchOffset, int32_t finallyOffset, int32_t endOffset);

    /**
     * @brief Exits a try block.
     */
    void ExitTry();

    /**
     * @brief Gets the catch offset.
     * @return The catch offset.
     */
    std::optional<int32_t> GetCatchOffset() const;

    /**
     * @brief Gets the finally offset.
     * @return The finally offset.
     */
    std::optional<int32_t> GetFinallyOffset() const;

    /**
     * @brief Gets the end offset.
     * @return The end offset.
     */
    std::optional<int32_t> GetEndOffset() const;

    /**
     * @brief Gets the current try context.
     * @return The current try context.
     */
    ExceptionHandlingContext& GetCurrentTry();

    /**
     * @brief Gets the current try context.
     * @return The current try context.
     */
    const ExceptionHandlingContext& GetCurrentTry() const;

    /**
     * @brief Sets the try state.
     * @param catchPosition The catch position.
     * @param finallyPosition The finally position.
     */
    void SetTryState(int32_t catchPosition, int32_t finallyPosition);

    /**
     * @brief Clears the try state.
     */
    void ClearTryState();

    /**
     * @brief Clones the context.
     * @param initialPosition The initial position.
     * @return The cloned context.
     */
    std::shared_ptr<ExecutionContext> Clone(int32_t initialPosition = -1) const;

    /**
     * @brief Gets the call table.
     * @return The call table.
     */
    const std::vector<int32_t>& GetCallTable() const;

   private:
    Script script_;
    int32_t instructionPointer_;
    int32_t rvcount_;
    std::vector<std::shared_ptr<StackItem>> staticFields_;
    std::vector<std::shared_ptr<StackItem>> localVariables_;
    std::vector<std::shared_ptr<StackItem>> arguments_;
    std::vector<std::shared_ptr<StackItem>> evaluationStack_;
    std::vector<ExceptionHandlingContext> tryStack_;
    std::vector<int32_t> callTable_;
    std::unordered_map<std::type_index, std::any> states_;
};
}  // namespace neo::vm
