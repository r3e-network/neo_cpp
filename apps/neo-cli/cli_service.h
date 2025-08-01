#pragma once

#include <atomic>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

namespace neo
{
// Forward declarations
namespace core
{
class NeoSystem;
}
namespace persistence
{
class RocksDBStore;
}
namespace ledger
{
class Blockchain;
class MemoryPool;
}
namespace network
{
class P2PServer;
}
namespace rpc
{
class RpcServer;
}
namespace consensus
{
class DbftConsensus;
}
namespace wallets
{
class NEP6Wallet;
}
}

namespace neo::cli
{

class CommandRegistry;
class PluginManager;
class ConsoleServiceNeo;

/**
 * @brief Main CLI service for Neo node
 *
 * This class manages the complete Neo node including blockchain,
 * network, RPC, consensus, and CLI interface.
 */
class CLIService
{
  public:
    CLIService(const std::filesystem::path& config_path, const std::string& network);
    ~CLIService();

    // Configuration
    void SetRPCEnabled(bool enabled)
    {
        rpc_enabled_ = enabled;
    }
    void SetConsensusEnabled(bool enabled)
    {
        consensus_enabled_ = enabled;
    }

    // Lifecycle
    void Initialize();
    void Start();
    void Run();
    void Stop();

    // Display
    void DisplayBanner();
    void DisplayStatus();
    void DisplayHelp();

    // Node access
    core::NeoSystem* GetNeoSystem()
    {
        return neo_system_.get();
    }
    ledger::Blockchain* GetBlockchain();
    ledger::MemoryPool* GetMemoryPool();
    network::P2PServer* GetP2PServer()
    {
        return p2p_server_.get();
    }
    rpc::RpcServer* GetRpcServer()
    {
        return rpc_server_.get();
    }

    // Wallet management
    bool OpenWallet(const std::filesystem::path& path, const std::string& password);
    void CloseWallet();
    wallets::NEP6Wallet* GetCurrentWallet()
    {
        return current_wallet_.get();
    }

    // Plugin system
    void LoadPlugins();
    PluginManager* GetPluginManager()
    {
        return plugin_manager_.get();
    }

  private:
    // Configuration
    std::filesystem::path config_path_;
    std::string network_;
    nlohmann::json config_;
    bool rpc_enabled_ = true;
    bool consensus_enabled_ = false;

    // Core components
    std::unique_ptr<core::NeoSystem> neo_system_;
    std::unique_ptr<persistence::RocksDBStore> store_;
    std::unique_ptr<network::P2PServer> p2p_server_;
    std::unique_ptr<rpc::RpcServer> rpc_server_;
    std::unique_ptr<consensus::DbftConsensus> consensus_;

    // CLI components
    std::unique_ptr<CommandRegistry> command_registry_;
    std::unique_ptr<PluginManager> plugin_manager_;
    std::unique_ptr<ConsoleServiceNeo> console_service_;

    // Wallet
    std::unique_ptr<wallets::NEP6Wallet> current_wallet_;

    // State
    std::atomic<bool> running_{false};
    std::thread status_thread_;

    // Private methods
    void LoadConfiguration();
    void SetupShutdownHandlers();
    void InitializeLogging();
    void InitializeMetrics();
    void InitializeHealthChecks();
    void InitializeStorage();
    void InitializeNeoSystem();
    void InitializeNetwork();
    void InitializeRPC();
    void InitializeConsensus();
    void InitializeConsole();
    void RegisterCommands();
    void StartMonitoring();
    void StatusLoop();

    // Production components
    std::unique_ptr<rpc::MethodRateLimiter> rateLimiter_;
    std::unique_ptr<network::ConnectionLimits> connectionLimits_;
    std::unique_ptr<network::TimeoutManager> timeoutManager_;
};