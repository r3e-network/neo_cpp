#include <algorithm>
#include <cstring>
#include <neo/cryptography/hash.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/jump_table.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/special_items.h>
#include <stdexcept>

namespace neo::vm
{
// Static JumpTable instance
static JumpTable defaultJumpTable;

// ExecutionEngine implementation
ExecutionEngine::ExecutionEngine()
    : state_(VMState::None), jumpTable_(defaultJumpTable), jumping_(false), limits_(ExecutionEngineLimits::Default)
{
}

ExecutionEngine::ExecutionEngine(const JumpTable& jumpTable)
    : state_(VMState::None), jumpTable_(jumpTable), jumping_(false), limits_(ExecutionEngineLimits::Default)
{
}

ExecutionEngine::ExecutionEngine(const ExecutionEngineLimits& limits)
    : state_(VMState::None), jumpTable_(defaultJumpTable), jumping_(false), limits_(limits)
{
}

ExecutionEngine::ExecutionEngine(const JumpTable& jumpTable, const ExecutionEngineLimits& limits)
    : state_(VMState::None), jumpTable_(jumpTable), jumping_(false), limits_(limits)
{
}

ExecutionEngine::~ExecutionEngine()
{
    invocationStack_.clear();
}

const ExecutionEngineLimits& ExecutionEngine::GetLimits() const
{
    return limits_;
}

ExecutionContext& ExecutionEngine::GetCurrentContext()
{
    if (invocationStack_.empty())
        throw std::runtime_error("Invocation stack is empty");

    return *invocationStack_.back();
}

const ExecutionContext& ExecutionEngine::GetCurrentContext() const
{
    if (invocationStack_.empty())
        throw std::runtime_error("Invocation stack is empty");

    return *invocationStack_.back();
}

std::shared_ptr<ExecutionContext> ExecutionEngine::GetEntryContext() const
{
    return entryContext_;
}

const std::vector<std::shared_ptr<ExecutionContext>>& ExecutionEngine::GetInvocationStack() const
{
    return invocationStack_;
}

void ExecutionEngine::Push(std::shared_ptr<StackItem> item)
{
    // Add stack reference before pushing to evaluation stack
    referenceCounter_.AddStackReference(item);

    // Ensure the item is a proper StackItem shared_ptr
    if (auto boolItem = std::dynamic_pointer_cast<BooleanItem>(item))
    {
        GetCurrentContext().Push(std::static_pointer_cast<StackItem>(boolItem));
        return;
    }
    else if (auto intItem = std::dynamic_pointer_cast<IntegerItem>(item))
    {
        GetCurrentContext().Push(std::static_pointer_cast<StackItem>(intItem));
        return;
    }
    else if (auto byteStringItem = std::dynamic_pointer_cast<ByteStringItem>(item))
    {
        GetCurrentContext().Push(std::static_pointer_cast<StackItem>(byteStringItem));
        return;
    }
    else if (auto bufferItem = std::dynamic_pointer_cast<BufferItem>(item))
    {
        GetCurrentContext().Push(std::static_pointer_cast<StackItem>(bufferItem));
        return;
    }
    else if (auto arrayItem = std::dynamic_pointer_cast<ArrayItem>(item))
    {
        GetCurrentContext().Push(std::static_pointer_cast<StackItem>(arrayItem));
        return;
    }
    else if (auto structItem = std::dynamic_pointer_cast<StructItem>(item))
    {
        GetCurrentContext().Push(std::static_pointer_cast<StackItem>(structItem));
        return;
    }
    else if (auto mapItem = std::dynamic_pointer_cast<MapItem>(item))
    {
        GetCurrentContext().Push(std::static_pointer_cast<StackItem>(mapItem));
        return;
    }
    else if (auto interopItem = std::dynamic_pointer_cast<InteropInterfaceItem>(item))
    {
        GetCurrentContext().Push(std::static_pointer_cast<StackItem>(interopItem));
        return;
    }
    else if (auto pointerItem = std::dynamic_pointer_cast<PointerItem>(item))
    {
        GetCurrentContext().Push(std::static_pointer_cast<StackItem>(pointerItem));
        return;
    }
    else if (auto nullItem = std::dynamic_pointer_cast<NullItem>(item))
    {
        GetCurrentContext().Push(std::static_pointer_cast<StackItem>(nullItem));
        return;
    }

    // If we get here, it's already a proper StackItem
    GetCurrentContext().Push(item);
}

std::shared_ptr<StackItem> ExecutionEngine::Pop()
{
    auto item = GetCurrentContext().Pop();
    // Remove stack reference after popping from evaluation stack
    referenceCounter_.RemoveStackReference(item);
    return item;
}

std::shared_ptr<StackItem> ExecutionEngine::Peek(int32_t index) const
{
    return GetCurrentContext().Peek(index);
}

void ExecutionEngine::SetJumping(bool jumping)
{
    jumping_ = jumping;
}

bool ExecutionEngine::IsJumping() const
{
    return jumping_;
}

ReferenceCounter* ExecutionEngine::GetReferenceCounter()
{
    return &referenceCounter_;
}

bool ExecutionEngine::HasUncaughtException() const
{
    return uncaughtException_ != nullptr;
}

std::shared_ptr<StackItem> ExecutionEngine::GetUncaughtException() const
{
    return uncaughtException_;
}

void ExecutionEngine::ClearUncaughtException()
{
    uncaughtException_ = nullptr;
}

void ExecutionEngine::SetUncaughtException(std::shared_ptr<StackItem> exception)
{
    uncaughtException_ = exception;
}

VMState ExecutionEngine::GetState() const
{
    return state_;
}

void ExecutionEngine::SetState(VMState state)
{
    if (state_ != state)
    {
        state_ = state;
        OnStateChanged();
    }
}

const std::vector<std::shared_ptr<StackItem>>& ExecutionEngine::GetResultStack() const
{
    return resultStack_;
}

std::shared_ptr<ExecutionContext> ExecutionEngine::CreateContext(const Script& script, int32_t rvcount,
                                                                 int32_t initialPosition)
{
    auto context = std::make_shared<ExecutionContext>(script, rvcount);
    context->SetInstructionPointer(initialPosition);
    return context;
}

std::shared_ptr<ExecutionContext> ExecutionEngine::LoadScript(const Script& script, int32_t initialPosition,
                                                              std::function<void(ExecutionContext&)> configureContext)
{
    auto context = CreateContext(script, -1, initialPosition);

    if (configureContext)
        configureContext(*context);

    LoadContext(context);
    return context;
}

void ExecutionEngine::LoadContext(std::shared_ptr<ExecutionContext> context)
{
    if (invocationStack_.size() >= limits_.MaxInvocationStackSize)
        throw InvalidOperationException("MaxInvocationStackSize exceed: " + std::to_string(invocationStack_.size()));

    invocationStack_.push_back(context);

    if (entryContext_ == nullptr)
        entryContext_ = context;
}

void ExecutionEngine::UnloadContext(ExecutionContext& context)
{
    if (invocationStack_.empty())
    {
        entryContext_ = nullptr;
    }
    else
    {
        // Cleanup for static fields if they are different from current context
        auto& staticFields = context.GetStaticFields();
        auto currentContext = invocationStack_.empty() ? nullptr : invocationStack_.back();

        if (!staticFields.empty() && (currentContext == nullptr || staticFields != currentContext->GetStaticFields()))
        {
            // Clear references for static fields
            for (const auto& item : staticFields)
            {
                if (item != nullptr)
                {
                    // Remove stack reference from the reference counter
                    referenceCounter_.RemoveStackReference(item);
                }
            }
        }

        // Clear references for local variables
        const auto& localVariables = context.GetLocalVariables();
        for (const auto& item : localVariables)
        {
            if (item != nullptr)
            {
                // Remove stack reference from the reference counter
                referenceCounter_.RemoveStackReference(item);
            }
        }

        // Clear references for arguments
        const auto& arguments = context.GetArguments();
        for (const auto& item : arguments)
        {
            if (item != nullptr)
            {
                // Remove stack reference from the reference counter
                referenceCounter_.RemoveStackReference(item);
            }
        }
    }
}

VMState ExecutionEngine::Execute(int64_t gasLimit)
{
    if (invocationStack_.empty())
        return VMState::None;

    // Check if the invocation stack is too large
    if (invocationStack_.size() > limits_.MaxInvocationStackSize)
    {
        SetState(VMState::Fault);
        return state_;
    }

    if (state_ == VMState::Break)
        SetState(VMState::None);

    while (state_ != VMState::Halt && state_ != VMState::Fault)
    {
        ExecuteNext();

        // Check gas limit if specified
        if (gasLimit >= 0 && gasLimit-- <= 0)
            break;
    }

    return state_;
}

void ExecutionEngine::ExecuteNext()
{
    if (invocationStack_.empty())
    {
        SetState(VMState::Halt);
        // Clean up any remaining zero-referenced items when execution completes
        referenceCounter_.CheckZeroReferred();
        return;
    }

    try
    {
        // Get the current context
        auto& context = *invocationStack_.back();

        // Use Instruction::RET if CurrentInstruction is null, exactly like C#
        Instruction instruction =
            context.GetCurrentInstruction() ? *context.GetCurrentInstruction() : Instruction(OpCode::RET);

        // Pre-execute instruction hook (for debugging)
        PreExecuteInstruction(instruction);

        try
        {
            // Execute the instruction
            OpCode opcode = instruction.opcode;

            // Use the jump table to execute the instruction
            const auto& handler = jumpTable_[opcode];
            handler(*this, instruction);
        }
        catch (const CatchableException& ex)
        {
            if (limits_.CatchEngineExceptions)
            {
                JumpTable::ExecuteThrow(*this, ex.what());
            }
            else
            {
                throw;
            }
        }

        // Post-execute instruction hook (for debugging)
        PostExecuteInstruction(instruction);

        if (!jumping_)
            context.MoveNext();

        jumping_ = false;
    }
    catch (const std::runtime_error& e)
    {
        // Set the state to fault on runtime errors
        OnFault();
    }
    catch (const std::invalid_argument& e)
    {
        // Set the state to fault on invalid arguments
        OnFault();
    }
    catch (const std::out_of_range& e)
    {
        // Set the state to fault on out of range errors
        OnFault();
    }
    catch (const std::exception& e)
    {
        // Set the state to fault on standard exceptions
        OnFault();
    }
    catch (...)
    {
        // Set the state to fault on any unhandled system exception
        OnFault();
    }
}

void ExecutionEngine::RegisterSystemCall(const std::string& name, std::function<bool(ExecutionEngine&)> handler)
{
    // Calculate the hash of the name
    uint32_t hash = 0;
    for (char c : name)
    {
        hash = ((hash << 5) + hash) ^ c;
    }

    systemCalls_[hash] = SystemCall(name, handler);
}

const SystemCall& ExecutionEngine::GetSystemCall(uint32_t hash) const
{
    auto it = systemCalls_.find(hash);
    if (it == systemCalls_.end())
        throw std::runtime_error("System call not found");

    return it->second;
}

void ExecutionEngine::PreExecuteInstruction(const Instruction& instruction)
{
    // This method is a hook for derived classes to override
}

void ExecutionEngine::PostExecuteInstruction(const Instruction& instruction)
{
    // Check if reference count exceeds limits
    auto currentCount = referenceCounter_.Count();
    if (currentCount > limits_.MaxStackSize)
    {
        // Try to clean up unreferenced items
        auto cleanedCount = referenceCounter_.CheckZeroReferred();
        if (cleanedCount > limits_.MaxStackSize)
        {
            throw InvalidOperationException("MaxStackSize exceed: " + std::to_string(cleanedCount) + "/" +
                                            std::to_string(limits_.MaxStackSize));
        }
    }
}

void ExecutionEngine::OnStateChanged()
{
    // This method is a hook for derived classes to override
}

void ExecutionEngine::OnFault(std::exception_ptr ex)
{
    SetState(VMState::Fault);
    uncaughtException_ = nullptr;  // Clear any previous exception

    // If an exception was provided, convert it to a string and set it as the uncaught exception
    if (ex)
    {
        try
        {
            std::rethrow_exception(ex);
        }
        catch (const std::exception& e)
        {
            // Create a ByteString with the exception message
            uncaughtException_ = StackItem::Create(e.what());
        }
        catch (const std::bad_alloc&)
        {
            // Create a ByteString for memory allocation failures
            uncaughtException_ = StackItem::Create("Out of memory");
        }
        catch (...)
        {
            // Create a ByteString with a generic message for system exceptions
            uncaughtException_ = StackItem::Create("System exception");
        }
    }
}

void ExecutionEngine::OnFault()
{
    OnFault(std::current_exception());
}

bool ExecutionEngine::ExecuteRet()
{
    auto& context = GetCurrentContext();
    auto rvcount = context.GetRVCount();
    auto evaluationStack = context.GetEvaluationStack();

    if (invocationStack_.size() <= 1)
    {
        // For the last context (main script), move evaluation stack to result stack
        // When rvcount is -1 (default), return only the top value if any
        if (rvcount == -1 && !evaluationStack.empty())
        {
            // Return only the top value
            resultStack_.push_back(evaluationStack.back());
        }
        else
        {
            // Return all values from evaluation stack in reverse order
            for (auto it = evaluationStack.rbegin(); it != evaluationStack.rend(); ++it)
            {
                resultStack_.push_back(*it);
            }
        }
        SetState(VMState::Halt);
        // Clean up any remaining zero-referenced items when execution completes
        referenceCounter_.CheckZeroReferred();
        return false;
    }

    // Extract the return values based on rvcount
    if (rvcount >= 0)
    {
        int count = std::min(static_cast<int>(evaluationStack.size()), rvcount);
        for (int i = 0; i < count; i++)
        {
            resultStack_.push_back(evaluationStack[evaluationStack.size() - count + i]);
        }
    }
    else
    {
        // Return all values
        for (auto it = evaluationStack.rbegin(); it != evaluationStack.rend(); ++it)
        {
            resultStack_.push_back(*it);
        }
    }

    // Unload the context
    UnloadContext(GetCurrentContext());
    invocationStack_.pop_back();

    return true;
}

bool ExecutionEngine::ExecuteSysCall(uint32_t hash)
{
    try
    {
        const auto& syscall = GetSystemCall(hash);
        return syscall.GetHandler()(*this);
    }
    catch (const std::exception&)
    {
        SetState(VMState::Fault);
        return false;
    }
}

bool ExecutionEngine::ExecuteCall(int32_t position)
{
    if (invocationStack_.size() >= limits_.MaxInvocationStackSize)
        throw StackOverflowException("Max invocation stack size exceeded");

    auto& context = GetCurrentContext();
    auto script = context.GetScript();
    auto newContext = CreateContext(script, -1, position);
    LoadContext(newContext);
    return true;
}

bool ExecutionEngine::ExecuteJump(int32_t position)
{
    auto& context = GetCurrentContext();
    context.SetInstructionPointer(position);
    SetJumping(true);
    return true;
}

bool ExecutionEngine::ExecuteThrow(std::shared_ptr<StackItem> exception)
{
    SetUncaughtException(exception);
    SetState(VMState::Fault);
    return false;
}

bool ExecutionEngine::ExecuteTry(int32_t catchPosition, int32_t finallyPosition)
{
    auto& context = GetCurrentContext();
    context.SetTryState(catchPosition, finallyPosition);
    return true;
}

bool ExecutionEngine::ExecuteEndTry(int32_t position)
{
    auto& context = GetCurrentContext();
    context.ClearTryState();
    if (position >= 0)
    {
        context.SetInstructionPointer(position);
        SetJumping(true);
    }
    return true;
}
}  // namespace neo::vm
