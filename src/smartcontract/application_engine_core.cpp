#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/system_call_exception.h>
#include <neo/smartcontract/transaction_verifier.h>
#include <neo/smartcontract/native/native_contract.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/policy_contract.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/smartcontract/native/std_lib.h>
#include <neo/smartcontract/native/crypto_lib.h>
#include <neo/cryptography/hash.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/vm/script.h>
#include <sstream>

namespace neo::smartcontract
{
    ApplicationEngine::ApplicationEngine(TriggerType trigger, const io::ISerializable* container, std::shared_ptr<persistence::DataCache> snapshot, const ledger::Block* persistingBlock, int64_t gas)
        : trigger_(trigger), container_(container), snapshot_(snapshot), persistingBlock_(persistingBlock), gasConsumed_(0), gasLeft_(gas), flags_(CallFlags::All)
    {
        RegisterSystemCalls();
    }

    // Note: GetTrigger(), GetGasConsumed(), GetGasLeft() are already inline in header
    
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
        return persistingBlock_;
    }

    io::UInt160 ApplicationEngine::GetCurrentScriptHash() const
    {
        if (scriptHashes_.empty())
            return io::UInt160();

        return scriptHashes_.back();
    }

    io::UInt160 ApplicationEngine::GetCallingScriptHash() const
    {
        if (scriptHashes_.size() < 2)
            return io::UInt160();

        return scriptHashes_[scriptHashes_.size() - 2];
    }

    io::UInt160 ApplicationEngine::GetEntryScriptHash() const
    {
        if (scriptHashes_.empty())
            return io::UInt160();

        return scriptHashes_.front();
    }

    // Note: GetNotifications() is already inline in header

    void ApplicationEngine::LoadScript(const std::vector<uint8_t>& script)
    {
        // Calculate script hash
        io::UInt160 hash = cryptography::Hash::Hash160(io::ByteSpan(script.data(), script.size()));

        // Load script into VM
        vm::ExecutionEngine::LoadScript(vm::Script(script));
        
        // Track script hash
        scriptHashes_.push_back(hash);
    }

    vm::VMState ApplicationEngine::Execute()
    {
        // Execute the VM with gas tracking
        auto state = vm::ExecutionEngine::Execute();
        
        // Note: Gas consumption tracking simplified for now
        // TODO: Implement proper gas metering
        
        return state;
    }

    bool ApplicationEngine::HasFlag(CallFlags flag) const
    {
        return (static_cast<uint8_t>(flags_) & static_cast<uint8_t>(flag)) != 0;
    }

    void ApplicationEngine::AddGas(int64_t gas)
    {
        if (gas < 0)
            throw std::invalid_argument("Gas cannot be negative");

        if (gasLeft_ >= 0)
        {
            if (gasLeft_ < gas)
                throw std::runtime_error("Insufficient gas");

            gasLeft_ -= gas;
        }

        gasConsumed_ += gas;
    }

    bool ApplicationEngine::CheckWitness(const io::UInt160& hash) const
    {
        // Check if the hash is the current script hash
        if (GetCurrentScriptHash() == hash)
            return true;

        // Check if the hash is in the calling chain
        for (const auto& scriptHash : scriptHashes_)
        {
            if (scriptHash == hash)
                return true;
        }

        // Check if the hash is in the transaction signers
        auto tx = GetTransaction();
        if (tx)
        {
            for (const auto& signer : tx->GetSigners())
            {
                if (signer == hash)
                    return true;
            }
        }

        return false;
    }

    bool ApplicationEngine::CheckWitness(const io::UInt256& hash) const
    {
        // For UInt256 (public key), we need to convert it to a script hash
        // This is a simplified implementation
        io::ByteVector data(hash.Data(), hash.size());
        io::UInt160 scriptHash = cryptography::Hash::Hash160(data.AsSpan());

        return CheckWitness(scriptHash);
    }

    ContractState ApplicationEngine::CreateContract(const io::ByteVector& script, const std::string& manifest, uint32_t offset)
    {
        if (!HasFlag(CallFlags::WriteStates))
            throw MissingFlagsException("CreateContract", "WriteStates");

        // Calculate script hash
        io::UInt160 scriptHash = cryptography::Hash::Hash160(script.AsSpan());

        // Check if contract already exists (simplified - using contract ID 0 as placeholder)
        persistence::StorageKey key(0, io::ByteVector{0x0f}); // 0x0f is the prefix for contract storage
        if (snapshot_->TryGet(key))
            throw SystemCallException("CreateContract", "Contract already exists");

        // Get next available ID (simplified)
        persistence::StorageKey idKey(0, io::ByteVector{0x0f}); // 0x0f is the prefix for next available ID
        auto idItem = snapshot_->TryGet(idKey);
        uint32_t id = 1;
        if (idItem)
        {
            std::istringstream stream(std::string(reinterpret_cast<const char*>(idItem->GetValue().Data()), idItem->GetValue().Size()));
            io::BinaryReader reader(stream);
            id = reader.ReadUInt32();
        }

        // Create contract state
        ContractState contract;
        contract.SetId(id);
        contract.SetScriptHash(scriptHash);
        contract.SetScript(script);
        contract.SetManifest(manifest);

        // Store contract state
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        contract.Serialize(writer);
        std::string data = stream.str();

        persistence::StorageItem item(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));
        snapshot_->Add(key, item);

        // Update next available ID
        std::ostringstream idStream;
        io::BinaryWriter idWriter(idStream);
        idWriter.Write(id + 1);
        std::string idData = idStream.str();
        persistence::StorageItem idItemNew(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(idData.data()), idData.size())));
        snapshot_->Add(idKey, idItemNew);

        return contract;
    }

    std::shared_ptr<vm::StackItem> ApplicationEngine::CallContract(const io::UInt160& scriptHash, const std::string& method, const std::vector<std::shared_ptr<vm::StackItem>>& args, CallFlags flags)
    {
        if (!HasFlag(CallFlags::AllowCall))
            throw std::runtime_error("Cannot call contract without AllowCall flag");

        // Get contract (simplified - use contract ID 0 as placeholder)
        persistence::StorageKey key(0, io::ByteVector{0x0f}); // 0x0f is the prefix for contract storage
        auto item = snapshot_->TryGet(key);
        if (!item)
            throw std::runtime_error("Contract not found");

        // Deserialize contract state
        std::istringstream stream(std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
        io::BinaryReader reader(stream);
        ContractState contract;
        contract.Deserialize(reader);

        // Check if method exists
        auto it = contracts_.find(scriptHash);
        if (it != contracts_.end())
        {
            auto methodIt = it->second.find(method);
            if (methodIt != it->second.end())
            {
                // Call native method
                CallFlags oldFlags = flags_;
                flags_ = flags;
                bool result = methodIt->second(*this);
                flags_ = oldFlags;

                if (!result)
                    throw std::runtime_error("Native method execution failed");

                return GetCurrentContext().Pop();
            }
        }

        // Call contract method
        CallFlags oldFlags = flags_;
        flags_ = flags;

        // Load script
        LoadScript(contract.GetScript());

        // Execute
        auto state = Execute();
        flags_ = oldFlags;

        if (state != vm::VMState::Halt)
            throw std::runtime_error("Contract execution failed");

        return GetCurrentContext().Pop();
    }

    void ApplicationEngine::Notify(const io::UInt160& scriptHash, const std::string& eventName, const std::vector<std::shared_ptr<vm::StackItem>>& state)
    {
        if (!HasFlag(CallFlags::AllowNotify))
            throw std::runtime_error("Cannot notify without AllowNotify flag");

        notifications_.emplace_back(scriptHash, eventName, state);
    }

    const ledger::Transaction* ApplicationEngine::GetTransaction() const
    {
        if (container_ && dynamic_cast<const ledger::Transaction*>(container_))
            return static_cast<const ledger::Transaction*>(container_);

        return nullptr;
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
        // Return the execution engine limits (same as C# implementation)
        return vm::ExecutionEngine::GetLimits();
    }

    std::unique_ptr<ApplicationEngine> ApplicationEngine::Create(TriggerType trigger, const io::ISerializable* container, std::shared_ptr<persistence::DataCache> snapshot, const ledger::Block* persistingBlock, int64_t gas)
    {
        return std::make_unique<ApplicationEngine>(trigger, container, snapshot, persistingBlock, gas);
    }

    std::unique_ptr<ApplicationEngine> ApplicationEngine::Run(const io::ByteVector& script, std::shared_ptr<persistence::DataCache> snapshot, const io::ISerializable* container, const ledger::Block* persistingBlock, int32_t offset, int64_t gas)
    {
        auto engine = Create(TriggerType::Application, container, snapshot, persistingBlock, gas);
        engine->LoadScript(script);
        engine->Execute();
        return engine;
    }

    const ProtocolSettings* ApplicationEngine::GetProtocolSettings() const
    {
        return &protocolSettings_;
    }

    bool ApplicationEngine::IsHardforkEnabled(int hardfork) const
    {
        // Implement proper hardfork checking matching C# logic
        // Return true if PersistingBlock is null and Hardfork is enabled
        if (persistingBlock_ == nullptr)
        {
            // Check if hardfork is configured in protocol settings
            return protocolSettings_.IsHardforkEnabled(static_cast<Hardfork>(hardfork), 0);
        }

        // Check if hardfork is enabled at the persisting block index
        return protocolSettings_.IsHardforkEnabled(static_cast<Hardfork>(hardfork), persistingBlock_->GetIndex());
    }

    native::NativeContract* ApplicationEngine::GetNativeContract(const io::UInt160& hash) const
    {
        // Implement proper native contract lookup matching C# logic
        // Check all registered native contracts

        // NEO Token
        auto neoToken = native::NeoToken::GetInstance();
        if (neoToken && neoToken->GetScriptHash() == hash)
            return neoToken.get();

        // Contract Management
        auto contractManagement = native::ContractManagement::GetInstance();
        if (contractManagement && contractManagement->GetScriptHash() == hash)
            return contractManagement.get();

        // Policy Contract
        auto policyContract = native::PolicyContract::GetInstance();
        if (policyContract && policyContract->GetScriptHash() == hash)
            return policyContract.get();

        // Ledger Contract
        auto ledgerContract = native::LedgerContract::GetInstance();
        if (ledgerContract && ledgerContract->GetScriptHash() == hash)
            return ledgerContract.get();

        // StdLib and CryptoLib contracts don't have GetInstance methods
        // TODO: Implement proper singleton pattern for these contracts

        return nullptr;
    }

}
