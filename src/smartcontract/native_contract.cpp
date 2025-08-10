#include <neo/cryptography/hash.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/smartcontract/native_contract.h>
#include <neo/vm/script.h>

#include <sstream>

namespace neo::smartcontract
{
// NativeContract implementation
NativeContract::NativeContract(const std::string& name, int32_t id) : name_(name), id_(id)
{
    // Calculate script hash
    std::string script = "neo.native." + name;
    scriptHash_ =
        cryptography::Hash::Hash160(io::ByteSpan(reinterpret_cast<const uint8_t*>(script.data()), script.size()));

    // Create contract state
    contractState_.SetId(id);
    contractState_.SetScriptHash(scriptHash_);
    contractState_.SetScript(
        io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(script.data()), script.size())));
    contractState_.SetManifest(CreateManifest());
}

const std::string& NativeContract::GetName() const { return name_; }

int32_t NativeContract::GetId() const { return id_; }

const io::UInt160& NativeContract::GetScriptHash() const { return scriptHash_; }

const ContractState& NativeContract::GetContractState() const { return contractState_; }

void NativeContract::RegisterMethod(const std::string& name, std::function<bool(ApplicationEngine&)> handler,
                                    CallFlags flags)
{
    methods_[name] = std::make_pair(handler, flags);
}

bool NativeContract::Invoke(ApplicationEngine& engine, const std::string& method)
{
    auto it = methods_.find(method);
    if (it == methods_.end()) return false;

    if (!engine.HasFlag(it->second.second)) throw std::runtime_error("Cannot invoke method without required flags");

    return it->second.first(engine);
}

persistence::StorageKey NativeContract::CreateStorageKey(uint8_t prefix, const io::ByteVector& key) const
{
    if (key.IsEmpty()) return persistence::StorageKey(scriptHash_, io::ByteVector{prefix});

    return persistence::StorageKey(scriptHash_, io::ByteVector{prefix}.Concat(key));
}

// NativeContractManager implementation
NativeContractManager& NativeContractManager::GetInstance()
{
    static NativeContractManager instance;
    return instance;
}

NativeContractManager::NativeContractManager() = default;

void NativeContractManager::RegisterContract(std::shared_ptr<NativeContract> contract)
{
    contracts_.push_back(contract);
    contractsByHash_[contract->GetScriptHash()] = contract;
    contractsByName_[contract->GetName()] = contract;
}

std::shared_ptr<NativeContract> NativeContractManager::GetContract(const io::UInt160& scriptHash) const
{
    auto it = contractsByHash_.find(scriptHash);
    if (it == contractsByHash_.end()) return nullptr;

    return it->second;
}

std::shared_ptr<NativeContract> NativeContractManager::GetContract(const std::string& name) const
{
    auto it = contractsByName_.find(name);
    if (it == contractsByName_.end()) return nullptr;

    return it->second;
}

const std::vector<std::shared_ptr<NativeContract>>& NativeContractManager::GetContracts() const { return contracts_; }

void NativeContractManager::Initialize(std::shared_ptr<persistence::DataCache> snapshot)
{
    for (const auto& contract : contracts_)
    {
        contract->Initialize(snapshot);
    }
}
}  // namespace neo::smartcontract
