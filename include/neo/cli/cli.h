/**
 * @file cli.h
 * @brief Cli
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/cli/command_handler.h>
#include <neo/node/neo_system.h>
#include <neo/rpc/rpc_server.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace neo::cli
{
/**
 * @brief Represents a CLI (Command Line Interface).
 */
class CLI
{
   public:
    /**
     * @brief Constructs a CLI.
     * @param neoSystem The Neo system.
     * @param rpcServer The RPC server.
     */
    CLI(std::shared_ptr<node::NeoSystem> neoSystem, std::shared_ptr<rpc::RpcServer> rpcServer);

    /**
     * @brief Destructor.
     */
    ~CLI();

    /**
     * @brief Starts the CLI.
     */
    void Start();

    /**
     * @brief Stops the CLI.
     */
    void Stop();

    /**
     * @brief Checks if the CLI is running.
     * @return True if the CLI is running, false otherwise.
     */
    bool IsRunning() const;

    /**
     * @brief Gets the Neo system.
     * @return The Neo system.
     */
    std::shared_ptr<node::NeoSystem> GetNeoSystem() const;

    /**
     * @brief Gets the RPC server.
     * @return The RPC server.
     */
    std::shared_ptr<rpc::RpcServer> GetRPCServer() const;

    /**
     * @brief Gets the wallet.
     * @return The wallet.
     */
    std::shared_ptr<wallets::Wallet> GetWallet() const;

    /**
     * @brief Sets the wallet.
     * @param wallet The wallet.
     */
    void SetWallet(std::shared_ptr<wallets::Wallet> wallet);

    /**
     * @brief Registers a command.
     * @param command The command.
     * @param handler The handler.
     * @param help The help text.
     */
    void RegisterCommand(const std::string& command, std::function<bool(const std::vector<std::string>&)> handler,
                         const std::string& help);

    /**
     * @brief Unregisters a command.
     * @param command The command.
     */
    void UnregisterCommand(const std::string& command);

    /**
     * @brief Executes a command.
     * @param command The command.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool ExecuteCommand(const std::string& command);

    /**
     * @brief Gets the command help.
     * @return The command help.
     */
    const std::unordered_map<std::string, std::string>& GetCommandHelp() const;

   private:
    std::shared_ptr<node::NeoSystem> neoSystem_;
    std::shared_ptr<rpc::RpcServer> rpcServer_;
    std::shared_ptr<CommandHandler> commandHandler_;
    std::atomic<bool> running_;
    std::thread cliThread_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::unordered_map<std::string, std::function<bool(const std::vector<std::string>&)>> commands_;
    std::unordered_map<std::string, std::string> commandHelp_;

    /**
     * @brief Runs the CLI.
     */
    void RunCLI();

    /**
     * @brief Initializes the commands.
     */
    void InitializeCommands();

    /**
     * @brief Parses a command.
     * @param command The command.
     * @return The command and arguments.
     */
    std::pair<std::string, std::vector<std::string>> ParseCommand(const std::string& command);
};
}  // namespace neo::cli
