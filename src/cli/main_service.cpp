#include <neo/cli/main_service.h>
#include <neo/cli/type_converters.h>
#include <neo/cryptography/ecc/ec_point.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/protocol_settings.h>
#include <neo/rpc/rpc_server.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/policy_contract.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

namespace neo::cli
{
MainService::MainService() : running_(false)
{
    InitializeTypeConverters();
    InitializeCommands();
}

void MainService::InitializeTypeConverters()
{
    // Register all default type converters from the TypeConverters class
    auto& converters = TypeConverters::Instance();

    // Copy all converters to our local map
    for (const auto& [typeName, converter] : converters.GetAllConverters())
    {
        RegisterTypeConverter(typeName, converter);
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

MainService::~MainService() { Stop(); }

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
    if (neoSystem_) return;

    // Create Neo system with settings from config file or defaults
    // Configuration can be provided via command line or config file
    std::string dbEngine = options.DbEngine.empty() ? "memory" : options.DbEngine;
    std::string dbPath = options.DbPath.empty() ? "./data" : options.DbPath;

    // Create protocol settings - can be loaded from config file if provided
    ProtocolSettings protocolSettings;
    if (!options.ConfigPath.empty())
    {
        // Config file path provided - attempt to load settings
        try
        {
            protocolSettings.LoadFromFile(options.ConfigPath);
        }
        catch (const std::exception& e)
        {
            LOG_WARNING("Failed to load config file: {} - using defaults", e.what());
        }
    }

    neoSystem_ = std::make_shared<node::NeoSystem>(protocolSettings, dbEngine, dbPath);

    // Native contracts are initialized internally by NeoSystem

    // Start RPC server if enabled with configuration
    // RPC configuration loaded from settings or defaults
    rpc::RpcConfig rpcConfig;
    rpcConfig.port = protocolSettings.GetRpcPort().value_or(10332);  // Default RPC port
    rpcConfig.max_concurrent_requests = 40;

    rpcServer_ = std::make_shared<rpc::RpcServer>(rpcConfig);
    rpcServer_->Start();

    // Start Neo system
    neoSystem_->Start();

    // Open wallet if specified
    if (!options.Wallet.empty())
    {
        OnOpenWallet(options.Wallet, options.Password);
    }

    running_ = true;
}

void MainService::Stop()
{
    if (!running_) return;

    running_ = false;

    // Stop console thread
    if (consoleThread_.joinable()) consoleThread_.join();

    // Close wallet
    currentWallet_.reset();

    // Stop RPC server
    if (rpcServer_)
    {
        rpcServer_->Stop();
        rpcServer_.reset();
    }

    // Stop Neo system
    if (neoSystem_)
    {
        neoSystem_->Stop();
        neoSystem_.reset();
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

std::shared_ptr<node::NeoSystem> MainService::GetNeoSystem() const { return neoSystem_; }

std::shared_ptr<wallets::Wallet> MainService::GetCurrentWallet() const { return currentWallet_; }

void MainService::OnCommand(const std::string& command)
{
    if (command.empty()) return;

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
            if (i + 1 < args.size()) options.Config = args[++i];
        }
        else if (args[i] == "-w" || args[i] == "--wallet")
        {
            if (i + 1 < args.size()) options.Wallet = args[++i];
        }
        else if (args[i] == "-p" || args[i] == "--password")
        {
            if (i + 1 < args.size()) options.Password = args[++i];
        }
        else if (args[i] == "--db-engine")
        {
            if (i + 1 < args.size()) options.DbEngine = args[++i];
        }
        else if (args[i] == "--db-path")
        {
            if (i + 1 < args.size()) options.DbPath = args[++i];
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
            if (i + 1 < args.size()) options.Verbose = std::stoi(args[++i]);
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
    ConsoleHelper::Info("Categories:");
    for (const auto& [category, commands] : commandsByCategory_)
    {
        ConsoleHelper::Info("  " + category);
    }

    // Show commands without category
    std::vector<std::string> uncategorizedCommands;
    for (const auto& [name, handler] : commands_)
    {
        bool found = false;
        for (const auto& [category, commands] : commandsByCategory_)
        {
            if (commands.find(name) != commands.end())
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            uncategorizedCommands.push_back(name);
        }
    }

    if (!uncategorizedCommands.empty())
    {
        ConsoleHelper::Info("Uncategorized Commands:");
        for (const auto& name : uncategorizedCommands)
        {
            ConsoleHelper::Info("  " + name);
        }
    }
}

void MainService::OnExit() { running_ = false; }

void MainService::OnClear()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void MainService::OnVersion() { ConsoleHelper::Info("Neo C++ CLI v1.0.0"); }

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
        if (command.empty()) continue;

        OnCommand(command);
    }
}
}  // namespace neo::cli
