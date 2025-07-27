#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <neo/io/json.h>
#include <neo/node/node.h>
#include <neo/plugins/plugin.h>
#include <neo/rpc/rpc_server.h>
#include <string>
#include <thread>
#include <unordered_map>

namespace neo::plugins::statistics
{
/**
 * @brief Statistics plugin.
 */
class StatisticsPlugin : public Plugin
{
  public:
    /**
     * @brief Constructs a StatisticsPlugin.
     */
    StatisticsPlugin();

    /**
     * @brief Destructor.
     */
    ~StatisticsPlugin() override;

    /**
     * @brief Gets the description of the plugin.
     * @return The description of the plugin.
     */
    std::string GetDescription() const override;

    /**
     * @brief Gets the version of the plugin.
     * @return The version of the plugin.
     */
    std::string GetVersion() const override;

    /**
     * @brief Gets the author of the plugin.
     * @return The author of the plugin.
     */
    std::string GetAuthor() const override;

    /**
     * @brief Initializes the plugin.
     * @param node The node.
     * @param rpcServer The RPC server.
     * @param settings The settings.
     * @return True if the plugin was initialized, false otherwise.
     */
    bool Initialize(std::shared_ptr<node::Node> node, std::shared_ptr<rpc::RPCServer> rpcServer,
                    const std::unordered_map<std::string, std::string>& settings) override;

    /**
     * @brief Starts the plugin.
     * @return True if the plugin was started, false otherwise.
     */
    bool Start() override;

    /**
     * @brief Stops the plugin.
     * @return True if the plugin was stopped, false otherwise.
     */
    bool Stop() override;

    /**
     * @brief Checks if the plugin is running.
     * @return True if the plugin is running, false otherwise.
     */
    bool IsRunning() const override;

  private:
    std::shared_ptr<node::Node> node_;
    std::shared_ptr<rpc::RPCServer> rpcServer_;
    std::atomic<bool> running_;
    std::thread statisticsThread_;
    std::mutex mutex_;

    // Statistics
    std::atomic<uint32_t> blockCount_;
    std::atomic<uint32_t> transactionCount_;
    std::atomic<uint32_t> peerCount_;
    std::atomic<uint32_t> memoryPoolSize_;

    // Settings
    std::chrono::seconds interval_;
    bool enableRPC_;

    /**
     * @brief Collects statistics.
     */
    void CollectStatistics();

    /**
     * @brief Handles the get statistics RPC method.
     * @param params The parameters.
     * @return The result.
     */
    nlohmann::json HandleGetStatistics(const nlohmann::json& params);
};
}  // namespace neo::plugins::statistics
