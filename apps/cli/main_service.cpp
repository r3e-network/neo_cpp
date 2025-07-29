#include "main_service.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iostream>
#include <neo/io/json.h>
#include <neo/network/p2p/network_synchronizer.h>
#include <neo/node/neo_system.h>
#include <neo/persistence/rocksdb_store.h>
#include <neo/persistence/store_provider.h>
#include <neo/rpc/rpc_server.h>
#include <neo/settings.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/wallets/wallet.h>
#include <neo/wallets/wallet_factory.h>
#include <sstream>
#include <thread>

namespace neo::cli
{
MainService::MainService() : running_(false)
{
    InitializeTypeConverters();
    InitializeCommands();
}

MainService::~MainService()
{
    Stop();
}

void MainService::Run(const std::vector<std::string>& args)
{
    if (args.empty())
    {
        // Interactive mode
        CommandLineOptions options;
        Start(options);
        RunConsole();
        Stop();
    }
    else
    {
        // Command line mode
        OnStartWithCommandLine(args);
    }
}

void MainService::Start(const CommandLineOptions& options)
{
    if (neoSystem_)
        return;

    try
    {
        // Load settings
        Settings settings;
        if (!options.Config.empty())
        {
            settings = Settings::Load(options.Config);
        }
        else
        {
            settings = Settings::Default();
        }

        // Override settings from command line
        if (!options.DbEngine.empty())
            settings.Storage.Engine = options.DbEngine;
        if (!options.DbPath.empty())
            settings.Storage.Path = options.DbPath;

        // Create store provider
        auto store = std::make_shared<persistence::RocksDBStore>(settings.Storage.Path);
        auto storeProvider = std::make_shared<persistence::StoreProvider>(store);

        // Create Neo system
        neoSystem_ = std::make_shared<node::NeoSystem>(settings, storeProvider);

        // Start Neo system
        neoSystem_->Start();

        ConsoleHelper::Info("Neo system started");
        ConsoleHelper::Info("Network: " + std::to_string(settings.Protocol.Network));
        ConsoleHelper::Info("Storage: " + settings.Storage.Path);

        // Start RPC server if enabled
        if (settings.RPC.Enabled)
        {
            rpcServer_ = std::make_shared<rpc::RPCServer>(neoSystem_, settings.RPC);
            rpcServer_->Start();
            ConsoleHelper::Info("RPC server started on port " + std::to_string(settings.RPC.Port));
        }

        // Open wallet if specified
        if (!options.Wallet.empty())
        {
            OnOpenWallet(options.Wallet, options.Password);
        }

        // Show synchronization status
        auto synchronizer = neoSystem_->GetNetworkSynchronizer();
        if (synchronizer)
        {
            synchronizer->SetStateChangedCallback(
                [this](network::p2p::SynchronizationState state)
                {
                    switch (state)
                    {
                        case network::p2p::SynchronizationState::NotSynchronizing:
                            ConsoleHelper::Info("Synchronization: Not synchronizing");
                            break;
                        case network::p2p::SynchronizationState::SynchronizingHeaders:
                            ConsoleHelper::Info("Synchronization: Synchronizing headers");
                            break;
                        case network::p2p::SynchronizationState::SynchronizingBlocks:
                            ConsoleHelper::Info("Synchronization: Synchronizing blocks");
                            break;
                        case network::p2p::SynchronizationState::Synchronized:
                            ConsoleHelper::Info("Synchronization: Synchronized");
                            break;
                    }
                });

            synchronizer->SetBlockReceivedCallback(
                [this](const std::shared_ptr<ledger::Block>& block)
                {
                    if (block->GetIndex() % 1000 == 0)
                    {
                        ConsoleHelper::Info("Block received: " + std::to_string(block->GetIndex()));
                    }
                });
        }

        running_ = true;
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error("Failed to start Neo system: " + std::string(ex.what()));
        throw;
    }
}

void MainService::Stop()
{
    if (!running_)
        return;

    running_ = false;

    // Stop console thread
    if (consoleThread_.joinable())
        consoleThread_.join();

    // Close wallet
    currentWallet_.reset();

    // Stop RPC server
    if (rpcServer_)
    {
        rpcServer_->Stop();
        rpcServer_.reset();
        ConsoleHelper::Info("RPC server stopped");
    }

    // Stop Neo system
    if (neoSystem_)
    {
        neoSystem_->Stop();
        neoSystem_.reset();
        ConsoleHelper::Info("Neo system stopped");
    }
}

void MainService::RegisterCommand(const std::string& name, const CommandHandler& handler, const std::string& category)
{
    commands_[name] = handler;

    if (!category.empty())
    {
        commandsByCategory_[category][name] = handler;
    }
}

void MainService::RegisterTypeConverter(const std::string& typeName, const TypeConverter& converter)
{
    typeConverters_[typeName] = converter;
}

std::shared_ptr<node::NeoSystem> MainService::GetNeoSystem() const
{
    return neoSystem_;
}

std::shared_ptr<wallets::Wallet> MainService::GetCurrentWallet() const
{
    return currentWallet_;
}

void MainService::OnCommand(const std::string& command)
{
    if (command.empty())
        return;

    // Parse command and arguments
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg)
    {
        args.push_back(arg);
    }

    // Find command handler
    auto it = commands_.find(cmd);
    if (it != commands_.end())
    {
        try
        {
            it->second(args);
        }
        catch (const std::exception& ex)
        {
            ConsoleHelper::Error(ex.what());
        }
    }
    else
    {
        ConsoleHelper::Error("Command not found: " + cmd);
    }
}

void MainService::OnStartWithCommandLine(const std::vector<std::string>& args)
{
    // Parse command line options
    CommandLineOptions options;

    for (size_t i = 0; i < args.size(); i++)
    {
        if (args[i] == "-c" || args[i] == "--config")
        {
            if (i + 1 < args.size())
                options.Config = args[++i];
        }
        else if (args[i] == "-w" || args[i] == "--wallet")
        {
            if (i + 1 < args.size())
                options.Wallet = args[++i];
        }
        else if (args[i] == "-p" || args[i] == "--password")
        {
            if (i + 1 < args.size())
                options.Password = args[++i];
        }
        else if (args[i] == "--db-engine")
        {
            if (i + 1 < args.size())
                options.DbEngine = args[++i];
        }
        else if (args[i] == "--db-path")
        {
            if (i + 1 < args.size())
                options.DbPath = args[++i];
        }
        else if (args[i] == "--noverify")
        {
            options.NoVerify = true;
        }
        else if (args[i] == "--plugins")
        {
            while (i + 1 < args.size() && args[i + 1][0] != '-')
            {
                options.Plugins.push_back(args[++i]);
            }
        }
        else if (args[i] == "--verbose")
        {
            if (i + 1 < args.size())
                options.Verbose = std::stoi(args[++i]);
        }
    }

    // Start the service
    Start(options);
}

void MainService::OnHelp(const std::string& category)
{
    if (!category.empty())
    {
        auto it = commandsByCategory_.find(category);
        if (it != commandsByCategory_.end())
        {
            ConsoleHelper::Info(category + " Commands:");
            for (const auto& [name, handler] : it->second)
            {
                ConsoleHelper::Info("  " + name);
            }
        }
        else
        {
            ConsoleHelper::Error("Category not found: " + category);
        }
        return;
    }

    // Show all categories
    ConsoleHelper::Info("Neo C++ CLI Commands:");
    ConsoleHelper::Info("");

    for (const auto& [category, commands] : commandsByCategory_)
    {
        ConsoleHelper::Info(category + " Commands:");
        for (const auto& [name, handler] : commands)
        {
            ConsoleHelper::Info("  " + name);
        }
        ConsoleHelper::Info("");
    }

    ConsoleHelper::Info("Use 'help <category>' for detailed help on a category");
}

void MainService::OnExit()
{
    running_ = false;
}

void MainService::OnClear()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void MainService::OnVersion()
{
    ConsoleHelper::Info("Neo C++ CLI v1.0.0");
    ConsoleHelper::Info("Neo Protocol Version: 3.0");

    if (neoSystem_)
    {
        auto blockchain = neoSystem_->GetBlockchain();
        if (blockchain)
        {
            ConsoleHelper::Info("Current Block Height: " + std::to_string(blockchain->GetHeight()));
            ConsoleHelper::Info("Current Header Height: " + std::to_string(blockchain->GetHeaderHeight()));
        }
    }
}

void MainService::InitializeCommands()
{
    // Base Commands
    RegisterCommand(
        "help",
        [this](const std::vector<std::string>& args)
        {
            if (args.empty())
                OnHelp();
            else
                OnHelp(args[0]);
            return true;
        },
        "Base");

    RegisterCommand(
        "exit",
        [this](const std::vector<std::string>& args)
        {
            OnExit();
            return true;
        },
        "Base");

    RegisterCommand(
        "clear",
        [this](const std::vector<std::string>& args)
        {
            OnClear();
            return true;
        },
        "Base");

    RegisterCommand(
        "version",
        [this](const std::vector<std::string>& args)
        {
            OnVersion();
            return true;
        },
        "Base");

    // Initialize command groups
    InitializeBlockchainCommands();
    InitializeNodeCommands();
    InitializeWalletCommands();
}

void MainService::InitializeBlockchainCommands()
{
    RegisterCommand(
        "showblock",
        [this](const std::vector<std::string>& args)
        {
            if (args.empty())
            {
                ConsoleHelper::Error("Missing argument: index or hash");
                return false;
            }
            OnShowBlock(args[0]);
            return true;
        },
        "Blockchain");

    RegisterCommand(
        "showheader",
        [this](const std::vector<std::string>& args)
        {
            if (args.empty())
            {
                ConsoleHelper::Error("Missing argument: index or hash");
                return false;
            }
            OnShowHeader(args[0]);
            return true;
        },
        "Blockchain");

    RegisterCommand(
        "showtx",
        [this](const std::vector<std::string>& args)
        {
            if (args.empty())
            {
                ConsoleHelper::Error("Missing argument: hash");
                return false;
            }
            try
            {
                io::UInt256 hash = io::UInt256::Parse(args[0]);
                OnShowTransaction(hash);
            }
            catch (const std::exception& ex)
            {
                ConsoleHelper::Error(ex.what());
            }
            return true;
        },
        "Blockchain");
}

void MainService::InitializeNodeCommands()
{
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

void MainService::InitializeWalletCommands()
{
    RegisterCommand(
        "openwallet",
        [this](const std::vector<std::string>& args)
        {
            if (args.empty())
            {
                ConsoleHelper::Error("Missing argument: path");
                return false;
            }

            std::string password;
            if (args.size() > 1)
            {
                password = args[1];
            }
            else
            {
                password = ConsoleHelper::ReadPassword("Password: ");
            }

            OnOpenWallet(args[0], password);
            return true;
        },
        "Wallet");

    RegisterCommand(
        "closewallet",
        [this](const std::vector<std::string>& args)
        {
            OnCloseWallet();
            return true;
        },
        "Wallet");

    RegisterCommand(
        "showbalance",
        [this](const std::vector<std::string>& args)
        {
            OnShowBalance();
            return true;
        },
        "Wallet");

    RegisterCommand(
        "showaddress",
        [this](const std::vector<std::string>& args)
        {
            OnShowAddress();
            return true;
        },
        "Wallet");

    RegisterCommand(
        "transfer",
        [this](const std::vector<std::string>& args)
        {
            if (args.size() < 3)
            {
                ConsoleHelper::Error("Usage: transfer <asset> <address> <amount>");
                return false;
            }

            std::string asset = args[0];
            std::string address = args[1];
            double amount = std::stod(args[2]);

            io::UInt160 assetId;
            if (asset == "neo" || asset == "NEO")
            {
                assetId = smartcontract::native::NeoToken::SCRIPT_HASH;
            }
            else if (asset == "gas" || asset == "GAS")
            {
                assetId = smartcontract::native::GasToken::SCRIPT_HASH;
            }
            else
            {
                assetId = io::UInt160::Parse(asset);
            }

            OnTransfer(assetId, address, amount);
            return true;
        },
        "Wallet");
}

void MainService::InitializeTypeConverters()
{
    // Basic type converters
    RegisterTypeConverter("string",
                          [](const std::vector<std::string>& args, bool canConsumeAll) -> void*
                          {
                              if (args.empty())
                                  return new std::string("");
                              return new std::string(args[0]);
                          });

    RegisterTypeConverter("int",
                          [](const std::vector<std::string>& args, bool canConsumeAll) -> void*
                          {
                              if (args.empty())
                                  throw std::runtime_error("Missing argument for int");
                              return new int(std::stoi(args[0]));
                          });

    RegisterTypeConverter("uint32_t",
                          [](const std::vector<std::string>& args, bool canConsumeAll) -> void*
                          {
                              if (args.empty())
                                  throw std::runtime_error("Missing argument for uint32_t");
                              return new uint32_t(std::stoul(args[0]));
                          });
}

void MainService::RunConsole()
{
    // Print welcome message
    ConsoleHelper::Info("Neo C++ CLI v1.0.0");
    ConsoleHelper::Info("Type 'help' for a list of commands");

    // Run console loop
    running_ = true;
    while (running_)
    {
        std::string command = ConsoleHelper::ReadLine("neo> ");
        if (command.empty())
            continue;

        OnCommand(command);
    }
}

// Blockchain command implementations
void MainService::OnShowBlock(const std::string& indexOrHash)
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto blockchain = neoSystem_->GetBlockchain();
        std::shared_ptr<ledger::Block> block;

        if (indexOrHash.size() == 64)
        {
            // Hash
            io::UInt256 hash = io::UInt256::Parse(indexOrHash);
            block = blockchain->GetBlock(hash);
        }
        else
        {
            // Index
            uint32_t index = std::stoul(indexOrHash);
            block = blockchain->GetBlock(index);
        }

        if (!block)
        {
            ConsoleHelper::Error("Block not found");
            return;
        }

        ConsoleHelper::Info("Block " + std::to_string(block->GetIndex()) + ":");
        ConsoleHelper::Info("  Hash: " + block->GetHash().ToString());
        ConsoleHelper::Info("  Previous Hash: " + block->GetPrevHash().ToString());
        ConsoleHelper::Info("  Merkle Root: " + block->GetMerkleRoot().ToString());
        ConsoleHelper::Info("  Timestamp: " + std::to_string(block->GetTimestamp()));
        ConsoleHelper::Info("  Version: " + std::to_string(block->GetVersion()));
        ConsoleHelper::Info("  Next Consensus: " + block->GetNextConsensus().ToString());
        ConsoleHelper::Info("  Transactions: " + std::to_string(block->GetTransactions().size()));
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

void MainService::OnShowHeader(const std::string& indexOrHash)
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto blockchain = neoSystem_->GetBlockchain();
        std::shared_ptr<ledger::BlockHeader> header;

        if (indexOrHash.size() == 64)
        {
            // Hash
            io::UInt256 hash = io::UInt256::Parse(indexOrHash);
            header = blockchain->GetHeader(hash);
        }
        else
        {
            // Index
            uint32_t index = std::stoul(indexOrHash);
            header = blockchain->GetHeader(index);
        }

        if (!header)
        {
            ConsoleHelper::Error("Header not found");
            return;
        }

        ConsoleHelper::Info("Header " + std::to_string(header->GetIndex()) + ":");
        ConsoleHelper::Info("  Hash: " + header->GetHash().ToString());
        ConsoleHelper::Info("  Previous Hash: " + header->GetPrevHash().ToString());
        ConsoleHelper::Info("  Merkle Root: " + header->GetMerkleRoot().ToString());
        ConsoleHelper::Info("  Timestamp: " + std::to_string(header->GetTimestamp()));
        ConsoleHelper::Info("  Version: " + std::to_string(header->GetVersion()));
        ConsoleHelper::Info("  Next Consensus: " + header->GetNextConsensus().ToString());
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

void MainService::OnShowTransaction(const io::UInt256& hash)
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto blockchain = neoSystem_->GetBlockchain();
        auto tx = blockchain->GetTransaction(hash);
        if (!tx)
        {
            ConsoleHelper::Error("Transaction not found");
            return;
        }

        ConsoleHelper::Info("Transaction " + hash.ToString() + ":");
        ConsoleHelper::Info("  Version: " + std::to_string(tx->GetVersion()));
        ConsoleHelper::Info("  Nonce: " + std::to_string(tx->GetNonce()));
        ConsoleHelper::Info("  Sender: " + tx->GetSender().ToString());
        ConsoleHelper::Info("  System Fee: " + std::to_string(tx->GetSystemFee()));
        ConsoleHelper::Info("  Network Fee: " + std::to_string(tx->GetNetworkFee()));
        ConsoleHelper::Info("  Valid Until Block: " + std::to_string(tx->GetValidUntilBlock()));
        ConsoleHelper::Info("  Script: " + tx->GetScript().ToHexString());
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

// Node command implementations
void MainService::OnShowState()
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto blockchain = neoSystem_->GetBlockchain();
        auto localNode = neoSystem_->GetLocalNode();
        auto memPool = neoSystem_->GetMemPool();
        auto synchronizer = neoSystem_->GetNetworkSynchronizer();

        ConsoleHelper::Info("Node State:");
        ConsoleHelper::Info("  Block Height: " + std::to_string(blockchain->GetHeight()));
        ConsoleHelper::Info("  Block Hash: " + blockchain->GetCurrentBlockHash().ToString());
        ConsoleHelper::Info("  Header Height: " + std::to_string(blockchain->GetHeaderHeight()));
        ConsoleHelper::Info("  Header Hash: " + blockchain->GetCurrentHeaderHash().ToString());
        ConsoleHelper::Info("  Connected Peers: " + std::to_string(localNode->GetConnectedCount()));
        ConsoleHelper::Info("  Memory Pool Size: " + std::to_string(memPool->GetCount()));

        if (synchronizer)
        {
            auto state = synchronizer->GetState();
            std::string stateStr;
            switch (state)
            {
                case network::p2p::SynchronizationState::NotSynchronizing:
                    stateStr = "Not synchronizing";
                    break;
                case network::p2p::SynchronizationState::SynchronizingHeaders:
                    stateStr = "Synchronizing headers";
                    break;
                case network::p2p::SynchronizationState::SynchronizingBlocks:
                    stateStr = "Synchronizing blocks";
                    break;
                case network::p2p::SynchronizationState::Synchronized:
                    stateStr = "Synchronized";
                    break;
            }
            ConsoleHelper::Info("  Synchronization State: " + stateStr);
            ConsoleHelper::Info("  Current Block Index: " + std::to_string(synchronizer->GetCurrentBlockIndex()));
            ConsoleHelper::Info("  Target Block Index: " + std::to_string(synchronizer->GetTargetBlockIndex()));
        }
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
        auto memPool = neoSystem_->GetMemPool();
        auto transactions = memPool->GetTransactions();

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
        auto peers = localNode->GetConnectedNodes();

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

// Wallet command implementations
void MainService::OnOpenWallet(const std::string& path, const std::string& password)
{
    try
    {
        // Close current wallet if open
        if (currentWallet_)
        {
            currentWallet_.reset();
        }

        // Open wallet
        currentWallet_ = wallets::WalletFactory::Open(path, password);

        ConsoleHelper::Info("Wallet opened: " + path);

        // Show wallet info
        auto accounts = currentWallet_->GetAccounts();
        ConsoleHelper::Info("Accounts: " + std::to_string(accounts.size()));
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error("Failed to open wallet: " + std::string(ex.what()));
    }
}

void MainService::OnCloseWallet()
{
    if (!currentWallet_)
    {
        ConsoleHelper::Error("No wallet is open");
        return;
    }

    currentWallet_.reset();
    ConsoleHelper::Info("Wallet closed");
}

void MainService::OnShowBalance()
{
    if (!currentWallet_)
    {
        ConsoleHelper::Error("No wallet is open");
        return;
    }

    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto accounts = currentWallet_->GetAccounts();

        for (const auto& account : accounts)
        {
            ConsoleHelper::Info("Account: " + account->GetAddress());

            // Get NEO balance
            auto neoBalance =
                currentWallet_->GetBalance(smartcontract::native::NeoToken::SCRIPT_HASH, account->GetScriptHash());

            // Get GAS balance
            auto gasBalance =
                currentWallet_->GetBalance(smartcontract::native::GasToken::SCRIPT_HASH, account->GetScriptHash());

            ConsoleHelper::Info("  NEO: " + std::to_string(neoBalance));
            ConsoleHelper::Info("  GAS: " + std::to_string(gasBalance));
        }
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

void MainService::OnShowBalance(const io::UInt160& assetId)
{
    if (!currentWallet_)
    {
        ConsoleHelper::Error("No wallet is open");
        return;
    }

    try
    {
        auto accounts = currentWallet_->GetAccounts();

        for (const auto& account : accounts)
        {
            auto balance = currentWallet_->GetBalance(assetId, account->GetScriptHash());
            ConsoleHelper::Info(account->GetAddress() + ": " + std::to_string(balance));
        }
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

void MainService::OnShowAddress()
{
    if (!currentWallet_)
    {
        ConsoleHelper::Error("No wallet is open");
        return;
    }

    try
    {
        auto accounts = currentWallet_->GetAccounts();

        ConsoleHelper::Info("Addresses:");
        for (const auto& account : accounts)
        {
            ConsoleHelper::Info("  " + account->GetAddress());
        }
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

void MainService::OnTransfer(const io::UInt160& assetId, const std::string& address, double amount)
{
    if (!currentWallet_)
    {
        ConsoleHelper::Error("No wallet is open");
        return;
    }

    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        // Create transfer transaction
        auto tx = currentWallet_->CreateTransferTransaction(assetId, address, amount);

        // Sign transaction
        currentWallet_->SignTransaction(tx);

        // Send transaction
        auto memPool = neoSystem_->GetMemPool();
        auto result = memPool->AddTransaction(tx);

        if (result)
        {
            ConsoleHelper::Info("Transaction sent: " + tx->GetHash().ToString());
        }
        else
        {
            ConsoleHelper::Error("Failed to send transaction");
        }
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error("Failed to transfer: " + std::string(ex.what()));
    }
}
}  // namespace neo::cli