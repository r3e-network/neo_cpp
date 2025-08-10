#pragma once

#include <neo/consensus/consensus_service.h>
#include <neo/plugins/plugin_base.h>
#include <neo/wallets/wallet.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace neo::plugins
{
/**
 * @brief Represents a DBFT plugin.
 */
class DBFTPlugin : public PluginBase
{
   public:
    /**
     * @brief Constructs a DBFTPlugin.
     */
    DBFTPlugin();

    /**
     * @brief Starts the consensus service.
     * @param wallet The wallet.
     * @return True if the consensus service was started, false otherwise.
     */
    bool StartConsensus(std::shared_ptr<wallets::Wallet> wallet);

    /**
     * @brief Stops the consensus service.
     * @return True if the consensus service was stopped, false otherwise.
     */
    bool StopConsensus();

    /**
     * @brief Checks if the consensus service is running.
     * @return True if the consensus service is running, false otherwise.
     */
    bool IsConsensusRunning() const;

   protected:
    /**
     * @brief Initializes the plugin.
     * @param settings The settings.
     * @return True if the plugin was initialized, false otherwise.
     */
    bool OnInitialize(const std::unordered_map<std::string, std::string>& settings) override;

    /**
     * @brief Starts the plugin.
     * @return True if the plugin was started, false otherwise.
     */
    bool OnStart() override;

    /**
     * @brief Stops the plugin.
     * @return True if the plugin was stopped, false otherwise.
     */
    bool OnStop() override;

   private:
    std::shared_ptr<consensus::ConsensusService> consensusService_;
    std::string walletPath_;
    std::string walletPassword_;
    bool autoStart_;
    mutable std::mutex mutex_;
};

/**
 * @brief Represents a DBFT plugin factory.
 */
class DBFTPluginFactory : public PluginFactoryBase<DBFTPlugin>
{
};
}  // namespace neo::plugins
