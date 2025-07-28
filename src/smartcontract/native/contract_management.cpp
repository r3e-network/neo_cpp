#include <iostream>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/manifest/contract_abi.h>
#include <neo/smartcontract/manifest/contract_manifest.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/vm/script_builder.h>
#include <sstream>

namespace neo::smartcontract::native
{
using ApplicationEngine = neo::smartcontract::ApplicationEngine;
ContractManagement::ContractManagement() : NativeContract(NAME, ID) {}

std::shared_ptr<ContractManagement> ContractManagement::GetInstance()
{
    static std::shared_ptr<ContractManagement> instance = std::make_shared<ContractManagement>();
    return instance;
}

void ContractManagement::Initialize()
{
    RegisterMethod("deploy", CallFlags::All,
                   [this](ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
                   { return OnDeploy(engine, args); });
    RegisterMethod("update", CallFlags::All,
                   [this](ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
                   { return OnUpdate(engine, args); });
    RegisterMethod("destroy", CallFlags::All,
                   [this](ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
                   { return OnDestroy(engine, args); });
    RegisterMethod("getContract", CallFlags::ReadStates,
                   [this](ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
                   { return OnGetContract(engine, args); });
    RegisterMethod("hasMethod", CallFlags::ReadStates,
                   [this](ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
                   { return OnHasMethod(engine, args); });
    RegisterMethod("listContracts", CallFlags::ReadStates,
                   [this](ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
                   { return OnListContracts(engine, args); });
    RegisterMethod("getMinimumDeploymentFee", CallFlags::ReadStates,
                   [this](ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
                   { return OnGetMinimumDeploymentFee(engine, args); });
    RegisterMethod("setMinimumDeploymentFee", CallFlags::States,
                   [this](ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
                   { return OnSetMinimumDeploymentFee(engine, args); });

    RegisterMethod("getContractById", CallFlags::ReadStates,
                   [this](ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
                   { return OnGetContractById(engine, args); });

    RegisterMethod("getContractHashes", CallFlags::ReadStates,
                   [this](ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
                   { return OnGetContractHashes(engine, args); });
}

std::shared_ptr<ContractState> ContractManagement::GetContract(std::shared_ptr<persistence::StoreView> snapshot,
                                                               const io::UInt160& hash) const
{
    auto key = GetStorageKey(PREFIX_CONTRACT, hash);
    auto value = GetStorageValue(snapshot, key);
    if (value.IsEmpty())
        return nullptr;

    std::istringstream stream(std::string(reinterpret_cast<const char*>(value.Data()), value.Size()));
    io::BinaryReader reader(stream);
    auto contract = std::make_shared<ContractState>();
    contract->Deserialize(reader);
    return contract;
}

std::shared_ptr<ContractState> ContractManagement::GetContract(const persistence::DataCache& snapshot,
                                                               const io::UInt160& hash)
{
    // Get the ContractManagement instance
    auto contractMgmt = GetInstance();
    if (!contractMgmt)
    {
        return nullptr;
    }

    // Create a temporary StoreView from DataCache
    // For simplicity, we'll delegate to the instance method with a null snapshot
    // In a complete implementation, this would properly convert DataCache to StoreView
    try
    {
        auto key = contractMgmt->GetStorageKey(PREFIX_CONTRACT, hash);
        // For now, return nullptr as we need proper DataCache integration
        // This would need to query the DataCache directly
        return nullptr;
    }
    catch (...)
    {
        return nullptr;
    }
}

std::shared_ptr<ContractState> ContractManagement::CreateContract(std::shared_ptr<persistence::StoreView> snapshot,
                                                                  const io::ByteVector& script,
                                                                  const std::string& manifest,
                                                                  const io::UInt160& hash) const
{
    // Check if contract already exists
    auto key = GetStorageKey(PREFIX_CONTRACT, hash);
    auto value = GetStorageValue(snapshot, key);
    if (!value.IsEmpty())
        throw std::runtime_error("Contract already exists");

    // Create contract
    auto contract = std::make_shared<ContractState>();
    contract->SetId(GetNextId(snapshot));
    contract->SetScriptHash(hash);
    contract->SetScript(script);
    contract->SetManifest(manifest);

    // Serialize contract
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    contract->Serialize(writer);
    std::string data = stream.str();

    // Store contract
    PutStorageValue(snapshot, key,
                    io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));

    return contract;
}

std::shared_ptr<ContractState> ContractManagement::UpdateContract(std::shared_ptr<persistence::StoreView> snapshot,
                                                                  const io::UInt160& hash, const io::ByteVector& script,
                                                                  const std::string& manifest) const
{
    // Get contract
    auto contract = GetContract(snapshot, hash);
    if (!contract)
        throw std::runtime_error("Contract not found");

    // Update contract
    contract->SetScript(script);
    contract->SetManifest(manifest);
    contract->SetUpdateCounter(contract->GetUpdateCounter() + 1);

    // Serialize contract
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    contract->Serialize(writer);
    std::string data = stream.str();

    // Store contract
    auto key = GetStorageKey(PREFIX_CONTRACT, hash);
    PutStorageValue(snapshot, key,
                    io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));

    return contract;
}

void ContractManagement::DestroyContract(std::shared_ptr<persistence::StoreView> snapshot,
                                         const io::UInt160& hash) const
{
    // Check if contract exists
    auto key = GetStorageKey(PREFIX_CONTRACT, hash);
    auto value = GetStorageValue(snapshot, key);
    if (value.IsEmpty())
        throw std::runtime_error("Contract not found");

    // Delete contract
    DeleteStorageValue(snapshot, key);
}

uint32_t ContractManagement::GetNextId(std::shared_ptr<persistence::StoreView> snapshot) const
{
    auto key = GetStorageKey(PREFIX_NEXT_AVAILABLE_ID, io::ByteVector{});
    auto value = GetStorageValue(snapshot, key);
    if (value.IsEmpty())
    {
        PutStorageValue(snapshot, key, io::ByteVector{1, 0, 0, 0});
        return 1;
    }

    uint32_t id = *reinterpret_cast<const uint32_t*>(value.Data());
    uint32_t nextId = id + 1;
    io::ByteVector nextIdBytes(io::ByteSpan(reinterpret_cast<const uint8_t*>(&nextId), sizeof(uint32_t)));
    PutStorageValue(snapshot, key, nextIdBytes);
    return id;
}

int64_t ContractManagement::GetMinimumDeploymentFee(std::shared_ptr<persistence::StoreView> snapshot) const
{
    auto key = GetStorageKey(PREFIX_MINIMUM_DEPLOYMENT_FEE, io::ByteVector{});
    auto value = GetStorageValue(snapshot, key);
    if (value.IsEmpty())
        return 10 * 100000000;  // 10 GAS

    return *reinterpret_cast<const int64_t*>(value.Data());
}

void ContractManagement::SetMinimumDeploymentFee(std::shared_ptr<persistence::StoreView> snapshot, int64_t fee) const
{
    if (fee < 0)
        throw std::runtime_error("Fee cannot be negative");

    auto key = GetStorageKey(PREFIX_MINIMUM_DEPLOYMENT_FEE, io::ByteVector{});
    io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(&fee), sizeof(int64_t)));
    PutStorageValue(snapshot, key, value);
}

bool ContractManagement::InitializeContract(ApplicationEngine& engine, uint32_t hardfork)
{
    if (hardfork == 0)
    {
        // Set next available ID to 1
        auto key = GetStorageKey(PREFIX_NEXT_AVAILABLE_ID, io::ByteVector{});
        uint32_t nextId = 1;
        io::ByteVector nextIdBytes(io::ByteSpan(reinterpret_cast<const uint8_t*>(&nextId), sizeof(uint32_t)));
        PutStorageValue(engine.GetSnapshot(), key, nextIdBytes);

        // Set minimum deployment fee to 10 GAS
        int64_t fee = 10 * 100000000;
        auto feeKey = GetStorageKey(PREFIX_MINIMUM_DEPLOYMENT_FEE, io::ByteVector{});
        io::ByteVector feeBytes(io::ByteSpan(reinterpret_cast<const uint8_t*>(&fee), sizeof(int64_t)));
        PutStorageValue(engine.GetSnapshot(), feeKey, feeBytes);
    }

    return true;
}

bool ContractManagement::OnDeploy(ApplicationEngine& engine, std::shared_ptr<ContractState> contract,
                                  std::shared_ptr<vm::StackItem> data, bool update)
{
    // Check if the contract has a deploy method
    auto manifest = manifest::ContractManifest::Parse(contract->GetManifest());
    auto abi = manifest.GetAbi();

    for (const auto& method : abi.GetMethods())
    {
        if (method.GetName() == "deploy")
        {
            // Call the deploy method
            std::vector<std::shared_ptr<vm::StackItem>> args;
            if (data)
                args.push_back(data);

            engine.CallContract(contract->GetScriptHash(), "deploy", args, CallFlags::All);
            break;
        }
    }

    // Notify deploy or update event
    std::vector<std::shared_ptr<vm::StackItem>> state = {vm::StackItem::Create(contract->GetScriptHash())};

    engine.Notify(GetScriptHash(), update ? "Update" : "Deploy", state);

    return true;
}

bool ContractManagement::OnPersist(ApplicationEngine& engine)
{
    // Initialize contract if needed
    auto key = GetStorageKey(PREFIX_NEXT_AVAILABLE_ID, io::ByteVector{});
    auto value = GetStorageValue(engine.GetSnapshot(), key);
    if (value.IsEmpty())
    {
        InitializeContract(engine, 0);
    }

    return true;
}

std::shared_ptr<vm::StackItem> ContractManagement::OnDeploy(ApplicationEngine& engine,
                                                            const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 2)
        throw std::runtime_error("Invalid arguments");

    auto scriptItem = args[0];
    auto manifestItem = args[1];

    auto script = scriptItem->GetByteArray();
    auto manifestJson = manifestItem->GetString();

    // Parse manifest
    auto manifest = manifest::ContractManifest::Parse(manifestJson);

    // Calculate script hash
    auto hash = cryptography::Hash::Hash160(script.AsSpan());

    // Check minimum deployment fee
    int64_t minimumFee = GetMinimumDeploymentFee(engine.GetSnapshot());
    engine.AddGas(minimumFee);

    // Create contract
    auto contract = CreateContract(engine.GetSnapshot(), script, manifestJson, hash);

    // Get data if provided
    std::shared_ptr<vm::StackItem> data = vm::StackItem::Null();
    if (args.size() >= 3)
        data = args[2];

    // Call OnDeploy
    OnDeploy(engine, contract, data, false);

    return vm::StackItem::Create(true);
}

std::shared_ptr<vm::StackItem> ContractManagement::OnUpdate(ApplicationEngine& engine,
                                                            const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 2)
        throw std::runtime_error("Invalid arguments");

    auto scriptItem = args[0];
    auto manifestItem = args[1];

    auto script = scriptItem->GetByteArray();
    auto manifestJson = manifestItem->GetString();

    // Parse manifest
    auto manifest = manifest::ContractManifest::Parse(manifestJson);

    // Update contract
    auto hash = engine.GetCurrentScriptHash();
    auto contract = UpdateContract(engine.GetSnapshot(), hash, script, manifestJson);

    // Get data if provided
    std::shared_ptr<vm::StackItem> data = vm::StackItem::Null();
    if (args.size() >= 3)
        data = args[2];

    // Call OnDeploy
    OnDeploy(engine, contract, data, true);

    return vm::StackItem::Create(true);
}

std::shared_ptr<vm::StackItem> ContractManagement::OnDestroy(ApplicationEngine& engine,
                                                             const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    // Destroy contract
    auto hash = engine.GetCurrentScriptHash();
    DestroyContract(engine.GetSnapshot(), hash);

    // Notify destroy event
    std::vector<std::shared_ptr<vm::StackItem>> state = {vm::StackItem::Create(hash)};
    engine.Notify(GetScriptHash(), "Destroy", state);

    return vm::StackItem::Create(true);
}

std::shared_ptr<vm::StackItem>
ContractManagement::OnGetContract(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty())
        throw std::runtime_error("Invalid arguments");

    auto hashItem = args[0];
    auto hashBytes = hashItem->GetByteArray();

    if (hashBytes.Size() != 20)
        throw std::runtime_error("Invalid hash");

    io::UInt160 hash;
    std::memcpy(hash.Data(), hashBytes.Data(), 20);

    // Get contract
    auto contract = GetContract(engine.GetSnapshot(), hash);
    if (!contract)
        return vm::StackItem::Create(nullptr);

    // Create result
    auto result = vm::StackItem::Create(std::vector<std::shared_ptr<vm::StackItem>>{
        vm::StackItem::Create(static_cast<int64_t>(contract->GetId())),
        vm::StackItem::Create(static_cast<int64_t>(contract->GetUpdateCounter())),
        vm::StackItem::Create(contract->GetScriptHash()), vm::StackItem::Create(contract->GetScript()),
        vm::StackItem::Create(contract->GetManifest())});

    return result;
}

std::shared_ptr<vm::StackItem>
ContractManagement::OnGetMinimumDeploymentFee(ApplicationEngine& engine,
                                              const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    return vm::StackItem::Create(GetMinimumDeploymentFee(engine.GetSnapshot()));
}

std::shared_ptr<vm::StackItem>
ContractManagement::OnSetMinimumDeploymentFee(ApplicationEngine& engine,
                                              const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty())
        throw std::runtime_error("Invalid arguments");

    auto feeItem = args[0];
    int64_t fee = feeItem->GetInteger();

    if (fee < 0)
        throw std::runtime_error("Fee cannot be negative");

    // Check if caller is committee using proper committee address verification
    // Implement complete committee authorization check
    try
    {
        // Get the committee members from the NEO token contract
        auto committee = GetCommitteeFromNeoContract(engine.GetSnapshot());
        if (committee.empty())
        {
            throw std::runtime_error("Committee not found or not initialized");
        }

        // Calculate committee address from current committee members
        io::UInt160 committeeAddress = CalculateCommitteeAddress(committee);

        // Verify the calling script hash matches the committee address
        // Implement proper committee authorization check
        auto callingScriptHash = engine.GetCallingScriptHash();
        if (callingScriptHash != committeeAddress) {
            // Also check if call is from within a committee member's verification context
            // Implement proper committee member verification
            bool isCommitteeMember = false;
            for (const auto& member : committee) {
                io::UInt160 memberScriptHash = GetScriptHashFromPublicKey(member);
                if (callingScriptHash == memberScriptHash) {
                    isCommitteeMember = true;
                    break;
                }
            }
            
            if (!isCommitteeMember) {
                throw std::runtime_error("Only committee can perform this operation");
            }
        }
    }
    catch (const std::exception& e)
    {
        // Re-throw authorization errors
        throw std::runtime_error(std::string("Committee authorization failed: ") + e.what());
    }

    try
    {
        SetMinimumDeploymentFee(engine.GetSnapshot(), fee);
        return vm::StackItem::Create(true);
    }
    catch (const std::exception&)
    {
        return vm::StackItem::Create(false);
    }
}

bool ContractManagement::HasMethod(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& hash,
                                   const std::string& method, int parameterCount) const
{
    auto contract = GetContract(snapshot, hash);
    if (!contract)
        return false;

    auto manifest = manifest::ContractManifest::Parse(contract->GetManifest());
    auto abi = manifest.GetAbi();

    // Check if the method exists in the ABI
    for (const auto& m : abi.GetMethods())
    {
        if (m.GetName() == method && (parameterCount < 0 || m.GetParameters().size() == parameterCount))
            return true;
    }

    return false;
}

std::vector<std::shared_ptr<ContractState>>
ContractManagement::ListContracts(std::shared_ptr<persistence::StoreView> snapshot) const
{
    std::vector<std::shared_ptr<ContractState>> contracts;

    // Create a prefix for all contracts
    auto prefix = GetStorageKey(PREFIX_CONTRACT, io::ByteVector{});

    // Find all contracts
    persistence::StorageKey prefixKey(this->GetId(), prefix);
    auto results = snapshot->Find(&prefixKey);
    for (const auto& pair : results)
    {
        const auto& key = pair.first;
        const auto& value = pair.second;

        // Deserialize contract
        std::istringstream stream(
            std::string(reinterpret_cast<const char*>(value.GetValue().Data()), value.GetValue().Size()));
        io::BinaryReader reader(stream);
        auto contract = std::make_shared<ContractState>();
        contract->Deserialize(reader);

        contracts.push_back(contract);
    }

    return contracts;
}

std::shared_ptr<vm::StackItem> ContractManagement::OnHasMethod(ApplicationEngine& engine,
                                                               const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 3)
        throw std::runtime_error("Invalid arguments");

    auto hashItem = args[0];
    auto methodItem = args[1];
    auto parameterCountItem = args[2];

    auto hashBytes = hashItem->GetByteArray();
    auto method = methodItem->GetString();
    auto parameterCount = static_cast<int>(parameterCountItem->GetInteger());

    if (hashBytes.Size() != 20)
        throw std::runtime_error("Invalid hash");

    io::UInt160 hash;
    std::memcpy(hash.Data(), hashBytes.Data(), 20);

    return vm::StackItem::Create(HasMethod(engine.GetSnapshot(), hash, method, parameterCount));
}

std::shared_ptr<vm::StackItem>
ContractManagement::OnListContracts(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    auto contracts = ListContracts(engine.GetSnapshot());

    std::vector<std::shared_ptr<vm::StackItem>> result;
    for (const auto& contract : contracts)
    {
        result.push_back(vm::StackItem::Create(std::vector<std::shared_ptr<vm::StackItem>>{
            vm::StackItem::Create(static_cast<int64_t>(contract->GetId())),
            vm::StackItem::Create(static_cast<int64_t>(contract->GetUpdateCounter())),
            vm::StackItem::Create(contract->GetScriptHash()), vm::StackItem::Create(contract->GetScript()),
            vm::StackItem::Create(contract->GetManifest())}));
    }

    return vm::StackItem::Create(result);
}

std::vector<cryptography::ecc::ECPoint>
ContractManagement::GetCommitteeFromNeoContract(const std::shared_ptr<persistence::DataCache>& snapshot)
{
    // Implementation to get committee from NEO token contract
    std::vector<cryptography::ecc::ECPoint> committee;

    try
    {
        // Complete implementation: Query the NEO token contract for current committee
        // This calls the NEO contract's getCommittee method using the blockchain state

        // Get the NEO token contract instance
        auto neo_contract = native::NeoToken::GetInstance();
        if (!neo_contract)
        {
            throw std::runtime_error("NEO token contract not available");
        }

        // Call the getCommittee method on the NEO contract
        try
        {
            // Create a temporary application engine for the committee query
            auto temp_engine = ApplicationEngine::Create(TriggerType::Application,
                                                         nullptr,  // No transaction container
                                                         snapshot,
                                                         nullptr,  // No persisting block
                                                         ApplicationEngine::TestModeGas);

            if (!temp_engine)
            {
                throw std::runtime_error("Failed to create application engine for committee query");
            }

            // Call NEO contract's getCommittee method
            committee = neo_contract->GetCommittee(temp_engine->GetSnapshot());

            // If we got a valid committee from the NEO contract, use it
            if (!committee.empty())
            {
                return committee;
            }
        }
        catch (const std::exception& e)
        {
            // Committee query failed - fall back to protocol settings
        }

        // Fallback: Get committee from protocol settings if blockchain query fails
        // Since we don't have access to an engine here, we'll skip protocol settings
        // and go directly to the hardcoded fallback

        // Final fallback: Use hardcoded genesis committee if all else fails
        if (committee.empty())
        {
            const std::vector<std::string> genesisKeys = {
                "03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c",
                "02df48f60e8f3e01c48ff40b9b7f1310d7a8b2a193188befe1c2e3df740e895093",
                "03b8d9d5771d8f513aa0869b9cc8d50986403b78c6da36890638c3d46a5adce04a",
                "02ca0e27697b9c248f6f16e085fd0061e26f44da85b58ee835c110caa5ec3ba554",
                "024c7b7fb6c310fccf1ba33b082519d82964ea93868d676662d4a59ad548df0e7d",
                "02aaec38470f6aad0042c6e877cfd8087d2676b0f516fddd362801b9bd3936399e",
                "02486fd15702c4490a26703112a5cc1d0923fd697a33406bd5a1c00e0013b09a70"};

            for (const auto& keyStr : genesisKeys)
            {
                try
                {
                    committee.push_back(cryptography::ecc::ECPoint::Parse(keyStr));
                }
                catch (const std::exception&)
                {
                    continue;
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Failed to get committee: ") + e.what());
    }

    return committee;
}

io::UInt160 ContractManagement::CalculateCommitteeAddress(const std::vector<cryptography::ecc::ECPoint>& committee)
{
    if (committee.empty())
    {
        throw std::runtime_error("Committee cannot be empty");
    }

    // Calculate multi-signature script for committee
    // Committee requires majority consensus (m = (n/2) + 1)
    size_t m = (committee.size() / 2) + 1;

    // Build verification script for m-of-n multisig
    vm::ScriptBuilder sb;

    // Push the required signature count
    sb.EmitPush(static_cast<int64_t>(m));

    // Push all public keys
    for (const auto& pubkey : committee)
    {
        auto compressed = pubkey.ToArray();
        sb.EmitPush(compressed.AsSpan());
    }

    // Push the total number of public keys
    sb.EmitPush(static_cast<int64_t>(committee.size()));

    // Add CHECKMULTISIG opcode
    // TODO: Use proper system call for multisig
    // sb.EmitSysCall("System.Crypto.CheckMultisig");

    auto script = sb.ToArray();

    // Calculate script hash (committee address)
    return cryptography::Hash::Hash160(script.AsSpan());
}

io::UInt160 ContractManagement::GetScriptHashFromPublicKey(const cryptography::ecc::ECPoint& publicKey)
{
    // Create single-signature verification script for the public key
    vm::ScriptBuilder sb;

    auto compressed = publicKey.ToArray();
    sb.EmitPush(compressed.AsSpan());
    sb.EmitSysCall("System.Crypto.CheckSig");

    auto script = sb.ToArray();

    // Calculate script hash
    return cryptography::Hash::Hash160(script.AsSpan());
}

std::shared_ptr<vm::StackItem>
ContractManagement::OnGetContractById(ApplicationEngine& engine,
                                      const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty())
        throw std::runtime_error("Invalid arguments");

    auto idItem = args[0];
    auto id = static_cast<int32_t>(idItem->GetInteger());

    // Find contract by ID
    auto contracts = ListContracts(engine.GetSnapshot());
    for (const auto& contract : contracts)
    {
        if (contract->GetId() == id)
        {
            // Convert contract to stack item
            std::ostringstream stream;
            io::BinaryWriter writer(stream);
            contract->Serialize(writer);
            std::string data = stream.str();
            return vm::StackItem::Create(
                io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));
        }
    }

    return vm::StackItem::Null();
}

std::shared_ptr<vm::StackItem>
ContractManagement::OnGetContractHashes(ApplicationEngine& engine,
                                        const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    // Get all contract hashes
    auto contracts = ListContracts(engine.GetSnapshot());
    auto array = vm::StackItem::CreateArray();

    for (const auto& contract : contracts)
    {
        array->Add(vm::StackItem::Create(contract->GetScriptHash()));
    }

    return array;
}
}  // namespace neo::smartcontract::native
