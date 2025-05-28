#include <neo/vm/execution_context.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/exception_handling_context.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/special_items.h>
#include <stdexcept>

namespace neo::vm
{
    // ExecutionContext implementation
    ExecutionContext::ExecutionContext(const Script& script, int32_t rvcount)
        : script_(script), instructionPointer_(0), rvcount_(rvcount)
    {
        if (rvcount < -1 || rvcount > 65535) // ushort.MaxValue in C#
            throw std::invalid_argument("rvcount out of range");
    }

    const Script& ExecutionContext::GetScript() const
    {
        return script_;
    }

    int32_t ExecutionContext::GetInstructionPointer() const
    {
        return instructionPointer_;
    }

    void ExecutionContext::SetInstructionPointer(int32_t instructionPointer)
    {
        if (instructionPointer < 0 || instructionPointer > static_cast<int32_t>(script_.GetLength()))
            throw std::out_of_range("Instruction pointer out of range");

        instructionPointer_ = instructionPointer;
    }

    OpCode ExecutionContext::GetNextInstructionOpCode() const
    {
        if (instructionPointer_ >= static_cast<int32_t>(script_.GetLength()))
            return OpCode::RET;

        return static_cast<OpCode>(script_.GetScript()[instructionPointer_]);
    }

    std::shared_ptr<Instruction> ExecutionContext::GetCurrentInstruction() const
    {
        if (instructionPointer_ >= static_cast<int32_t>(script_.GetLength()))
            return nullptr;

        return script_.GetInstruction(instructionPointer_);
    }

    std::shared_ptr<Instruction> ExecutionContext::GetNextInstructionObject() const
    {
        auto current = GetCurrentInstruction();
        if (!current)
            return nullptr;

        return script_.GetInstruction(instructionPointer_ + current->Size());
    }

    void ExecutionContext::MoveNext()
    {
        auto instruction = GetCurrentInstruction();
        if (instruction)
            instructionPointer_ += instruction->Size();
    }

    int32_t ExecutionContext::GetRVCount() const
    {
        return rvcount_;
    }

    int32_t ExecutionContext::GetCurrentPosition() const
    {
        return instructionPointer_;
    }

    const std::vector<std::shared_ptr<StackItem>>& ExecutionContext::GetStaticFields() const
    {
        return staticFields_;
    }

    const std::vector<std::shared_ptr<StackItem>>& ExecutionContext::GetLocalVariables() const
    {
        return localVariables_;
    }

    const std::vector<std::shared_ptr<StackItem>>& ExecutionContext::GetArguments() const
    {
        return arguments_;
    }

    const std::vector<std::shared_ptr<StackItem>>& ExecutionContext::GetEvaluationStack() const
    {
        return evaluationStack_;
    }

    int32_t ExecutionContext::GetTryCount() const
    {
        return static_cast<int32_t>(tryStack_.size());
    }



    void ExecutionContext::InitializeStaticFields(int32_t count)
    {
        if (count < 0)
            throw std::invalid_argument("Count cannot be negative");

        staticFields_.resize(count);
    }

    std::shared_ptr<StackItem> ExecutionContext::GetStaticField(int32_t index) const
    {
        if (index < 0 || index >= static_cast<int32_t>(staticFields_.size()))
            throw std::out_of_range("Static field index out of range");

        return staticFields_[index];
    }

    void ExecutionContext::SetStaticField(int32_t index, std::shared_ptr<StackItem> item)
    {
        if (index < 0 || index >= static_cast<int32_t>(staticFields_.size()))
            throw std::out_of_range("Static field index out of range");

        staticFields_[index] = item;
    }

    void ExecutionContext::InitializeLocalVariables(int32_t count)
    {
        if (count < 0)
            throw std::invalid_argument("Count cannot be negative");

        localVariables_.resize(count);
    }

    void ExecutionContext::SetLocalVariables(const std::vector<std::shared_ptr<StackItem>>& variables)
    {
        localVariables_ = variables;
    }

    void ExecutionContext::SetArguments(const std::vector<std::shared_ptr<StackItem>>& arguments)
    {
        arguments_ = arguments;
    }

    void ExecutionContext::InitializeLocalVariables(int32_t localCount, int32_t argumentCount)
    {
        InitializeLocalVariables(localCount);
        InitializeArguments(argumentCount);
    }

    void ExecutionContext::InitializeArguments(int32_t count)
    {
        if (count < 0)
            throw std::invalid_argument("Count cannot be negative");

        arguments_.resize(count);
    }

    std::shared_ptr<StackItem> ExecutionContext::LoadLocalVariable(int32_t index) const
    {
        if (index < 0 || index >= static_cast<int32_t>(localVariables_.size()))
            throw std::out_of_range("Local variable index out of range");

        return localVariables_[index];
    }

    void ExecutionContext::StoreLocalVariable(int32_t index, std::shared_ptr<StackItem> item)
    {
        if (index < 0 || index >= static_cast<int32_t>(localVariables_.size()))
            throw std::out_of_range("Local variable index out of range");

        localVariables_[index] = item;
    }

    std::shared_ptr<StackItem> ExecutionContext::LoadArgument(int32_t index) const
    {
        if (index < 0 || index >= static_cast<int32_t>(arguments_.size()))
            throw std::out_of_range("Argument index out of range");

        return arguments_[index];
    }

    void ExecutionContext::StoreArgument(int32_t index, std::shared_ptr<StackItem> item)
    {
        if (index < 0 || index >= static_cast<int32_t>(arguments_.size()))
            throw std::out_of_range("Argument index out of range");

        arguments_[index] = item;
    }

    std::shared_ptr<StackItem> ExecutionContext::LoadStaticField(int32_t index) const
    {
        if (index < 0 || index >= static_cast<int32_t>(staticFields_.size()))
            throw std::out_of_range("Static field index out of range");

        return staticFields_[index];
    }

    void ExecutionContext::StoreStaticField(int32_t index, std::shared_ptr<StackItem> item)
    {
        if (index < 0 || index >= static_cast<int32_t>(staticFields_.size()))
            throw std::out_of_range("Static field index out of range");

        staticFields_[index] = item;
    }

    void ExecutionContext::Push(std::shared_ptr<StackItem> item)
    {
        // Ensure the item is a proper StackItem shared_ptr
        if (auto boolItem = std::dynamic_pointer_cast<BooleanItem>(item))
        {
            evaluationStack_.push_back(std::static_pointer_cast<StackItem>(boolItem));
            return;
        }
        else if (auto intItem = std::dynamic_pointer_cast<IntegerItem>(item))
        {
            evaluationStack_.push_back(std::static_pointer_cast<StackItem>(intItem));
            return;
        }
        else if (auto byteStringItem = std::dynamic_pointer_cast<ByteStringItem>(item))
        {
            evaluationStack_.push_back(std::static_pointer_cast<StackItem>(byteStringItem));
            return;
        }
        else if (auto bufferItem = std::dynamic_pointer_cast<BufferItem>(item))
        {
            evaluationStack_.push_back(std::static_pointer_cast<StackItem>(bufferItem));
            return;
        }
        else if (auto arrayItem = std::dynamic_pointer_cast<ArrayItem>(item))
        {
            evaluationStack_.push_back(std::static_pointer_cast<StackItem>(arrayItem));
            return;
        }
        else if (auto structItem = std::dynamic_pointer_cast<StructItem>(item))
        {
            evaluationStack_.push_back(std::static_pointer_cast<StackItem>(structItem));
            return;
        }
        else if (auto mapItem = std::dynamic_pointer_cast<MapItem>(item))
        {
            evaluationStack_.push_back(std::static_pointer_cast<StackItem>(mapItem));
            return;
        }
        else if (auto interopItem = std::dynamic_pointer_cast<InteropInterfaceItem>(item))
        {
            evaluationStack_.push_back(std::static_pointer_cast<StackItem>(interopItem));
            return;
        }
        else if (auto pointerItem = std::dynamic_pointer_cast<PointerItem>(item))
        {
            evaluationStack_.push_back(std::static_pointer_cast<StackItem>(pointerItem));
            return;
        }
        else if (auto nullItem = std::dynamic_pointer_cast<NullItem>(item))
        {
            evaluationStack_.push_back(std::static_pointer_cast<StackItem>(nullItem));
            return;
        }

        // If we get here, it's already a proper StackItem
        evaluationStack_.push_back(item);
    }

    std::shared_ptr<StackItem> ExecutionContext::Pop()
    {
        if (evaluationStack_.empty())
            throw std::runtime_error("Stack is empty");

        auto item = evaluationStack_.back();
        evaluationStack_.pop_back();
        return item;
    }

    std::shared_ptr<StackItem> ExecutionContext::Peek(int32_t index) const
    {
        if (index < 0 || index >= static_cast<int32_t>(evaluationStack_.size()))
            throw std::out_of_range("Stack index out of range");

        return evaluationStack_[evaluationStack_.size() - 1 - index];
    }

    void ExecutionContext::ClearStack()
    {
        evaluationStack_.clear();
    }

    int32_t ExecutionContext::GetStackSize() const
    {
        return static_cast<int32_t>(evaluationStack_.size());
    }

    void ExecutionContext::EnterTry(int32_t catchOffset, int32_t finallyOffset, int32_t endOffset)
    {
        tryStack_.emplace_back(catchOffset, finallyOffset);
        tryStack_.back().SetEndPointer(endOffset);
    }

    void ExecutionContext::ExitTry()
    {
        if (tryStack_.empty())
            throw std::runtime_error("Not in a try block");

        tryStack_.pop_back();
    }

    std::optional<int32_t> ExecutionContext::GetCatchOffset() const
    {
        if (tryStack_.empty())
            return std::nullopt;

        int32_t catchOffset = tryStack_.back().GetCatchPointer();
        if (catchOffset < 0)
            return std::nullopt;

        return catchOffset;
    }

    std::optional<int32_t> ExecutionContext::GetFinallyOffset() const
    {
        if (tryStack_.empty())
            return std::nullopt;

        int32_t finallyOffset = tryStack_.back().GetFinallyPointer();
        if (finallyOffset < 0)
            return std::nullopt;

        return finallyOffset;
    }

    std::optional<int32_t> ExecutionContext::GetEndOffset() const
    {
        if (tryStack_.empty())
            return std::nullopt;

        int32_t endOffset = tryStack_.back().GetEndPointer();
        if (endOffset < 0)
            return std::nullopt;

        return endOffset;
    }

    ExceptionHandlingContext& ExecutionContext::GetCurrentTry()
    {
        if (tryStack_.empty())
            throw InvalidOperationException("No try context available");

        return tryStack_.back();
    }

    const ExceptionHandlingContext& ExecutionContext::GetCurrentTry() const
    {
        if (tryStack_.empty())
            throw InvalidOperationException("No try context available");

        return tryStack_.back();
    }

    void ExecutionContext::SetTryState(int32_t catchPosition, int32_t finallyPosition)
    {
        EnterTry(catchPosition, finallyPosition, -1);
    }

    void ExecutionContext::ClearTryState()
    {
        if (!tryStack_.empty())
            tryStack_.pop_back();
    }

    std::shared_ptr<ExecutionContext> ExecutionContext::Clone(int32_t initialPosition) const
    {
        // Create a new context with the same script and rvcount
        auto clone = std::make_shared<ExecutionContext>(script_, rvcount_);

        // Set the instruction pointer
        if (initialPosition >= 0)
            clone->SetInstructionPointer(initialPosition);
        else
            clone->SetInstructionPointer(instructionPointer_);

        // Copy static fields, local variables, and arguments
        clone->staticFields_ = staticFields_;
        clone->localVariables_ = localVariables_;
        clone->arguments_ = arguments_;

        // Copy evaluation stack
        clone->evaluationStack_ = evaluationStack_;

        // Copy try stack
        clone->tryStack_ = tryStack_;

        // Copy call table
        clone->callTable_ = callTable_;

        // Copy states
        clone->states_ = states_;

        return clone;
    }

    const std::vector<int32_t>& ExecutionContext::GetCallTable() const
    {
        return callTable_;
    }
}
