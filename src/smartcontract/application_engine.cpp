#include <neo/smartcontract/application_engine.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/block.h>
#include <neo/smartcontract/native/native_contract.h>

namespace neo::smartcontract
{
    ApplicationEngine::ApplicationEngine(TriggerType trigger, 
                                       const io::ISerializable* container,
                                       std::shared_ptr<persistence::DataCache> snapshot,
                                       const ledger::Block* persistingBlock,
                                       int64_t gas)
        : trigger_(trigger)
        , container_(container)
        , snapshot_(snapshot)
        , persisting_block_(persistingBlock)
        , gas_limit_(gas)
        , gas_consumed_(0)
        , state_(neo::vm::VMState::None)
        , flags_(CallFlags::None)
    {
        // Initialize basic state
    }

    neo::vm::VMState ApplicationEngine::Execute()
    {
        try
        {
            // Execute using base ExecutionEngine functionality
            state_ = ExecutionEngine::Execute();
            return state_;
        }
        catch (...)
        {
            state_ = neo::vm::VMState::Fault;
            return state_;
        }
    }

    neo::vm::VMState ApplicationEngine::Execute(const std::vector<uint8_t>& script)
    {
        LoadScript(script);
        return Execute();
    }

    void ApplicationEngine::LoadScript(const std::vector<uint8_t>& script)
    {
        // Convert to internal ByteVector and load into VM
        neo::vm::internal::ByteVector internalScript;
        internalScript.Reserve(script.size());
        for (uint8_t byte : script)
        {
            internalScript.Push(byte);
        }
        
        neo::vm::Script vmScript(internalScript);
        ExecutionEngine::LoadScript(vmScript);
    }

    const io::ISerializable* ApplicationEngine::GetContainer() const
    {
        return container_;
    }

    const io::ISerializable* ApplicationEngine::GetScriptContainer() const
    {
        return container_;
    }

    std::shared_ptr<persistence::DataCache> ApplicationEngine::GetSnapshot() const
    {
        return snapshot_;
    }

    const ledger::Block* ApplicationEngine::GetPersistingBlock() const
    {
        return persisting_block_;
    }

    io::UInt160 ApplicationEngine::GetCurrentScriptHash() const
    {
        // Return zero hash for minimal implementation
        return io::UInt160::Zero();
    }

    io::UInt160 ApplicationEngine::GetCallingScriptHash() const
    {
        // Return zero hash for minimal implementation
        return io::UInt160::Zero();
    }

    io::UInt160 ApplicationEngine::GetEntryScriptHash() const
    {
        // Return zero hash for minimal implementation
        return io::UInt160::Zero();
    }

    bool ApplicationEngine::HasFlag(CallFlags flag) const
    {
        return (static_cast<uint8_t>(flags_) & static_cast<uint8_t>(flag)) != 0;
    }

    void ApplicationEngine::AddGas(int64_t gas)
    {
        gas_consumed_ += gas;
    }

    bool ApplicationEngine::CheckWitness(const io::UInt160& hash) const
    {
        // Basic witness checking - minimal implementation
        // In full implementation, this would check transaction signers
        return true;
    }

    bool ApplicationEngine::CheckWitness(const io::UInt256& hash) const
    {
        // Basic witness checking - minimal implementation
        return true;
    }

    bool ApplicationEngine::CheckWitnessInternal(const io::UInt160& hash) const
    {
        // Internal witness checking - minimal implementation
        return true;
    }

    void ApplicationEngine::Log(const std::string& message)
    {
        // Add log entry
        LogEntry entry;
        entry.script_hash = GetCurrentScriptHash();
        entry.message = message;
        
        AddLog(entry);
    }

    void ApplicationEngine::Notify(const io::UInt160& scriptHash, const std::string& eventName, const std::vector<std::shared_ptr<vm::StackItem>>& state)
    {
        // Add notification entry
        NotifyEntry entry;
        entry.script_hash = scriptHash;
        entry.event_name = eventName;
        entry.state = state;
        
        AddNotification(entry);
    }

    const ledger::Transaction* ApplicationEngine::GetTransaction() const
    {
        // Try to cast container to transaction
        return dynamic_cast<const ledger::Transaction*>(container_);
    }

    int64_t ApplicationEngine::GetGasPrice() const
    {
        return gasPrice_;
    }

    uint32_t ApplicationEngine::GetPlatformVersion() const
    {
        return platformVersion_;
    }

    uint64_t ApplicationEngine::GetRandom() const
    {
        return random_;
    }

    int64_t ApplicationEngine::GetNetworkFeePerByte() const
    {
        return networkFeePerByte_;
    }

    vm::ExecutionEngineLimits ApplicationEngine::GetLimits() const
    {
        vm::ExecutionEngineLimits limits;
        limits.MaxStackSize = 2048;
        limits.MaxInvocationStackSize = 1024;
        limits.MaxItemSize = 1024 * 1024;
        return limits;
    }

    native::NativeContract* ApplicationEngine::GetNativeContract(const io::UInt160& hash) const
    {
        // For now, return nullptr - in full implementation this would look up native contracts
        return nullptr;
    }

    ContractState ApplicationEngine::CreateContract(const io::ByteVector& script, const std::string& manifest, uint32_t offset)
    {
        // Basic contract creation - minimal implementation
        ContractState state;
        // Use public constructor or methods to set data
        return state;
    }

    std::shared_ptr<vm::StackItem> ApplicationEngine::CallContract(const io::UInt160& scriptHash, const std::string& method, const std::vector<std::shared_ptr<vm::StackItem>>& args, CallFlags flags)
    {
        // Basic contract call - minimal implementation
        return vm::StackItem::Null();
    }

    void ApplicationEngine::RegisterSystemCall(const std::string& name, std::function<bool(vm::ExecutionEngine&)> handler, int64_t gasCost, CallFlags requiredFlags)
    {
        SystemCallDescriptor descriptor;
        descriptor.name = name;
        descriptor.handler = handler;
        descriptor.gasCost = gasCost;
        descriptor.requiredFlags = requiredFlags;
        
        systemCalls_[name] = descriptor;
    }

    std::unique_ptr<ApplicationEngine> ApplicationEngine::Create(
        TriggerType trigger,
        const io::ISerializable* container,
        std::shared_ptr<persistence::DataCache> snapshot,
        const ledger::Block* persistingBlock,
        int64_t gas)
    {
        return std::make_unique<ApplicationEngine>(trigger, container, snapshot, persistingBlock, gas);
    }

    std::unique_ptr<ApplicationEngine> ApplicationEngine::Run(
        const io::ByteVector& script,
        std::shared_ptr<persistence::DataCache> snapshot,
        const io::ISerializable* container,
        const ledger::Block* persistingBlock,
        int32_t offset,
        int64_t gas)
    {
        auto engine = Create(TriggerType::Application, container, snapshot, persistingBlock, gas);
        
        // Convert ByteVector to std::vector<uint8_t>
        std::vector<uint8_t> scriptBytes(script.AsSpan().Data(), 
                                       script.AsSpan().Data() + script.AsSpan().Size());
        
        engine->Execute(scriptBytes);
        return engine;
    }

    std::shared_ptr<vm::StackItem> ApplicationEngine::Pop()
    {
        // Delegate to base ExecutionEngine
        return ExecutionEngine::Pop();
    }

    void ApplicationEngine::Push(std::shared_ptr<vm::StackItem> item)
    {
        // Delegate to base ExecutionEngine
        ExecutionEngine::Push(item);
    }

    std::shared_ptr<vm::StackItem> ApplicationEngine::Peek() const
    {
        // For now, return a dummy item - in full implementation this would peek at evaluation stack
        return vm::StackItem::CreateByteString(std::vector<uint8_t>{});
    }

    io::ByteVector ApplicationEngine::GetScript() const
    {
        // Return the script currently being executed
        // For now, return empty script - in full implementation this would return the current script
        return io::ByteVector();
    }

    std::string ApplicationEngine::GetException() const
    {
        // Return exception message if execution failed
        if (state_ == neo::vm::VMState::Fault)
        {
            return "Script execution failed";
        }
        return "";
    }

    std::vector<std::shared_ptr<vm::StackItem>> ApplicationEngine::GetResultStack() const
    {
        // Return the result stack
        // For now, return empty stack - in full implementation this would return the evaluation stack
        return std::vector<std::shared_ptr<vm::StackItem>>{};
    }

    uint32_t ApplicationEngine::GetNetworkMagic() const
    {
        // Return default mainnet magic for now
        return 0x4E454F00; // "NEO\0"
    }

    const ProtocolSettings* ApplicationEngine::GetProtocolSettings() const
    {
        return &protocolSettings_;
    }

    uint32_t ApplicationEngine::GetCurrentBlockHeight() const
    {
        if (persisting_block_)
        {
            return persisting_block_->GetIndex();
        }
        
        // Fallback to 0 if no block information available
        return 0;
    }

    bool ApplicationEngine::IsHardforkEnabled(int hardfork) const
    {
        // Basic hardfork check - in full implementation this would check protocol settings
        return true; // For now, assume all hardforks are enabled
    }

} // namespace neo::smartcontract