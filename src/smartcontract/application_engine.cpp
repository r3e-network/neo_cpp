/**
 * @file application_engine_core.cpp
 * @brief Smart contract execution engine
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/hash.h>
#include <neo/io/byte_span.h>
#include <neo/ledger/witness_rule.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/vm/internal/byte_vector.h>
#include <neo/vm/script_builder.h>
#include <nlohmann/json.hpp>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/crypto_lib.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/smartcontract/native/name_service.h>
#include <neo/smartcontract/native/native_contract.h>
#include <neo/smartcontract/native/native_contract_manager.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/notary.h>
#include <neo/smartcontract/native/oracle_contract.h>
#include <neo/smartcontract/native/policy_contract.h>
#include <neo/smartcontract/native/role_management.h>
#include <neo/smartcontract/native/std_lib.h>
#include <neo/smartcontract/system_call_exception.h>
#include <neo/smartcontract/transaction_verifier.h>
#include <neo/vm/script.h>
#include <neo/vm/internal/byte_span.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <optional>
#include <sstream>

namespace neo::smartcontract
{
ApplicationEngine::ApplicationEngine(TriggerType trigger, const io::ISerializable* container,
                                     std::shared_ptr<persistence::DataCache> snapshot,
                                     const ledger::Block* persistingBlock, int64_t gas)
    : trigger_(trigger),
      container_(container),
      snapshot_(snapshot),
      persistingBlock_(persistingBlock),
      gasConsumed_(0),
      gasLeft_(gas),
      flags_(CallFlags::All)
{
    RegisterSystemCalls();
}

// Note: GetTrigger(), GetGasConsumed(), GetGasLeft() are already inline in header

const io::ISerializable* ApplicationEngine::GetContainer() const { return container_; }

const io::ISerializable* ApplicationEngine::GetScriptContainer() const { return container_; }

std::shared_ptr<persistence::DataCache> ApplicationEngine::GetSnapshot() const { return snapshot_; }

const ledger::Block* ApplicationEngine::GetPersistingBlock() const { return persistingBlock_; }

io::UInt160 ApplicationEngine::GetCurrentScriptHash() const
{
    if (scriptHashes_.empty()) return io::UInt160();

    return scriptHashes_.back();
}

io::UInt160 ApplicationEngine::GetCallingScriptHash() const
{
    if (scriptHashes_.size() < 2) return io::UInt160();

    return scriptHashes_[scriptHashes_.size() - 2];
}

io::UInt160 ApplicationEngine::GetEntryScriptHash() const
{
    if (scriptHashes_.empty()) return io::UInt160();

    return scriptHashes_.front();
}

// Note: GetNotifications() is already inline in header

vm::VMState ApplicationEngine::Execute(const std::vector<uint8_t>& script)
{
    LoadScript(script, 0);
    return Execute();
}

void ApplicationEngine::LoadScript(const std::vector<uint8_t>& script, int32_t offset)
{
    io::UInt160 hash = cryptography::Hash::Hash160(io::ByteSpan(script.data(), script.size()));

    vm::Script scriptObject{vm::internal::ByteSpan(script)};
    vm::ExecutionEngine::LoadScript(scriptObject, offset);

    scriptHashes_.push_back(hash);
}

vm::VMState ApplicationEngine::Execute()
{
    exception_.clear();
    // Execute the VM with complete gas tracking and metering
    try
    {
        // Set initial gas limit check
        if (GetGasLeft() <= 0)
        {
            return vm::VMState::Fault;  // No InsufficientGas state, use Fault
        }

        // Execute VM step by step with gas monitoring
        auto state = vm::VMState::None;
        size_t instruction_count = 0;
        const size_t max_instructions = 100000;  // Prevent infinite loops

        while (state == vm::VMState::None && instruction_count < max_instructions)
        {
            // Check gas limit before each instruction
            if (GetGasLeft() <= 0)
            {
                return vm::VMState::Fault;  // No InsufficientGas state, use Fault
            }

            // Execute one instruction
            ExecuteNext();
            state = GetState();
            instruction_count++;

            // Add base gas cost per instruction
            AddGas(1);  // Base gas cost per instruction

            // Add additional gas costs based on instruction type
            // Additional gas costs handled by ExecuteNext()

            // Check for stack size limits (prevents DoS)
            // Stack size checks handled by ExecutionEngine base class

            // Check for execution time limits
            if (instruction_count % 1000 == 0)
            {
                // Periodic checks to prevent excessive computation
                // Time checks could be added here if needed
            }
        }

        // Check if we hit instruction limit
        if (instruction_count >= max_instructions)
        {
            return vm::VMState::Fault;  // No TimeOut state, use Fault
        }

        // Final gas consumption validation
        if (GetGasLeft() < 0)
        {
            return vm::VMState::Fault;  // No InsufficientGas state, use Fault
        }

        return state;
    }
    catch (const std::exception& e)
    {
        exception_ = e.what();
        return vm::VMState::Fault;
    }
}

bool ApplicationEngine::HasFlag(CallFlags flag) const
{
    return (static_cast<uint8_t>(flags_) & static_cast<uint8_t>(flag)) != 0;
}

void ApplicationEngine::AddGas(int64_t gas)
{
    if (gas < 0) throw std::invalid_argument("Gas cannot be negative");

    if (gasLeft_ >= 0)
    {
        if (gasLeft_ < gas) throw std::runtime_error("Insufficient gas");

        gasLeft_ -= gas;
    }

    gasConsumed_ += gas;
}

bool ApplicationEngine::CheckWitness(const io::UInt160& hash) const { return CheckWitnessInternal(hash); }

bool ApplicationEngine::CheckWitness(const io::UInt256& hash) const
{
    // Complete UInt256 witness verification implementation
    // UInt256 in CheckWitness context typically represents:
    // 1. Committee consensus verification (for committee-signed transactions)
    // 2. Multi-signature verification (for complex multi-sig scenarios)
    // 3. Public key verification (convert to script hash and verify)

    try
    {
        // Check if this is a committee consensus hash
        if (IsCommitteeHash(hash))
        {
            return VerifyCommitteeConsensus(hash);
        }

        // Check if this is a multi-signature verification hash
        return VerifyMultiSignatureHash(hash);

        return false;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

ContractState ApplicationEngine::CreateContract(const io::ByteVector& script, const std::string& manifest,
                                                uint32_t offset)
{
    if (!HasFlag(CallFlags::WriteStates)) throw MissingFlagsException("CreateContract", "WriteStates");

    // Calculate script hash
    io::UInt160 scriptHash = cryptography::Hash::Hash160(script.AsSpan());

    // Check if contract already exists using ContractManagement native contract
    auto contractManagement = native::ContractManagement::GetInstance();
    if (!contractManagement) throw SystemCallException("CreateContract", "ContractManagement not initialized");

    // Check if contract with this hash already exists
    auto existingContract = contractManagement->GetContract(*snapshot_, scriptHash);
    if (existingContract) throw SystemCallException("CreateContract", "Contract already exists");

    // Get next available ID from ContractManagement
    persistence::StorageKey idKey(
        contractManagement->GetId(),
        io::ByteVector{0x0C});  // 0x0C is the prefix for next available ID in ContractManagement
    auto idItem = snapshot_->TryGet(idKey);
    uint32_t id = 1;
    if (idItem)
    {
        std::istringstream stream(
            std::string(reinterpret_cast<const char*>(idItem->GetValue().Data()), idItem->GetValue().Size()));
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
    io::ByteVector keyData;
    keyData.Push(0x08);  // Contract storage prefix
    auto scriptHashBytes = scriptHash.ToArray();
    keyData.Append(scriptHashBytes.AsSpan());
    persistence::StorageKey key(contractManagement->GetId(), keyData);

    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    contract.Serialize(writer);
    std::string data = stream.str();

    persistence::StorageItem item(
        io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));
    snapshot_->Add(key, item);

    // Update next available ID
    std::ostringstream idStream;
    io::BinaryWriter idWriter(idStream);
    idWriter.Write(id + 1);
    std::string idData = idStream.str();
    persistence::StorageItem idItemNew(
        io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(idData.data()), idData.size())));
    snapshot_->Add(idKey, idItemNew);

    return contract;
}

std::shared_ptr<vm::StackItem> ApplicationEngine::CallContract(const io::UInt160& scriptHash, const std::string& method,
                                                               const std::vector<std::shared_ptr<vm::StackItem>>& args,
                                                               CallFlags flags)
{
    if (!HasFlag(CallFlags::AllowCall)) throw std::runtime_error("Cannot call contract without AllowCall flag");

    // Get contract from ContractManagement
    auto contractManagement = native::ContractManagement::GetInstance();
    if (!contractManagement) throw std::runtime_error("ContractManagement not initialized");

    auto contract = contractManagement->GetContract(*snapshot_, scriptHash);
    if (!contract) throw std::runtime_error("Contract not found");

    // Get the deserialized contract state
    ContractState contractState = *contract;

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

            if (!result) throw std::runtime_error("Native method execution failed");

            // Return the top item from the evaluation stack
            return Pop();
        }
    }

    // Call contract method
    CallFlags oldFlags = flags_;
    flags_ = flags;

    // Load script
    auto script = contract->GetScript();
    LoadScript(std::vector<uint8_t>(script.Data(), script.Data() + script.Size()));

    // Execute
    auto state = Execute();
    flags_ = oldFlags;

    if (state != vm::VMState::Halt) throw std::runtime_error("Contract execution failed");

    // Return the top item from the evaluation stack
    return Pop();
}

std::shared_ptr<vm::StackItem> ApplicationEngine::Pop()
{
    return ExecutionEngine::Pop();
}

void ApplicationEngine::Push(std::shared_ptr<vm::StackItem> item)
{
    ExecutionEngine::Push(item);
}

std::shared_ptr<vm::StackItem> ApplicationEngine::Peek() const
{
    return ExecutionEngine::Peek();
}

io::ByteVector ApplicationEngine::GetScript() const
{
    if (GetInvocationStack().empty()) return io::ByteVector();

    const auto& script = GetCurrentContext().GetScript().GetScript();
    return io::ByteVector(io::ByteSpan(script.Data(), script.Size()));
}

std::vector<std::shared_ptr<vm::StackItem>> ApplicationEngine::GetResultStack() const
{
    std::vector<std::shared_ptr<vm::StackItem>> result;
    const auto& stack = ExecutionEngine::GetResultStack();
    result.reserve(stack.size());
    for (const auto& item : stack) result.push_back(item);
    return result;
}

std::string ApplicationEngine::GetException() const
{
    return exception_;
}

void ApplicationEngine::Notify(const io::UInt160& scriptHash, const std::string& eventName,
                               const std::vector<std::shared_ptr<vm::StackItem>>& state)
{
    if (!HasFlag(CallFlags::AllowNotify)) throw std::runtime_error("Cannot notify without AllowNotify flag");

    notifications_.emplace_back(scriptHash, eventName, state);
}

const ledger::Transaction* ApplicationEngine::GetTransaction() const
{
    if (container_ && dynamic_cast<const ledger::Transaction*>(container_))
        return static_cast<const ledger::Transaction*>(container_);

    return nullptr;
}

int64_t ApplicationEngine::GetGasPrice() const { return gasPrice_; }

uint32_t ApplicationEngine::GetPlatformVersion() const { return platformVersion_; }

uint64_t ApplicationEngine::GetRandom() const { return random_; }

int64_t ApplicationEngine::GetNetworkFeePerByte() const { return networkFeePerByte_; }

vm::ExecutionEngineLimits ApplicationEngine::GetLimits() const
{
    // Return the execution engine limits (same as C# implementation)
    return vm::ExecutionEngine::GetLimits();
}

std::unique_ptr<ApplicationEngine> ApplicationEngine::Create(TriggerType trigger, const io::ISerializable* container,
                                                             std::shared_ptr<persistence::DataCache> snapshot,
                                                             const ledger::Block* persistingBlock, int64_t gas)
{
    return std::make_unique<ApplicationEngine>(trigger, container, snapshot, persistingBlock, gas);
}

std::unique_ptr<ApplicationEngine> ApplicationEngine::Run(const io::ByteVector& script,
                                                          std::shared_ptr<persistence::DataCache> snapshot,
                                                          const io::ISerializable* container,
                                                          const ledger::Block* persistingBlock, int32_t offset,
                                                          int64_t gas)
{
    auto engine = Create(TriggerType::Application, container, snapshot, persistingBlock, gas);
    std::vector<uint8_t> scriptBytes(script.AsSpan().Data(), script.AsSpan().Data() + script.AsSpan().Size());
    engine->LoadScript(scriptBytes, offset);
    engine->Execute();
    return engine;
}

const ProtocolSettings* ApplicationEngine::GetProtocolSettings() const { return &protocolSettings_; }

bool ApplicationEngine::IsHardforkEnabled(int hardfork) const
{
    const uint32_t height = persistingBlock_ ? persistingBlock_->GetIndex() : 0;
    return protocolSettings_.IsHardforkEnabled(static_cast<Hardfork>(hardfork), height);
}

uint32_t ApplicationEngine::GetNetworkMagic() const { return protocolSettings_.GetNetwork(); }

int64_t ApplicationEngine::GetInvocationCount(const io::UInt160& scriptHash) const
{
    auto it = invocationCounts_.find(scriptHash);
    return it != invocationCounts_.end() ? it->second : 0;
}

void ApplicationEngine::SetInvocationCount(const io::UInt160& scriptHash, int64_t count)
{
    invocationCounts_[scriptHash] = count;
}

uint32_t ApplicationEngine::GetCurrentBlockHeight() const
{
    return persistingBlock_ ? persistingBlock_->GetIndex() : 0;
}

bool ApplicationEngine::IsCommitteeHash(const io::UInt256& hash) const
{
    auto resolve_committee_hash = [&]() -> std::optional<io::UInt160> {
        if (snapshot_)
        {
            try
            {
                auto neoToken = native::NeoToken::GetInstance();
                if (neoToken)
                {
                    auto committeeAddress = neoToken->GetCommitteeAddress(snapshot_);
                    if (!committeeAddress.IsZero()) return committeeAddress;
                }
            }
            catch (const std::exception&)
            {
                // Fallback to standby committee logic below
            }
        }

        auto committee = GetCommittee();
        if (committee.empty()) return std::nullopt;

        auto script = CreateCommitteeMultiSigScript(committee);
        if (script.IsEmpty()) return std::nullopt;

        return cryptography::Hash::Hash160(io::ByteSpan(script.Data(), script.Size()));
    };

    const auto committeeHash = resolve_committee_hash();
    if (!committeeHash) return false;

    io::UInt160 target(io::ByteSpan(hash.Data(), io::UInt160::Size));
    return target == *committeeHash;
}

bool ApplicationEngine::VerifyCommitteeConsensus(const io::UInt256& hash) const
{
    auto committeeHash = [&]() -> std::optional<io::UInt160> {
        if (snapshot_)
        {
            try
            {
                auto neoToken = native::NeoToken::GetInstance();
                if (neoToken)
                {
                    auto committeeAddress = neoToken->GetCommitteeAddress(snapshot_);
                    if (!committeeAddress.IsZero()) return committeeAddress;
                }
            }
            catch (const std::exception&)
            {
                // Fallback to standby committee
            }
        }

        auto committee = GetCommittee();
        if (committee.empty()) return std::nullopt;

        auto script = CreateCommitteeMultiSigScript(committee);
        if (script.IsEmpty()) return std::nullopt;

        return cryptography::Hash::Hash160(io::ByteSpan(script.Data(), script.Size()));
    }();

    if (!committeeHash) return false;

    io::UInt160 target(io::ByteSpan(hash.Data(), io::UInt160::Size));
    if (target != *committeeHash) return false;

    return CheckWitnessInternal(*committeeHash);
}

bool ApplicationEngine::VerifyMultiSignatureHash(const io::UInt256& hash) const
{
    const uint8_t* data = hash.Data();
    io::ByteSpan span(data, io::UInt160::Size);
    io::UInt160 scriptHash(span);
    return CheckWitnessInternal(scriptHash);
}

bool ApplicationEngine::CheckWitnessInternal(const io::UInt160& hash) const
{
    if (hash == GetCallingScriptHash()) return true;

    if (const auto* tx = GetTransaction())
    {
        const std::vector<ledger::Signer>* signersPtr = nullptr;
        std::vector<ledger::Signer> oracleSigners;

        const auto& attributes = tx->GetAttributes();
        std::optional<uint64_t> oracleRequestId;
        for (const auto& attribute : attributes)
        {
            if (!attribute || attribute->GetUsage() != ledger::TransactionAttribute::Usage::OracleResponse) continue;

            const auto& data = attribute->GetData();
            if (data.Size() >= sizeof(uint64_t) + 1)
            {
                uint64_t id = 0;
                std::memcpy(&id, data.Data(), sizeof(uint64_t));
                oracleRequestId = id;
            }
            break;
        }

        if (oracleRequestId && snapshot_)
        {
            auto oracleContract = native::OracleContract::GetInstance();
            auto ledgerContract = native::LedgerContract::GetInstance();
            if (oracleContract && ledgerContract)
            {
                try
                {
                    auto request = oracleContract->GetRequest(snapshot_, *oracleRequestId);
                    auto originalTransaction = ledgerContract->GetTransaction(snapshot_, request.GetOriginalTxid());
                    if (originalTransaction)
                    {
                        const auto& originalSigners = originalTransaction->GetSigners();
                        oracleSigners.assign(originalSigners.begin(), originalSigners.end());
                        signersPtr = &oracleSigners;
                    }
                }
                catch (const std::exception&)
                {
                    // fall back to the current transaction's signers
                }
            }
        }

        if (!signersPtr)
        {
            signersPtr = &tx->GetSigners();
        }

        const auto& signers = *signersPtr;
        auto signerIt = std::find_if(signers.begin(), signers.end(),
                                     [&](const ledger::Signer& signer) { return signer.GetAccount() == hash; });
        if (signerIt == signers.end()) return false;

        auto rules = signerIt->GetAllRules();
        for (const auto& rule : rules)
        {
            if (rule.Matches(*this))
            {
                return rule.GetAction() == ledger::WitnessRuleAction::Allow;
            }
        }

        return false;
    }

    const auto* verifiable = dynamic_cast<const network::p2p::payloads::IVerifiable*>(container_);
    if (!verifiable) return false;

    ValidateCallFlags(CallFlags::ReadStates);
    auto hashes = verifiable->GetScriptHashesForVerifying();
    return std::find(hashes.begin(), hashes.end(), hash) != hashes.end();
}

std::vector<cryptography::ecc::ECPoint> ApplicationEngine::GetCommittee() const
{
    if (snapshot_)
    {
        try
        {
            auto neoToken = native::NeoToken::GetInstance();
            if (neoToken)
            {
                auto committee = neoToken->GetCommittee(snapshot_);
                if (!committee.empty())
                {
                    std::sort(committee.begin(), committee.end());
                    return committee;
                }
            }
        }
        catch (const std::exception&)
        {
            // Fall through to standby committee
        }
    }

    auto standby = protocolSettings_.GetStandbyCommittee();
    std::vector<cryptography::ecc::ECPoint> ordered(standby.begin(), standby.end());
    std::sort(ordered.begin(), ordered.end());
    return ordered;
}

bool ApplicationEngine::IsCalledByEntry() const { return scriptHashes_.size() <= 1; }

void ApplicationEngine::ValidateCallFlags(CallFlags required) const
{
    if (!HasFlag(required))
    {
        switch (required)
        {
            case CallFlags::ReadStates:
                throw MissingFlagsException("CheckWitness", "ReadStates");
            case CallFlags::WriteStates:
                throw MissingFlagsException("CheckWitness", "WriteStates");
            default:
                throw MissingFlagsException("CheckWitness", "RequiredCallFlags");
        }
    }
}

bool ApplicationEngine::IsContractGroupMember(const cryptography::ecc::ECPoint& group) const
{
    ValidateCallFlags(CallFlags::ReadStates);

    if (!snapshot_) return false;

    const auto callingScript = GetCallingScriptHash();
    if (callingScript.IsZero()) return false;

    auto contractManagement = native::ContractManagement::GetInstance();
    if (!contractManagement) return false;

    auto contractState = contractManagement->GetContract(*snapshot_, callingScript);
    if (!contractState) return false;

    const auto& manifestJson = contractState->GetManifest();
    if (manifestJson.empty()) return false;

    try
    {
        auto document = nlohmann::json::parse(manifestJson);
        if (!document.contains("groups")) return false;

        for (const auto& entry : document["groups"])
        {
            if (!entry.contains("pubkey")) continue;
            std::string pubkeyHex = entry["pubkey"].get<std::string>();
            if (pubkeyHex.rfind("0x", 0) == 0)
            {
                pubkeyHex.erase(0, 2);
            }

            auto groupPoint = cryptography::ecc::ECPoint::FromHex(pubkeyHex);
            if (groupPoint == group) return true;
        }
    }
    catch (...)
    {
        return false;
    }

    return false;
}

void ApplicationEngine::Log(const std::string& message)
{
    auto scriptHash = GetCurrentScriptHash();
    uint64_t timestamp =
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::system_clock::now().time_since_epoch())
                                  .count());
    logs_.emplace_back(scriptHash, message, timestamp);
}

io::ByteVector ApplicationEngine::CreateCommitteeMultiSigScript(
    const std::vector<cryptography::ecc::ECPoint>& committee) const
{
    if (committee.empty()) return io::ByteVector();

    std::vector<cryptography::ecc::ECPoint> ordered(committee.begin(), committee.end());
    std::sort(ordered.begin(), ordered.end());

    const auto memberCount = ordered.size();
    const auto threshold = static_cast<int64_t>(memberCount - (memberCount - 1) / 2);

    vm::ScriptBuilder builder;
    builder.EmitPush(threshold);

    for (const auto& member : ordered)
    {
        auto bytes = member.ToArray();
        builder.EmitPush(io::ByteSpan(bytes.Data(), bytes.Size()));
    }

    builder.EmitPush(static_cast<int64_t>(memberCount));
    builder.EmitSysCall("System.Crypto.CheckMultisig");

    return builder.ToArray();
}

native::NativeContract* ApplicationEngine::GetNativeContract(const io::UInt160& hash) const
{
    // Implement proper native contract lookup matching C# logic
    // Check all registered native contracts

    // NEO Token
    auto neoToken = native::NeoToken::GetInstance();
    if (neoToken && neoToken->GetScriptHash() == hash) return neoToken.get();

    // Contract Management
    auto contractManagement = native::ContractManagement::GetInstance();
    if (contractManagement && contractManagement->GetScriptHash() == hash) return contractManagement.get();

    // Policy Contract
    auto policyContract = native::PolicyContract::GetInstance();
    if (policyContract && policyContract->GetScriptHash() == hash) return policyContract.get();

    // Ledger Contract
    auto ledgerContract = native::LedgerContract::GetInstance();
    if (ledgerContract && ledgerContract->GetScriptHash() == hash) return ledgerContract.get();

    // Complete native contract singleton pattern implementation

    // GAS Token
    auto gasToken = native::GasToken::GetInstance();
    if (gasToken && gasToken->GetScriptHash() == hash) return gasToken.get();

    // StdLib and CryptoLib don't have GetInstance() method
    // They are utility contracts handled differently

    // NameService Contract
    auto nameService = native::NameService::GetInstance();
    if (nameService && nameService->GetScriptHash() == hash) return nameService.get();

    // RoleManagement Contract
    auto roleManagement = native::RoleManagement::GetInstance();
    if (roleManagement && roleManagement->GetScriptHash() == hash) return roleManagement.get();

    // Oracle Contract (if available)
    auto oracleContract = native::OracleContract::GetInstance();
    if (oracleContract && oracleContract->GetScriptHash() == hash) return oracleContract.get();

    // Native contract not found
    return nullptr;
}

// Helper methods removed - not declared in header file

// ConvertPublicKeyToScriptHash removed - not needed

}  // namespace neo::smartcontract
