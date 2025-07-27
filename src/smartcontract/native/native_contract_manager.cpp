#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/crypto_lib.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/smartcontract/native/name_service.h>
#include <neo/smartcontract/native/native_contract_manager.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/notary.h>
#include <neo/smartcontract/native/oracle_contract.h>
#include <neo/smartcontract/native/policy_contract.h>
#include <neo/smartcontract/native/role_management.h>
#include <neo/smartcontract/native/std_lib.h>

namespace neo::smartcontract::native
{
NativeContractManager& NativeContractManager::GetInstance()
{
    static NativeContractManager instance;
    return instance;
}

NativeContractManager::NativeContractManager()
{
    // Register native contracts
    RegisterContract(std::make_shared<ContractManagement>());
    RegisterContract(std::make_shared<StdLib>());
    RegisterContract(std::make_shared<CryptoLib>());
    RegisterContract(std::make_shared<LedgerContract>());
    RegisterContract(std::make_shared<NeoToken>());
    RegisterContract(std::make_shared<GasToken>());
    RegisterContract(std::make_shared<PolicyContract>());
    RegisterContract(std::make_shared<OracleContract>());
    RegisterContract(std::make_shared<RoleManagement>());
    RegisterContract(NameService::GetInstance());
    RegisterContract(Notary::GetInstance());
}

const std::vector<std::shared_ptr<NativeContract>>& NativeContractManager::GetContracts() const
{
    return contracts_;
}

std::shared_ptr<NativeContract> NativeContractManager::GetContract(const std::string& name) const
{
    auto it = contractsByName_.find(name);
    if (it == contractsByName_.end())
        return nullptr;

    return it->second;
}

std::shared_ptr<NativeContract> NativeContractManager::GetContract(const io::UInt160& scriptHash) const
{
    auto it = contractsByScriptHash_.find(scriptHash);
    if (it == contractsByScriptHash_.end())
        return nullptr;

    return it->second;
}

std::shared_ptr<NativeContract> NativeContractManager::GetContract(uint32_t id) const
{
    auto it = contractsById_.find(id);
    if (it == contractsById_.end())
        return nullptr;

    return it->second;
}

void NativeContractManager::RegisterContract(std::shared_ptr<NativeContract> contract)
{
    contracts_.push_back(contract);
    contractsByName_[contract->GetName()] = contract;
    contractsByScriptHash_[contract->GetScriptHash()] = contract;
    contractsById_[contract->GetId()] = contract;
}

void NativeContractManager::Initialize()
{
    for (auto& contract : contracts_)
    {
        contract->Initialize();
    }
}
}  // namespace neo::smartcontract::native
