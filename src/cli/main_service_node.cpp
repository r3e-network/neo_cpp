/**
 * @file main_service_node.cpp
 * @brief Service implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cli/console_helper.h>
#include <neo/cli/main_service.h>
#include <neo/network/p2p/remote_node.h>

namespace neo::cli
{
void MainService::InitializeNodeCommands()
{
    // Node Commands
    RegisterCommand(
        "showstate",
        [this](const std::vector<std::string>& args)
        {
            OnShowState();
            return true;
        },
        "Node");

    RegisterCommand(
        "showpool",
        [this](const std::vector<std::string>& args)
        {
            OnShowPool();
            return true;
        },
        "Node");

    RegisterCommand(
        "showpeers",
        [this](const std::vector<std::string>& args)
        {
            OnShowPeers();
            return true;
        },
        "Node");
}

void MainService::OnShowState()
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto height = neoSystem_->GetBlockchain().GetCurrentBlockIndex();
        auto hash = neoSystem_->GetBlockchain().GetCurrentBlockHash();
        auto headerHeight = neoSystem_->GetBlockchain().GetCurrentHeaderIndex();
        auto headerHash = neoSystem_->GetBlockchain().GetCurrentHeaderHash();
        auto localNode = neoSystem_->GetLocalNode();
        auto peerCount = localNode ? localNode->GetConnectedCount() : 0U;
        auto memoryPoolSize = neoSystem_->GetMemPool().GetCount();

        ConsoleHelper::Info("State:");
        ConsoleHelper::Info("  Block Height: " + std::to_string(height));
        ConsoleHelper::Info("  Block Hash: " + hash.ToString());
        ConsoleHelper::Info("  Header Height: " + std::to_string(headerHeight));
        ConsoleHelper::Info("  Header Hash: " + headerHash.ToString());
        ConsoleHelper::Info("  Connected Peers: " + std::to_string(peerCount));
        ConsoleHelper::Info("  Memory Pool Size: " + std::to_string(memoryPoolSize));
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

void MainService::OnShowPool()
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto transactions = neoSystem_->GetMemPool().GetTransactions();

        ConsoleHelper::Info("Memory Pool Transactions: " + std::to_string(transactions.size()));
        for (const auto& tx : transactions)
        {
            ConsoleHelper::Info("  " + tx->GetHash().ToString());
        }
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

void MainService::OnShowPeers()
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto localNode = neoSystem_->GetLocalNode();
        auto peers = localNode ? localNode->GetConnectedNodes() : std::vector<network::p2p::RemoteNode*>{};

        ConsoleHelper::Info("Connected Peers: " + std::to_string(peers.size()));
        for (const auto& peer : peers)
        {
            ConsoleHelper::Info("  " + peer->GetRemoteEndPoint().ToString());
        }
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}
}  // namespace neo::cli
