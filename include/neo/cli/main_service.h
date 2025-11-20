/**
 * @file main_service.h
 * @brief Service implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/cli/command_line_options.h>
#include <neo/cli/console_command_attribute.h>
#include <neo/cli/console_helper.h>
#include <neo/cli/type_converters.h>
#include <neo/node/neo_system.h>
#include <neo/rpc/rpc_server.h>
#include <neo/wallets/wallet.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace neo::cli
{
/**
 * @brief Type converter function.
 */
using TypeConverter = std::function<void*(const std::vector<std::string>&, bool)>;

/**
 * @brief Command handler function.
 */
using CommandHandler = std::function<bool(const std::vector<std::string>&)>;

/**
 * @brief Main service for the CLI.
 */
class MainService
{
   public:
    /**
     * @brief Constructs a MainService.
     */
    MainService();

    /**
     * @brief Destructor.
     */
    ~MainService();

    /**
     * @brief Runs the service with the specified arguments.
     * @param args The arguments.
     */
    void Run(const std::vector<std::string>& args);

    /**
     * @brief Starts the service with the specified options.
     * @param options The options.
     */
    void Start(const CommandLineOptions& options);

    /**
     * @brief Stops the service.
     */
    void Stop();

    /**
     * @brief Registers a command.
     * @param name The command name.
     * @param handler The command handler.
     * @param category The command category.
     */
    void RegisterCommand(const std::string& name, const CommandHandler& handler, const std::string& category = "");

    /**
     * @brief Registers a type converter.
     * @param typeName The type name.
     * @param converter The converter.
     */
    void RegisterTypeConverter(const std::string& typeName, const TypeConverter& converter);

    /**
     * @brief Gets the Neo system.
     * @return The Neo system.
     */
    std::shared_ptr<node::NeoSystem> GetNeoSystem() const;

    /**
     * @brief Gets the current wallet.
     * @return The current wallet.
     */
    std::shared_ptr<wallets::Wallet> GetCurrentWallet() const;

    /**
     * @brief Handles a command.
     * @param command The command.
     */
    void OnCommand(const std::string& command);

   private:
    std::shared_ptr<node::NeoSystem> neoSystem_;
    std::shared_ptr<wallets::Wallet> currentWallet_;
    std::shared_ptr<rpc::RpcServer> rpcServer_;
    std::unordered_map<std::string, CommandHandler> commands_;
    std::unordered_map<std::string, std::unordered_map<std::string, CommandHandler>> commandsByCategory_;
    std::unordered_map<std::string, TypeConverter> typeConverters_;
    std::atomic<bool> running_;
    std::thread consoleThread_;
    std::mutex mutex_;

    void OnStartWithCommandLine(const std::vector<std::string>& args);
    void OnHelp(const std::string& category = "");
    void OnExit();
    void OnClear();
    void OnVersion();

    // Blockchain Commands
    void OnShowBlock(const std::string& indexOrHash);
    void OnShowHeader(const std::string& indexOrHash);
    void OnShowTransaction(const io::UInt256& hash);

    // Node Commands
    void OnShowState();
    void OnShowPool(bool verbose = false);
    void OnShowPeers();

    // Wallet Commands
    void OnOpenWallet(const std::string& path, const std::string& password);
    void OnCloseWallet();
    void OnShowBalance();

    void RunConsole();
    void InitializeCommands();
    void InitializeTypeConverters();
    void InitializeBlockchainCommands();
    void InitializeNodeCommands();
    void InitializeWalletCommands();
};
}  // namespace neo::cli
