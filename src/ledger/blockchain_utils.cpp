#include <algorithm>
#include <iostream>
#include <neo/crypto/ecc/ecc_point.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/neo_system.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/contract.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/role_management.h>
#include <sstream>

namespace neo::ledger
{
std::unordered_set<io::UInt160>
Blockchain::UpdateExtensibleWitnessWhiteList(std::shared_ptr<persistence::DataCache> snapshot)
{
    std::unordered_set<io::UInt160> whitelist;

    try
    {
        uint32_t current_height = system_->GetLedgerContract()->GetCurrentIndex(snapshot);

        // Add NEO committee address
        auto neo_contract = system_->GetNeoToken();
        auto committee_address = neo_contract->GetCommitteeAddress(snapshot);
        whitelist.insert(committee_address);

        // Add next block validators BFT address
        auto validators = neo_contract->GetNextBlockValidators(snapshot, system_->GetSettings()->GetValidatorsCount());
        auto validators_bft_address = smartcontract::Contract::GetBFTAddress(validators);
        whitelist.insert(validators_bft_address);

        // Add individual validator addresses
        for (const auto& validator : validators)
        {
            auto script = smartcontract::Contract::CreateSignatureRedeemScript(validator);
            auto script_hash = script.ToScriptHash();
            whitelist.insert(script_hash);
        }

        // Add state validators if any
        auto role_management = system_->GetRoleManagement();
        auto state_validators =
            role_management->GetDesignatedByRole(snapshot, smartcontract::Role::StateValidator, current_height);

        if (!state_validators.empty())
        {
            auto state_validators_bft_address = smartcontract::Contract::GetBFTAddress(state_validators);
            whitelist.insert(state_validators_bft_address);

            for (const auto& state_validator : state_validators)
            {
                auto script = smartcontract::Contract::CreateSignatureRedeemScript(state_validator);
                auto script_hash = script.ToScriptHash();
                whitelist.insert(script_hash);
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error updating extensible witness whitelist: " << e.what() << std::endl;
    }

    return whitelist;
}

bool Blockchain::IsGenesisBlockInitialized() const
{
    try
    {
        auto ledger_contract = system_->GetLedgerContract();
        return ledger_contract->IsInitialized(data_cache_);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error checking genesis block initialization: " << e.what() << std::endl;
        return false;
    }
}

void Blockchain::InitializeGenesisBlock()
{
    try
    {
        auto genesis_block = system_->GetGenesisBlock();
        if (!genesis_block)
        {
            throw std::runtime_error("Genesis block not found in NeoSystem");
        }

        std::cout << "Initializing blockchain with genesis block..." << std::endl;

        // Persist the genesis block
        PersistBlock(genesis_block);

        std::cout << "Genesis block initialized successfully" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error initializing genesis block: " << e.what() << std::endl;
        throw;
    }
}

}  // namespace neo::ledger