/**
 * @file state_service_plugin.h
 * @brief Service implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/plugins/plugin_base.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo::plugins
{
/**
 * @brief Represents a state root.
 */
struct StateRoot
{
    /**
     * @brief The block index.
     */
    uint32_t Index;

    /**
     * @brief The block hash.
     */
    io::UInt256 BlockHash;

    /**
     * @brief The state root.
     */
    io::UInt256 Root;

    /**
     * @brief The version.
     */
    uint8_t Version;
};

/**
 * @brief Represents a state service plugin.
 */
class StateServicePlugin : public PluginBase
{
   public:
    /**
     * @brief Constructs a StateServicePlugin.
     */
    StateServicePlugin();

    /**
     * @brief Gets the state root for a block index.
     * @param index The block index.
     * @return The state root, or nullptr if not found.
     */
    std::shared_ptr<StateRoot> GetStateRoot(uint32_t index) const;

    /**
     * @brief Gets the state root for a block hash.
     * @param hash The block hash.
     * @return The state root, or nullptr if not found.
     */
    std::shared_ptr<StateRoot> GetStateRoot(const io::UInt256& hash) const;

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
    std::string statePath_;
    std::unordered_map<uint32_t, std::shared_ptr<StateRoot>> stateRoots_;
    std::unordered_map<io::UInt256, std::shared_ptr<StateRoot>> stateRootsByHash_;
    mutable std::mutex mutex_;

    void OnBlockPersisted(std::shared_ptr<ledger::Block> block);
    void SaveStateRoots();
    void LoadStateRoots();
    io::UInt256 CalculateStateRoot(
        uint32_t index, const std::vector<std::pair<persistence::StorageKey, persistence::StorageItem>>& changes);
};

/**
 * @brief Represents a state service plugin factory.
 */
class StateServicePluginFactory : public PluginFactoryBase<StateServicePlugin>
{
};
}  // namespace neo::plugins
