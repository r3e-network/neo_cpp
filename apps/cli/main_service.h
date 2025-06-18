#pragma once

#include <neo/node/neo_system.h>
#include <neo/rpc/rpc_server.h>
#include <neo/wallets/wallet.h>
#include <neo/cli/console_helper.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>

namespace neo::cli
{
    struct CommandLineOptions
    {
        std::string Config;
        std::string Wallet;
        std::string Password;
        std::string DbEngine;
        std::string DbPath;
        bool NoVerify = false;
        std::vector<std::string> Plugins;
        int Verbose = 0;
    };

    using CommandHandler = std::function<bool(const std::vector<std::string>&)>;
    using TypeConverter = std::function<void*(const std::vector<std::string>&, bool)>;

    class MainService
    {
    public:
        MainService();
        ~MainService();

        // Main entry points
        void Run(const std::vector<std::string>& args);
        void Start(const CommandLineOptions& options);
        void Stop();

        // Command registration
        void RegisterCommand(const std::string& name, const CommandHandler& handler, const std::string& category = "");
        void RegisterTypeConverter(const std::string& typeName, const TypeConverter& converter);

        // Accessors
        std::shared_ptr<node::NeoSystem> GetNeoSystem() const;
        std::shared_ptr<wallets::Wallet> GetCurrentWallet() const;
        bool HasWallet() const { return currentWallet_ != nullptr; }

        // Command handlers
        void OnCommand(const std::string& command);
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
        void OnShowPool();
        void OnShowPeers();

        // Wallet Commands
        void OnOpenWallet(const std::string& path, const std::string& password);
        void OnCloseWallet();
        void OnShowBalance();
        void OnShowBalance(const io::UInt160& assetId);
        void OnShowAddress();
        void OnTransfer(const io::UInt160& assetId, const std::string& address, double amount);

    private:
        std::shared_ptr<node::NeoSystem> neoSystem_;
        std::shared_ptr<rpc::RPCServer> rpcServer_;
        std::shared_ptr<wallets::Wallet> currentWallet_;
        std::atomic<bool> running_;
        std::thread consoleThread_;

        std::unordered_map<std::string, CommandHandler> commands_;
        std::unordered_map<std::string, std::unordered_map<std::string, CommandHandler>> commandsByCategory_;
        std::unordered_map<std::string, TypeConverter> typeConverters_;

        void InitializeCommands();
        void InitializeBlockchainCommands();
        void InitializeNodeCommands();
        void InitializeWalletCommands();
        void InitializeTypeConverters();
        void RunConsole();
    };
}
