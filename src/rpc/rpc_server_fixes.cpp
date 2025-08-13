/**
 * @file rpc_server_fixes.cpp
 * @brief JSON-RPC server implementation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/network/p2p_server.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/rpc/rpc_server.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/neo_token.h>

namespace neo::rpc
{
// Fixed implementation for GetUnclaimedGas
json::JObject RpcServer::GetUnclaimedGasFixed(const json::JArray& params)
{
    if (params.size() < 1)
    {
        throw std::invalid_argument("Missing address parameter");
    }

    auto address = params[0].AsString();
    io::UInt160 scriptHash;
    if (!io::UInt160::TryParseAddress(address, scriptHash))
    {
        throw std::invalid_argument("Invalid address format");
    }

    // Get the latest snapshot
    auto snapshot = blockchain_->GetSnapshot();
    if (!snapshot)
    {
        throw std::runtime_error("Failed to get blockchain snapshot");
    }

    // Calculate unclaimed GAS using GasToken native contract
    try
    {
        smartcontract::ApplicationEngine engine(smartcontract::TriggerType::Application, nullptr, snapshot);

        // Call GasToken.UnclaimedGas method
        std::vector<std::shared_ptr<vm::StackItem>> args;
        args.push_back(vm::StackItem::Create(scriptHash.ToArray()));
        args.push_back(vm::StackItem::Create(blockchain_->GetHeight()));

        auto gasContract = engine.GetNativeContract(smartcontract::native::GasToken::GetContractId());
        if (!gasContract)
        {
            throw std::runtime_error("GAS native contract not found");
        }

        // Get unclaimed amount
        auto result = gasContract->InvokeMethod(engine, "unclaimedGas", args);
        if (!result)
        {
            throw std::runtime_error("Failed to invoke unclaimedGas method");
        }

        auto unclaimedAmount = result->GetInteger();

        json::JObject response;
        response["unclaimed"] = json::JString(std::to_string(unclaimedAmount));
        response["address"] = json::JString(address);
        return response;
    }
    catch (const std::exception& e)
    {
        // If calculation fails, return 0
        json::JObject response;
        response["unclaimed"] = json::JString("0");
        response["address"] = json::JString(address);
        response["error"] = json::JString(e.what());
        return response;
    }
}

// Fixed implementation for GetPeerCount
int RpcServer::GetPeerCountFixed() const
{
    // Get actual peer count from P2P server if available
    if (p2p_server_)
    {
        return p2p_server_->GetConnectedPeerCount();
    }
    return 0;
}

// Fixed implementation for GetPeers
json::JObject RpcServer::GetPeersFixed(const json::JArray& /* params */)
{
    json::JObject result;
    json::JArray connected;
    json::JArray bad;
    json::JArray unconnected;

    if (p2p_server_)
    {
        // Get connected peers
        auto connectedPeers = p2p_server_->GetConnectedPeers();
        for (const auto& peer : connectedPeers)
        {
            json::JObject peerObj;
            peerObj.SetProperty("address", std::make_shared<json::JString>(peer.address));
            peerObj.SetProperty("port", std::make_shared<json::JNumber>(peer.port));
            peerObj.SetProperty("version", std::make_shared<json::JString>(peer.version));
            peerObj.SetProperty("lastSeen", std::make_shared<json::JNumber>(peer.lastSeen));
            connected.Add(std::make_shared<json::JObject>(peerObj));
        }

        // Get bad peers
        auto badPeers = p2p_server_->GetBadPeers();
        for (const auto& peer : badPeers)
        {
            json::JObject peerObj;
            peerObj.SetProperty("address", std::make_shared<json::JString>(peer.address));
            peerObj.SetProperty("port", std::make_shared<json::JNumber>(peer.port));
            bad.Add(std::make_shared<json::JObject>(peerObj));
        }

        // Get unconnected peers
        auto unconnectedPeers = p2p_server_->GetUnconnectedPeers();
        for (const auto& peer : unconnectedPeers)
        {
            json::JObject peerObj;
            peerObj.SetProperty("address", std::make_shared<json::JString>(peer.address));
            peerObj.SetProperty("port", std::make_shared<json::JNumber>(peer.port));
            unconnected.Add(std::make_shared<json::JObject>(peerObj));
        }
    }

    result.SetProperty("connected", std::make_shared<json::JArray>(connected));
    result.SetProperty("bad", std::make_shared<json::JArray>(bad));
    result.SetProperty("unconnected", std::make_shared<json::JArray>(unconnected));

    return result;
}

// Fixed implementation for GetNep17Balances
json::JObject RpcServer::GetNep17BalancesFixed(const json::JArray& params)
{
    if (params.size() < 1)
    {
        throw std::invalid_argument("Missing address parameter");
    }

    auto address = params[0].AsString();
    io::UInt160 scriptHash;
    if (!io::UInt160::TryParseAddress(address, scriptHash))
    {
        throw std::invalid_argument("Invalid address format");
    }

    json::JObject result;
    result.SetProperty("address", std::make_shared<json::JString>(address));

    json::JArray balances;

    // Get the latest snapshot
    auto snapshot = blockchain_->GetSnapshot();
    if (!snapshot)
    {
        result.SetProperty("balance", std::make_shared<json::JArray>(balances));
        return result;
    }

    try
    {
        // Get NEO balance
        {
            smartcontract::ApplicationEngine engine(smartcontract::TriggerType::Application, nullptr, snapshot);
            auto neoContract = engine.GetNativeContract(smartcontract::native::NeoToken::GetContractId());
            if (neoContract)
            {
                std::vector<std::shared_ptr<vm::StackItem>> args;
                args.push_back(vm::StackItem::Create(scriptHash.ToArray()));

                auto balanceResult = neoContract->InvokeMethod(engine, "balanceOf", args);
                if (balanceResult)
                {
                    auto balance = balanceResult->GetInteger();
                    if (balance > 0)
                    {
                        json::JObject neoBalance;
                        neoBalance.SetProperty(
                            "assethash", std::make_shared<json::JString>(neoContract->GetScriptHash().ToString()));
                        neoBalance.SetProperty("amount", std::make_shared<json::JString>(std::to_string(balance)));
                        neoBalance.SetProperty("lastupdatedblock",
                                               std::make_shared<json::JNumber>(blockchain_->GetHeight()));
                        balances.Add(std::make_shared<json::JObject>(neoBalance));
                    }
                }
            }
        }

        // Get GAS balance
        {
            smartcontract::ApplicationEngine engine(smartcontract::TriggerType::Application, nullptr, snapshot);
            auto gasContract = engine.GetNativeContract(smartcontract::native::GasToken::GetContractId());
            if (gasContract)
            {
                std::vector<std::shared_ptr<vm::StackItem>> args;
                args.push_back(vm::StackItem::Create(scriptHash.ToArray()));

                auto balanceResult = gasContract->InvokeMethod(engine, "balanceOf", args);
                if (balanceResult)
                {
                    auto balance = balanceResult->GetInteger();
                    if (balance > 0)
                    {
                        json::JObject gasBalance;
                        gasBalance.SetProperty(
                            "assethash", std::make_shared<json::JString>(gasContract->GetScriptHash().ToString()));
                        gasBalance.SetProperty("amount", std::make_shared<json::JString>(std::to_string(balance)));
                        gasBalance.SetProperty("lastupdatedblock",
                                               std::make_shared<json::JNumber>(blockchain_->GetHeight()));
                        balances.Add(std::make_shared<json::JObject>(gasBalance));
                    }
                }
            }
        }

        // Add other NEP-17 token balances by iterating through contracts
        {
            // Get all contracts from the ContractManagement native contract
            auto contractManagement = smartcontract::native::ContractManagement::GetInstance();
            if (contractManagement)
            {
                auto contracts = contractManagement->ListContracts(snapshot);

                for (const auto& contract : contracts)
                {
                    // Skip native contracts (already handled above)
                    if (contract->GetId() < 0)
                    {
                        continue;
                    }

                    // Check if this contract implements NEP-17 by looking for required methods
                    bool hasBalanceOf =
                        contractManagement->HasMethod(snapshot, contract->GetScriptHash(), "balanceOf", 1);
                    bool hasSymbol = contractManagement->HasMethod(snapshot, contract->GetScriptHash(), "symbol", 0);
                    bool hasDecimals =
                        contractManagement->HasMethod(snapshot, contract->GetScriptHash(), "decimals", 0);
                    bool hasTotalSupply =
                        contractManagement->HasMethod(snapshot, contract->GetScriptHash(), "totalSupply", 0);
                    bool hasTransfer =
                        contractManagement->HasMethod(snapshot, contract->GetScriptHash(), "transfer", 4);

                    // If it has all NEP-17 required methods, query balance
                    if (hasBalanceOf && hasSymbol && hasDecimals && hasTotalSupply && hasTransfer)
                    {
                        try
                        {
                            smartcontract::ApplicationEngine engine(smartcontract::TriggerType::Application, nullptr,
                                                                    snapshot);

                            // Call balanceOf method
                            std::vector<std::shared_ptr<vm::StackItem>> args;
                            args.push_back(vm::StackItem::Create(scriptHash.ToArray()));

                            engine.LoadContract(contract->GetScript());
                            engine.CallContract(contract->GetScriptHash(), "balanceOf", args,
                                                smartcontract::CallFlags::ReadStates);

                            if (engine.Execute() == vm::VMState::HALT && engine.GetResultStack().GetCount() > 0)
                            {
                                auto balanceItem = engine.GetResultStack().Pop();
                                if (balanceItem && balanceItem->IsInteger())
                                {
                                    auto balance = balanceItem->GetInteger();
                                    if (balance > 0)
                                    {
                                        json::JObject tokenBalance;
                                        tokenBalance.SetProperty(
                                            "assethash",
                                            std::make_shared<json::JString>(contract->GetScriptHash().ToString()));
                                        tokenBalance.SetProperty(
                                            "amount", std::make_shared<json::JString>(std::to_string(balance)));
                                        tokenBalance.SetProperty("lastupdatedblock", std::make_shared<json::JNumber>(
                                                                                         blockchain_->GetHeight()));
                                        balances.Add(std::make_shared<json::JObject>(tokenBalance));
                                    }
                                }
                            }
                        }
                        catch (const std::exception&)
                        {
                            // Skip this contract if balance query fails
                            continue;
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        // Log error but don't fail the entire request
    }

    result.SetProperty("balance", std::make_shared<json::JArray>(balances));
    return result;
}

// Fixed implementation for ExecuteScript
json::JObject RpcServer::ExecuteScriptFixed(const io::ByteVector& script) const
{
    json::JObject result;

    try
    {
        // Get the latest snapshot
        auto snapshot = blockchain_->GetSnapshot();
        if (!snapshot)
        {
            throw std::runtime_error("Failed to get blockchain snapshot");
        }

        // Create application engine
        smartcontract::ApplicationEngine engine(smartcontract::TriggerType::Application, nullptr, snapshot);

        // Load and execute script
        engine.LoadScript(script);
        auto vmState = engine.Execute();

        // Get gas consumed
        auto gasConsumed = engine.GetGasConsumed();

        // Build result
        result["script"] = json::JString(io::ToHexString(script));
        result["state"] = json::JString(vm::VMStateToString(vmState));
        result["gasconsumed"] = json::JString(std::to_string(gasConsumed));

        // Add exception if any
        if (engine.GetFaultException())
        {
            result["exception"] = json::JString(engine.GetFaultException()->what());
        }
        else
        {
            result["exception"] = json::JNull();
        }

        // Add result stack
        json::JArray stack;
        auto& resultStack = engine.GetResultStack();
        while (!resultStack.empty())
        {
            auto item = resultStack.top();
            resultStack.pop();

            json::JObject stackItem;
            stackItem["type"] = json::JString(vm::StackItemTypeToString(item->GetType()));
            stackItem["value"] = json::JString(item->ToString());
            stack.Add(std::make_shared<json::JObject>(stackItem));
        }
        result["stack"] = stack;
    }
    catch (const std::exception& e)
    {
        result["script"] = json::JString(io::ToHexString(script));
        result["state"] = json::JString("FAULT");
        result["gasconsumed"] = json::JString("0");
        result["exception"] = json::JString(e.what());
        result["stack"] = json::JArray();
    }

    return result;
}

// Fixed implementation for GetContractId
uint32_t RpcServer::GetContractIdFixed(const io::UInt160& hash) const
{
    // Get the latest snapshot
    auto snapshot = blockchain_->GetSnapshot();
    if (!snapshot)
    {
        throw std::runtime_error("Failed to get blockchain snapshot");
    }

    // Look up contract from ContractManagement
    auto contract = smartcontract::native::ContractManagement::GetContract(*snapshot, hash);
    if (!contract)
    {
        throw std::runtime_error("Contract not found");
    }

    return contract->GetId();
}
}  // namespace neo::rpc