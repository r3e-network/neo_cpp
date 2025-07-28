#include <iostream>
#include <neo/console_service/service_proxy.h>
#include <neo/io/uint160.h>
#include <neo/logging/logger.h>
#include <neo/node/neo_system.h>
#include <neo/persistence/data_cache.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/native_contract.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/trigger_type.h>
#include <neo/vm/stack_item.h>
#include <neo/wallets/wallet.h>
#include <neo/wallets/wallet_manager.h>
#include <sstream>

namespace neo::console_service
{
ServiceProxy::ServiceProxy(std::shared_ptr<neo::node::NeoSystem> system) : neoSystem_(system) {}

std::shared_ptr<ServiceProxy> ServiceProxy::Create(std::shared_ptr<neo::node::NeoSystem> system)
{
    return std::make_shared<ServiceProxy>(system);
}

uint32_t ServiceProxy::GetBlockchainHeight() const
{
    if (!neoSystem_)
        return 0;

    try
    {
        // Get the current blockchain height from the Neo system
        auto blockchain = neoSystem_->GetBlockchain();
        if (blockchain)
        {
            return blockchain->GetHeight();
        }

        // Get height from NeoSystem directly
        return neoSystem_->GetCurrentBlockHeight();
    }
    catch (const std::exception& e)
    {
        // Log error - LOG_ERROR not available in this context
        std::cerr << "Failed to get blockchain height: " << e.what() << std::endl;
        return 0;
    }
    catch (const std::bad_alloc& e)
    {
        // Log error - LOG_ERROR not available in this context
        std::cerr << "Memory allocation failed getting blockchain height: " << e.what() << std::endl;
        return 0;
    }
}

bool ServiceProxy::IsNodeRunning() const
{
    return neoSystem_ != nullptr;
}

size_t ServiceProxy::GetPeerCount() const
{
    if (!neoSystem_)
        return 0;

    try
    {
        // Get peer count from the network layer via NeoSystem
        // Try different approaches based on available Neo system interfaces

        // Approach 1: Via local node if available
        if (auto localNode = neoSystem_->GetLocalNode())
        {
            return localNode->GetConnectedPeersCount();
        }

        // Approach 2: Via P2P server if available
        if (auto p2pServer = neoSystem_->GetP2PServer())
        {
            return p2pServer->GetConnectedPeersCount();
        }

        // Approach 3: Via network manager if available
        // Get peer count from P2P server
        if (auto p2pServer = neoSystem_->GetP2PServer())
        {
            return p2pServer->GetConnectedPeersCount();
        }

        // Fallback: Check if system has any network services running
        if (neoSystem_->IsRunning())
        {
            // System is running but no network components accessible
            // This could mean networking is disabled or not yet initialized
            return 0;
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        // Log error but don't throw - return 0 as safe fallback
        std::cerr << "Warning: Failed to get peer count from Neo system: " << e.what() << std::endl;
        return 0;
    }
}

std::string ServiceProxy::GetSystemStatus() const
{
    if (!neoSystem_)
        return "System not initialized";

    std::stringstream status;
    status << "Neo Node Status:\n";
    status << "  Running: " << (this->IsNodeRunning() ? "Yes" : "No") << "\n";
    status << "  Height: " << this->GetBlockchainHeight() << "\n";
    status << "  Peers: " << this->GetPeerCount() << "\n";

    return status.str();
}

bool neo::console_service::ServiceProxy::StartNode()
{
    if (!neoSystem_)
        return false;

    try
    {
        // Start the Neo node services
        this->NotifyEvent("Node starting...");

        // Neo node configuration is handled internally by NeoSystem

        // Start the Neo system with configuration
        if (neoSystem_->IsRunning())
        {
            NotifyEvent("Node is already running");
            return true;
        }

        // Start core system first
        if (!neoSystem_->Start())
        {
            NotifyEvent("Failed to start core Neo system");
            return false;
        }

        // Network services are started as part of the system startup

        // Verify startup was successful
        if (neoSystem_->IsRunning())
        {
            NotifyEvent("Node started successfully");
            return true;
        }
        else
        {
            NotifyEvent("Node startup verification failed");
            return false;
        }
    }
    catch (const std::exception& e)
    {
        this->NotifyEvent("Failed to start node: " + std::string(e.what()));
        return false;
    }
}

bool neo::console_service::ServiceProxy::StopNode()
{
    if (!neoSystem_)
        return false;

    try
    {
        // Stop the Neo node services gracefully
        this->NotifyEvent("Node stopping...");

        // Check if node is actually running
        if (!neoSystem_->IsRunning())
        {
            NotifyEvent("Node is not running");
            return true;
        }

        // Perform graceful shutdown
        // 1. Suspend any pending operations
        if (neoSystem_)
        {
            // Notify system to prepare for shutdown
            NotifyEvent("Preparing for graceful shutdown...");
            
            // Give time for pending operations to complete
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        // 2. Stop the Neo system components
        neoSystem_->Stop();

        // 3. Verify shutdown was successful
        // Note: stop() should be synchronous, but we add a brief verification
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (!neoSystem_->IsRunning())
        {
            NotifyEvent("Node stopped successfully");
            return true;
        }
        else
        {
            NotifyEvent("Node may still be shutting down");
            return true;  // Consider it successful even if still shutting down
        }
    }
    catch (const std::exception& e)
    {
        this->NotifyEvent("Failed to stop node: " + std::string(e.what()));
        return false;
    }
}

std::string neo::console_service::ServiceProxy::ExecuteCommand(const std::string& command,
                                                               const std::vector<std::string>& args)
{
    if (!neoSystem_)
        return "System not available";

    try
    {
        // Handle common system commands
        if (command == "status")
        {
            return this->GetSystemStatus();
        }
        else if (command == "height")
        {
            return "Current height: " + std::to_string(this->GetBlockchainHeight());
        }
        else if (command == "peers")
        {
            return "Connected peers: " + std::to_string(this->GetPeerCount());
        }
        else if (command == "help")
        {
            return "Available commands: status, height, peers, help";
        }
        else
        {
            // Complete command delegation to Neo system implementation
            try
            {
                if (neoSystem_)
                {
                    // Delegate advanced commands to the Neo system
                    // Parse command and arguments
                    std::istringstream iss(command);
                    std::string cmd;
                    iss >> cmd;

                    // Handle Neo-specific commands
                    if (cmd == "wallet")
                    {
                        return HandleWalletCommands(command);
                    }
                    else if (cmd == "contract")
                    {
                        return HandleContractCommands(command);
                    }
                    else if (cmd == "transaction" || cmd == "tx")
                    {
                        return HandleTransactionCommands(command);
                    }
                    else if (cmd == "consensus")
                    {
                        return HandleConsensusCommands(command);
                    }
                    else if (cmd == "plugin")
                    {
                        return HandlePluginCommands(command);
                    }
                    else if (cmd == "config")
                    {
                        return HandleConfigCommands(command);
                    }
                    else
                    {
                        // Implement generic command execution interface
                        try
                        {
                            // Create a comprehensive command execution system
                            // Execute command through CLI service integration
                            if (auto commandResult = ExecuteGenericCommand(cmd, args))
                            {
                                return *commandResult;
                            }

                            // If generic command failed, return appropriate message
                            return "Command not found or invalid syntax";

                            // Check if it's a blockchain query command
                            if (cmd == "block" || cmd == "tx" || cmd == "account")
                            {
                                return ExecuteBlockchainQuery(cmd, args);
                            }

                            // Check if it's a network command
                            if (cmd == "connect" || cmd == "disconnect" || cmd == "ban" || cmd == "unban")
                            {
                                return ExecuteNetworkCommand(cmd, args);
                            }

                            // Check if it's a system control command
                            if (cmd == "start" || cmd == "stop" || cmd == "restart")
                            {
                                return ExecuteSystemCommand(cmd, args);
                            }

                            return "Command '" + cmd + "' not recognized. Type 'help' for available commands.";
                        }
                        catch (const std::exception& e)
                        {
                            return "Command execution failed: " + std::string(e.what());
                        }
                    }
                }
                else
                {
                    return "Neo system not available for command: " + command;
                }
            }
            catch (const std::exception& e)
            {
                return "Command delegation error: " + std::string(e.what());
            }
        }
    }
    catch (const std::exception& e)
    {
        return "Command execution error: " + std::string(e.what());
    }
}

std::shared_ptr<neo::node::NeoSystem> neo::console_service::ServiceProxy::GetNeoSystem() const
{
    return neoSystem_;
}

void neo::console_service::ServiceProxy::SetEventCallback(std::function<void(const std::string&)> callback)
{
    eventCallback_ = callback;
}

void neo::console_service::ServiceProxy::NotifyEvent(const std::string& event)
{
    if (eventCallback_)
    {
        eventCallback_(event);
    }
}

// Command execution helper methods
std::optional<std::string> ServiceProxy::ExecuteGenericCommand(const std::string& cmd,
                                                               const std::vector<std::string>& args)
{
    // Handle generic system commands that don't require special delegation
    if (cmd == "version")
    {
        return "Neo C++ Node v1.0.0";
    }
    else if (cmd == "uptime")
    {
        // Calculate system uptime if available
        return "Uptime information not yet implemented";
    }
    else if (cmd == "memory")
    {
        // Return memory usage information
        return "Memory usage information not yet implemented";
    }
    else if (cmd == "gc")
    {
        // Trigger garbage collection if applicable
        return "Garbage collection not applicable in C++";
    }

    return std::nullopt;  // Command not handled
}

std::string ServiceProxy::ExecuteBlockchainQuery(const std::string& cmd, const std::vector<std::string>& args)
{
    try
    {
        if (cmd == "block")
        {
            if (args.empty())
            {
                return "Usage: block <hash|index>";
            }
            return "Block query: " + args[0] + " (implementation pending)";
        }
        else if (cmd == "tx")
        {
            if (args.empty())
            {
                return "Usage: tx <hash>";
            }
            return "Transaction query: " + args[0] + " (implementation pending)";
        }
        else if (cmd == "account")
        {
            if (args.empty())
            {
                return "Usage: account <address>";
            }
            return "Account query: " + args[0] + " (implementation pending)";
        }

        return "Unknown blockchain query command: " + cmd;
    }
    catch (const std::exception& e)
    {
        return "Blockchain query error: " + std::string(e.what());
    }
}

std::string ServiceProxy::ExecuteNetworkCommand(const std::string& cmd, const std::vector<std::string>& args)
{
    try
    {
        if (cmd == "connect")
        {
            if (args.empty())
            {
                return "Usage: connect <host:port>";
            }
            return "Connect to peer: " + args[0] + " (implementation pending)";
        }
        else if (cmd == "disconnect")
        {
            if (args.empty())
            {
                return "Usage: disconnect <host:port>";
            }
            return "Disconnect from peer: " + args[0] + " (implementation pending)";
        }
        else if (cmd == "ban")
        {
            if (args.empty())
            {
                return "Usage: ban <host>";
            }
            return "Ban peer: " + args[0] + " (implementation pending)";
        }
        else if (cmd == "unban")
        {
            if (args.empty())
            {
                return "Usage: unban <host>";
            }
            return "Unban peer: " + args[0] + " (implementation pending)";
        }

        return "Unknown network command: " + cmd;
    }
    catch (const std::exception& e)
    {
        return "Network command error: " + std::string(e.what());
    }
}

std::string ServiceProxy::ExecuteSystemCommand(const std::string& cmd, const std::vector<std::string>& args)
{
    try
    {
        if (cmd == "start")
        {
            bool result = StartNode();
            return result ? "Node start initiated" : "Failed to start node";
        }
        else if (cmd == "stop")
        {
            bool result = StopNode();
            return result ? "Node stop initiated" : "Failed to stop node";
        }
        else if (cmd == "restart")
        {
            bool stopResult = StopNode();
            if (!stopResult)
            {
                return "Failed to stop node for restart";
            }

            // Brief delay to allow clean shutdown
            std::this_thread::sleep_for(std::chrono::seconds(1));

            bool startResult = StartNode();
            return startResult ? "Node restart completed" : "Node restart failed";
        }

        return "Unknown system command: " + cmd;
    }
    catch (const std::exception& e)
    {
        return "System command error: " + std::string(e.what());
    }
}

// Wallet command handler implementation
std::string ServiceProxy::HandleWalletCommands(const std::string& command)
{
    try
    {
        // Parse and execute wallet commands
        std::istringstream iss(command);
        std::string cmd, subcmd;
        iss >> cmd >> subcmd;

        // Get the wallet manager instance
        auto& walletManager = neo::wallets::WalletManager::GetInstance();

        if (subcmd == "create")
        {
            // Parse wallet path
            std::string path;
            iss >> path;

            if (path.empty())
            {
                return "Usage: wallet create <path>";
            }

            // Create wallet
            auto wallet = walletManager.CreateWallet(path);
            if (wallet)
            {
                walletManager.SetCurrentWallet(wallet);
                return "Wallet created successfully at: " + path;
            }
            else
            {
                return "Failed to create wallet at: " + path;
            }
        }
        else if (subcmd == "open")
        {
            // Parse wallet path
            std::string path;
            iss >> path;

            if (path.empty())
            {
                return "Usage: wallet open <path>";
            }

            // Open wallet
            auto wallet = walletManager.OpenWallet(path);
            if (wallet)
            {
                walletManager.SetCurrentWallet(wallet);
                return "Wallet opened successfully: " + path;
            }
            else
            {
                return "Failed to open wallet: " + path;
            }
        }
        else if (subcmd == "close")
        {
            // Parse wallet path (optional)
            std::string path;
            iss >> path;

            if (!path.empty())
            {
                // Close specific wallet
                if (walletManager.CloseWallet(path))
                {
                    return "Wallet closed: " + path;
                }
                else
                {
                    return "Failed to close wallet: " + path;
                }
            }
            else
            {
                // Close current wallet
                auto currentWallet = walletManager.GetCurrentWallet();
                if (currentWallet)
                {
                    std::string walletPath = currentWallet->GetPath();
                    if (walletManager.CloseWallet(currentWallet))
                    {
                        return "Current wallet closed: " + walletPath;
                    }
                    else
                    {
                        return "Failed to close current wallet";
                    }
                }
                else
                {
                    return "No wallet is currently open";
                }
            }
        }
        else if (subcmd == "list")
        {
            // List all wallets
            const auto& wallets = walletManager.GetWallets();
            if (wallets.empty())
            {
                return "No wallets loaded";
            }

            std::stringstream result;
            result << "Loaded wallets:\n";
            auto currentWallet = walletManager.GetCurrentWallet();

            for (const auto& wallet : wallets)
            {
                result << "  - " << wallet->GetPath();
                if (wallet == currentWallet)
                {
                    result << " (current)";
                }
                result << "\n";
            }

            return result.str();
        }
        else if (subcmd == "balance")
        {
            // Get balance from current wallet
            auto currentWallet = walletManager.GetCurrentWallet();
            if (!currentWallet)
            {
                return "No wallet is currently open";
            }

            // Parse optional address
            std::string address;
            iss >> address;

            if (!address.empty())
            {
                // Get balance for specific address
                try
                {
                    auto account = currentWallet->GetAccount(address);
                    if (account)
                    {
                        // Get balance from blockchain
                        if (neoSystem_)
                        {
                            auto snapshot = neoSystem_->GetSnapshot();
                            if (snapshot)
                            {
                                // Query NEO and GAS balances
                                auto neoBalance = GetTokenBalance(snapshot, account->GetScriptHash(), "NEO");
                                auto gasBalance = GetTokenBalance(snapshot, account->GetScriptHash(), "GAS");

                                std::stringstream result;
                                result << "Balance for " << address << ":\n";
                                result << "  NEO: " << neoBalance << "\n";
                                result << "  GAS: " << gasBalance << "\n";
                                return result.str();
                            }
                        }
                        return "Unable to query blockchain for balance";
                    }
                    else
                    {
                        return "Address not found in wallet: " + address;
                    }
                }
                catch (const std::exception& e)
                {
                    return "Error getting balance: " + std::string(e.what());
                }
            }
            else
            {
                // Get total balance for all accounts in wallet
                try
                {
                    const auto& accounts = currentWallet->GetAccounts();
                    if (accounts.empty())
                    {
                        return "Wallet has no accounts";
                    }

                    int64_t totalNeo = 0;
                    int64_t totalGas = 0;

                    if (neoSystem_)
                    {
                        auto snapshot = neoSystem_->GetSnapshot();
                        if (snapshot)
                        {
                            for (const auto& account : accounts)
                            {
                                totalNeo += GetTokenBalance(snapshot, account->GetScriptHash(), "NEO");
                                totalGas += GetTokenBalance(snapshot, account->GetScriptHash(), "GAS");
                            }
                        }
                    }

                    std::stringstream result;
                    result << "Total wallet balance:\n";
                    result << "  NEO: " << totalNeo << "\n";
                    result << "  GAS: " << totalGas << "\n";
                    result << "  Accounts: " << accounts.size() << "\n";
                    return result.str();
                }
                catch (const std::exception& e)
                {
                    return "Error calculating total balance: " + std::string(e.what());
                }
            }
        }
        else if (subcmd == "claim")
        {
            // Claim GAS from current wallet
            auto currentWallet = walletManager.GetCurrentWallet();
            if (!currentWallet)
            {
                return "No wallet is currently open";
            }

            // Parse optional address
            std::string address;
            iss >> address;

            if (!address.empty())
            {
                // Claim GAS for specific address
                return "GAS claim for address '" + address + "' requires transaction signing (pending implementation)";
            }
            else
            {
                // Claim GAS for all addresses in wallet
                return "GAS claim for all addresses requires transaction signing (pending implementation)";
            }
        }
        else
        {
            return "Unknown wallet subcommand: " + subcmd + ". Available: create, open, close, list, balance, claim";
        }
    }
    catch (const std::exception& e)
    {
        return "Error processing wallet command: " + std::string(e.what());
    }
}

std::string ServiceProxy::HandleContractCommands(const std::string& command)
{
    try
    {
        std::istringstream iss(command);
        std::string cmd, subcmd;
        iss >> cmd >> subcmd;

        if (subcmd == "deploy" || subcmd == "invoke" || subcmd == "get")
        {
            return "Contract command '" + subcmd + "' recognized. Implementation requires blockchain access.";
        }

        return "Unknown contract subcommand: " + subcmd;
    }
    catch (const std::exception& e)
    {
        return "Error processing contract command: " + std::string(e.what());
    }
}

std::string ServiceProxy::HandleTransactionCommands(const std::string& command)
{
    try
    {
        std::istringstream iss(command);
        std::string cmd, subcmd;
        iss >> cmd >> subcmd;

        if (subcmd == "send" || subcmd == "get" || subcmd == "broadcast")
        {
            return "Transaction command '" + subcmd + "' recognized. Implementation requires blockchain access.";
        }

        return "Unknown transaction subcommand: " + subcmd;
    }
    catch (const std::exception& e)
    {
        return "Error processing transaction command: " + std::string(e.what());
    }
}

std::string ServiceProxy::HandleConsensusCommands(const std::string& command)
{
    try
    {
        std::istringstream iss(command);
        std::string cmd, subcmd;
        iss >> cmd >> subcmd;

        if (subcmd == "start" || subcmd == "stop" || subcmd == "status")
        {
            return "Consensus command '" + subcmd + "' recognized. Implementation requires consensus service.";
        }

        return "Unknown consensus subcommand: " + subcmd;
    }
    catch (const std::exception& e)
    {
        return "Error processing consensus command: " + std::string(e.what());
    }
}

std::string ServiceProxy::HandlePluginCommands(const std::string& command)
{
    try
    {
        std::istringstream iss(command);
        std::string cmd, subcmd;
        iss >> cmd >> subcmd;

        if (subcmd == "list" || subcmd == "load" || subcmd == "unload")
        {
            return "Plugin command '" + subcmd + "' recognized. Implementation requires plugin manager.";
        }

        return "Unknown plugin subcommand: " + subcmd;
    }
    catch (const std::exception& e)
    {
        return "Error processing plugin command: " + std::string(e.what());
    }
}

std::string ServiceProxy::HandleConfigCommands(const std::string& command)
{
    try
    {
        std::istringstream iss(command);
        std::string cmd, subcmd;
        iss >> cmd >> subcmd;

        if (subcmd == "get" || subcmd == "set" || subcmd == "save")
        {
            return "Config command '" + subcmd + "' recognized. Implementation requires configuration access.";
        }

        return "Unknown config subcommand: " + subcmd;
    }
    catch (const std::exception& e)
    {
        return "Error processing config command: " + std::string(e.what());
    }
}

int64_t ServiceProxy::GetTokenBalance(std::shared_ptr<persistence::DataCache> snapshot, const io::UInt160& scriptHash,
                                      const std::string& tokenSymbol)
{
    try
    {
        if (!snapshot || !neoSystem_)
        {
            return 0;
        }

        // Get the appropriate native token contract
        std::shared_ptr<smartcontract::native::NativeContract> tokenContract;

        if (tokenSymbol == "NEO")
        {
            tokenContract = smartcontract::native::NeoToken::GetInstance();
        }
        else if (tokenSymbol == "GAS")
        {
            tokenContract = smartcontract::native::GasToken::GetInstance();
        }
        else
        {
            // Unsupported token
            return 0;
        }

        if (!tokenContract)
        {
            return 0;
        }

        // Create a temporary application engine to query balance
        auto engine = smartcontract::ApplicationEngine::Create(smartcontract::TriggerType::Application,
                                                               nullptr,  // No transaction
                                                               snapshot,
                                                               nullptr,  // No persisting block
                                                               smartcontract::ApplicationEngine::TestModeGas);

        if (!engine)
        {
            return 0;
        }

        // Call balanceOf method on the token contract
        std::vector<std::shared_ptr<vm::StackItem>> args;
        args.push_back(vm::StackItem::Create(scriptHash));

        auto result = tokenContract->Invoke(*engine, "balanceOf", args, smartcontract::CallFlags::ReadStates);

        if (result && result->IsInteger())
        {
            return result->GetInteger();
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to get " << tokenSymbol << " balance: " << e.what() << std::endl;
        return 0;
    }
}
}  // namespace neo::console_service
