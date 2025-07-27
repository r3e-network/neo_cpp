#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/oracle_response.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/oracle_contract.h>
#include <neo/smartcontract/native/role_management.h>
#include <sstream>
#include <string>
#include <vector>

namespace neo::smartcontract::native
{
std::shared_ptr<vm::StackItem> OracleContract::OnGetPrice(ApplicationEngine& engine,
                                                          const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    return vm::StackItem::Create(GetPrice(engine.GetSnapshot()));
}

std::shared_ptr<vm::StackItem> OracleContract::OnSetPrice(ApplicationEngine& engine,
                                                          const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty())
        throw std::runtime_error("Invalid arguments");

    // Check if caller is committee
    if (!CheckCommittee(engine))
        throw std::runtime_error("Not authorized");

    auto priceItem = args[0];
    auto price = priceItem->GetInteger();

    if (price <= 0)
        throw std::runtime_error("Invalid price");

    // Set price
    SetPrice(engine.GetSnapshot(), price);

    return vm::StackItem::Create(true);
}

std::shared_ptr<vm::StackItem> OracleContract::OnGetOracles(ApplicationEngine& engine,
                                                            const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    auto oracles = GetOracles(engine.GetSnapshot());
    std::vector<std::shared_ptr<vm::StackItem>> result;
    result.reserve(oracles.size());
    for (const auto& oracle : oracles)
    {
        result.push_back(vm::StackItem::Create(io::ByteVector(oracle.AsSpan())));
    }
    return vm::StackItem::Create(result);
}

std::shared_ptr<vm::StackItem> OracleContract::OnSetOracles(ApplicationEngine& engine,
                                                            const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty())
        throw std::runtime_error("Invalid arguments");

    // Check if caller is committee
    if (!CheckCommittee(engine))
        throw std::runtime_error("Not authorized");

    auto oraclesItem = args[0];
    if (oraclesItem->GetType() != vm::StackItemType::Array)
        throw std::runtime_error("Invalid oracles");

    auto oraclesArray = oraclesItem->GetArray();
    std::vector<io::UInt160> oracles;
    oracles.reserve(oraclesArray.size());
    for (const auto& item : oraclesArray)
    {
        auto bytes = item->GetByteArray();
        if (bytes.Size() != 20)
            throw std::runtime_error("Invalid oracle");

        io::UInt160 oracle;
        std::memcpy(oracle.Data(), bytes.Data(), 20);
        oracles.push_back(oracle);
    }

    // Set oracles
    SetOracles(engine.GetSnapshot(), oracles);

    return vm::StackItem::Create(true);
}

std::shared_ptr<vm::StackItem> OracleContract::OnRequest(ApplicationEngine& engine,
                                                         const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 4)
        throw std::runtime_error("Invalid arguments");

    auto urlItem = args[0];
    auto filterItem = args[1];
    auto callbackItem = args[2];
    auto callbackMethodItem = args[3];
    auto gasForResponseItem = args.size() > 4 ? args[4] : vm::StackItem::Create(static_cast<int64_t>(0));
    auto userDataItem = args.size() > 5 ? args[5] : vm::StackItem::Create(io::ByteVector{});

    auto url = urlItem->GetString();
    auto filterStr = filterItem->GetString();
    auto callback = callbackItem->GetByteArray();
    auto callbackMethod = callbackMethodItem->GetString();
    auto gasForResponse = gasForResponseItem->GetInteger();
    auto userData = userDataItem->GetByteArray();

    if (url.empty() || url.length() > MAX_URL_LENGTH)
        throw std::runtime_error("Invalid URL");

    if (filterStr.length() > MAX_FILTER_LENGTH)
        throw std::runtime_error("Filter too long");

    if (callback.Size() != 20)
        throw std::runtime_error("Invalid callback contract");

    if (callbackMethod.empty() || callbackMethod.length() > MAX_CALLBACK_LENGTH)
        throw std::runtime_error("Invalid callback method");

    if (gasForResponse < 0)
        throw std::runtime_error("Invalid gas for response");

    if (userData.Size() > MAX_USER_DATA_LENGTH)
        throw std::runtime_error("User data too large");

    // Check if the caller has enough GAS
    auto price = GetPrice(engine.GetSnapshot());
    auto gasToken = GasToken::GetInstance();
    auto caller = engine.GetCurrentScriptHash();
    auto gasBalance = gasToken->GetBalance(engine.GetSnapshot(), caller);
    if (gasBalance < price + gasForResponse)
        throw std::runtime_error("Insufficient GAS");

    // Transfer GAS to the oracle contract
    if (!gasToken->Transfer(engine.GetSnapshot(), caller, GetScriptHash(), price))
        throw std::runtime_error("Failed to transfer GAS");

    // Reserve gas for response
    if (gasForResponse > 0)
    {
        if (!gasToken->Transfer(engine.GetSnapshot(), caller, GetScriptHash(), gasForResponse))
            throw std::runtime_error("Failed to reserve gas for response");
    }

    // Create request
    io::UInt160 callbackContract;
    std::memcpy(callbackContract.Data(), callback.Data(), 20);

    // Get original transaction ID
    auto originalTxid = GetOriginalTxid(engine);

    // Create request
    uint64_t id = CreateRequest(engine.GetSnapshot(), url, filterStr, callbackContract, callbackMethod, gasForResponse,
                                userData, originalTxid);

    // Send notification
    std::vector<std::shared_ptr<vm::StackItem>> notificationArgs;
    notificationArgs.push_back(vm::StackItem::Create(static_cast<int64_t>(id)));
    notificationArgs.push_back(vm::StackItem::Create(io::ByteVector(callbackContract.AsSpan())));
    notificationArgs.push_back(vm::StackItem::Create(url));
    notificationArgs.push_back(vm::StackItem::Create(filterStr));
    engine.Notify(GetScriptHash(), "OracleRequest", notificationArgs);

    return vm::StackItem::Create(static_cast<int64_t>(id));
}

std::shared_ptr<vm::StackItem> OracleContract::OnFinish(ApplicationEngine& engine,
                                                        const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    // TODO: Implement oracle response finishing
    // This requires GetOracleResponse method in transactions
    throw std::runtime_error("Oracle response handling not implemented");
}

std::shared_ptr<vm::StackItem> OracleContract::OnVerify(ApplicationEngine& engine,
                                                        const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    // TODO: Implement oracle response verification
    // This requires GetOracleResponse method in transactions
    return vm::StackItem::Create(false);
}

bool OracleContract::CheckCommittee(ApplicationEngine& engine) const
{
    // Use the RoleManagement contract's CheckCommittee method
    auto roleManagement = RoleManagement::GetInstance();
    return roleManagement->CheckCommittee(engine);
}

bool OracleContract::CheckOracleNode(ApplicationEngine& engine) const
{
    // Get the current script hash
    auto currentScriptHash = engine.GetCurrentScriptHash();

    // Get the oracle nodes
    auto roleManagement = RoleManagement::GetInstance();
    auto oracleNodes = roleManagement->GetDesignatedByRole(engine.GetSnapshot(), Role::Oracle,
                                                           engine.GetPersistingBlock()->GetIndex());

    // Check if the current script hash is an oracle node
    for (const auto& node : oracleNodes)
    {
        auto scriptHash = neo::cryptography::Hash::Hash160(node.ToArray().AsSpan());
        if (scriptHash == currentScriptHash)
            return true;
    }

    return false;
}

io::UInt256 OracleContract::GetOriginalTxid(ApplicationEngine& engine) const
{
    // Get the transaction
    auto scriptContainer = engine.GetScriptContainer();
    auto tx = dynamic_cast<const ledger::Transaction*>(scriptContainer);
    if (!tx)
        return io::UInt256();

    // TODO: Implement oracle response attribute handling
    // For now, return the transaction hash
    return tx->GetHash();
}
}  // namespace neo::smartcontract::native
