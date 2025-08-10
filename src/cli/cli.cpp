#include <neo/cli/cli.h>
#include <neo/cli/command_handler.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

namespace neo::cli
{
CLI::CLI(std::shared_ptr<node::NeoSystem> neoSystem, std::shared_ptr<rpc::RpcServer> rpcServer)
    : neoSystem_(neoSystem), rpcServer_(rpcServer), running_(false)
{
    commandHandler_ = std::make_shared<CommandHandler>(neoSystem, rpcServer);
    InitializeCommands();
}

CLI::~CLI() { Stop(); }

void CLI::Start()
{
    if (running_) return;

    running_ = true;
    cliThread_ = std::thread(&CLI::RunCLI, this);
}

void CLI::Stop()
{
    if (!running_) return;

    running_ = false;
    condition_.notify_all();

    if (cliThread_.joinable()) cliThread_.join();
}

bool CLI::IsRunning() const { return running_; }

std::shared_ptr<node::NeoSystem> CLI::GetNeoSystem() const { return neoSystem_; }

std::shared_ptr<rpc::RpcServer> CLI::GetRPCServer() const { return rpcServer_; }

std::shared_ptr<wallets::Wallet> CLI::GetWallet() const { return commandHandler_->GetWallet(); }

void CLI::SetWallet(std::shared_ptr<wallets::Wallet> wallet) { commandHandler_->SetWallet(wallet); }

void CLI::RegisterCommand(const std::string& command, std::function<bool(const std::vector<std::string>&)> handler,
                          const std::string& help)
{
    std::lock_guard<std::mutex> lock(mutex_);
    commands_[command] = handler;
    commandHelp_[command] = help;
}

void CLI::UnregisterCommand(const std::string& command)
{
    std::lock_guard<std::mutex> lock(mutex_);
    commands_.erase(command);
    commandHelp_.erase(command);
}

bool CLI::ExecuteCommand(const std::string& command)
{
    auto [cmd, args] = ParseCommand(command);

    if (cmd.empty()) return true;

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = commands_.find(cmd);
    if (it == commands_.end())
    {
        std::cout << "Unknown command: " << cmd << std::endl;
        return false;
    }

    try
    {
        return it->second(args);
    }
    catch (const std::exception& ex)
    {
        std::cout << "Error: " << ex.what() << std::endl;
        return false;
    }
}

void CLI::RunCLI()
{
    std::cout << "Neo C++ CLI" << std::endl;
    std::cout << "Type 'help' for a list of commands" << std::endl;

    while (running_)
    {
        std::cout << "neo> ";
        std::string command;
        std::getline(std::cin, command);

        if (!running_) break;

        ExecuteCommand(command);
    }
}

void CLI::InitializeCommands()
{
    RegisterCommand(
        "help", [this](const std::vector<std::string>& args) { return commandHandler_->HandleHelp(args); },
        "Show help information");
    RegisterCommand(
        "exit",
        [this](const std::vector<std::string>& args)
        {
            bool result = commandHandler_->HandleExit(args);
            if (result) running_ = false;
            return result;
        },
        "Exit the CLI");
    RegisterCommand(
        "clear", [this](const std::vector<std::string>& args) { return commandHandler_->HandleClear(args); },
        "Clear the screen");
    RegisterCommand(
        "version", [this](const std::vector<std::string>& args) { return commandHandler_->HandleVersion(args); },
        "Show version information");
    RegisterCommand(
        "show state", [this](const std::vector<std::string>& args) { return commandHandler_->HandleShowState(args); },
        "Show the current state of the blockchain");
    RegisterCommand(
        "show node", [this](const std::vector<std::string>& args) { return commandHandler_->HandleShowNode(args); },
        "Show the current node information");
    RegisterCommand(
        "show pool", [this](const std::vector<std::string>& args) { return commandHandler_->HandleShowPool(args); },
        "Show the memory pool information");
    RegisterCommand(
        "open wallet", [this](const std::vector<std::string>& args) { return commandHandler_->HandleOpenWallet(args); },
        "Open a wallet");
    RegisterCommand(
        "close wallet", [this](const std::vector<std::string>& args)
        { return commandHandler_->HandleCloseWallet(args); }, "Close the current wallet");
    RegisterCommand(
        "create wallet", [this](const std::vector<std::string>& args)
        { return commandHandler_->HandleCreateWallet(args); }, "Create a new wallet");
    RegisterCommand(
        "import key", [this](const std::vector<std::string>& args) { return commandHandler_->HandleImportKey(args); },
        "Import a private key");
    RegisterCommand(
        "import nep2", [this](const std::vector<std::string>& args) { return commandHandler_->HandleImportNEP2(args); },
        "Import a NEP2 key");
    RegisterCommand(
        "export key", [this](const std::vector<std::string>& args) { return commandHandler_->HandleExportKey(args); },
        "Export a private key");
    RegisterCommand(
        "list address", [this](const std::vector<std::string>& args)
        { return commandHandler_->HandleListAddress(args); }, "List all addresses in the wallet");
    RegisterCommand(
        "list asset", [this](const std::vector<std::string>& args) { return commandHandler_->HandleListAsset(args); },
        "List all assets in the wallet");
    RegisterCommand(
        "transfer", [this](const std::vector<std::string>& args) { return commandHandler_->HandleTransfer(args); },
        "Transfer assets");
    RegisterCommand(
        "claim gas", [this](const std::vector<std::string>& args) { return commandHandler_->HandleClaimGas(args); },
        "Claim GAS");
    RegisterCommand(
        "send", [this](const std::vector<std::string>& args) { return commandHandler_->HandleSend(args); },
        "Send a transaction");
    RegisterCommand(
        "deploy", [this](const std::vector<std::string>& args) { return commandHandler_->HandleDeploy(args); },
        "Deploy a contract");
    RegisterCommand(
        "invoke", [this](const std::vector<std::string>& args) { return commandHandler_->HandleInvoke(args); },
        "Invoke a contract");
}

std::pair<std::string, std::vector<std::string>> CLI::ParseCommand(const std::string& command)
{
    std::vector<std::string> tokens;
    std::string token;
    bool inQuotes = false;

    for (char c : command)
    {
        if (c == '"')
        {
            inQuotes = !inQuotes;
        }
        else if (c == ' ' && !inQuotes)
        {
            if (!token.empty())
            {
                tokens.push_back(token);
                token.clear();
            }
        }
        else
        {
            token += c;
        }
    }

    if (!token.empty()) tokens.push_back(token);

    if (tokens.empty()) return {"", {}};

    std::string cmd = tokens[0];
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());

    // Check for multi-word commands
    for (size_t i = 1; i < tokens.size(); i++)
    {
        std::string multiCmd = cmd + " " + tokens[i];
        if (commands_.find(multiCmd) != commands_.end())
        {
            cmd = multiCmd;
            args = std::vector<std::string>(tokens.begin() + i + 1, tokens.end());
        }
    }

    return {cmd, args};
}

const std::unordered_map<std::string, std::string>& CLI::GetCommandHelp() const { return commandHelp_; }
}  // namespace neo::cli
