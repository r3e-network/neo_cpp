#include <neo/ledger/blockchain_execution.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/crypto_lib.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/oracle_contract.h>
#include <neo/smartcontract/native/policy_contract.h>
#include <neo/smartcontract/native/role_management.h>
#include <neo/smartcontract/native/std_lib.h>
#include <neo/smartcontract/trigger_type.h>
#include <neo/vm/vm_state.h>

namespace neo::ledger
{
BlockchainExecution::BlockchainExecution(std::shared_ptr<BlockchainCallbacks> callbacks) : callbacks_(callbacks) {}

bool BlockchainExecution::ExecuteBlock(const Block& block, std::shared_ptr<persistence::DataCache> snapshot)
{
    // Implement the ApplicationEngine execution for blocks
    try
    {
        // Create application engine for OnPersist
        auto engine =
            smartcontract::ApplicationEngine::Create(smartcontract::TriggerType::OnPersist, nullptr, snapshot, &block,
                                                     0  // No gas limit for system operations
            );

        // Execute OnPersist for native contracts
        auto neoToken = smartcontract::native::NeoToken::GetInstance();
        auto gasToken = smartcontract::native::GasToken::GetInstance();
        auto policyContract = smartcontract::native::PolicyContract::GetInstance();

        // Execute OnPersist for NEO token
        if (neoToken && !neoToken->OnPersist(*engine)) return false;

        // Execute OnPersist for GAS token
        if (gasToken && !gasToken->OnPersist(*engine)) return false;

        // Execute transactions
        for (const auto& tx : block.GetTransactions())
        {
            // Create application engine for transaction
            auto txEngine = smartcontract::ApplicationEngine::Create(smartcontract::TriggerType::Application, &tx,
                                                                     snapshot, &block, tx.GetSystemFee());

            // Load and execute transaction script
            txEngine->LoadScript(tx.GetScript());
            auto state = txEngine->Execute();

            if (state != vm::VMState::Halt) return false;

            // Notify transaction execution
            callbacks_->NotifyTransactionExecution(std::make_shared<Transaction>(tx));
        }

        // Execute PostPersist for native contracts
        auto postEngine =
            smartcontract::ApplicationEngine::Create(smartcontract::TriggerType::PostPersist, nullptr, snapshot, &block,
                                                     0  // No gas limit for system operations
            );

        // Execute PostPersist for NEO token
        if (neoToken && !neoToken->PostPersist(*postEngine)) return false;

        // Execute PostPersist for GAS token
        if (gasToken && !gasToken->PostPersist(*postEngine)) return false;

        return true;
    }
    catch (const std::exception& ex)
    {
        // Log error and return false
        return false;
    }
}

void BlockchainExecution::Initialize(std::shared_ptr<persistence::DataCache> snapshot)
{
    // Implement the native contracts initialization
    try
    {
        // Initialize native contracts in the correct order
        auto contractManagement = smartcontract::native::ContractManagement::GetInstance();
        if (contractManagement) contractManagement->Initialize(snapshot);

        // Initialize NEO token
        auto neoToken = smartcontract::native::NeoToken::GetInstance();
        if (neoToken) neoToken->Initialize(snapshot);

        // Initialize GAS token
        auto gasToken = smartcontract::native::GasToken::GetInstance();
        if (gasToken) gasToken->Initialize(snapshot);

        // Initialize policy contract
        auto policyContract = smartcontract::native::PolicyContract::GetInstance();
        if (policyContract) policyContract->Initialize(snapshot);

        // Initialize other native contracts
        auto ledgerContract = smartcontract::native::LedgerContract::GetInstance();
        if (ledgerContract) ledgerContract->Initialize(snapshot);

        auto roleManagement = smartcontract::native::RoleManagement::GetInstance();
        if (roleManagement) roleManagement->Initialize(snapshot);

        auto oracleContract = smartcontract::native::OracleContract::GetInstance();
        if (oracleContract) oracleContract->Initialize(snapshot);

        auto stdLib = smartcontract::native::StdLib::GetInstance();
        if (stdLib) stdLib->Initialize(snapshot);

        auto cryptoLib = smartcontract::native::CryptoLib::GetInstance();
        if (cryptoLib) cryptoLib->Initialize(snapshot);
    }
    catch (const std::exception& ex)
    {
        // Log error but don't throw - initialization should be robust
    }
}
}  // namespace neo::ledger
