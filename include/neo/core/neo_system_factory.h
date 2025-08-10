#pragma once

#include <neo/core/neo_system.h>
#include <neo/persistence/istore.h>

#include <memory>
#include <string>

namespace neo
{
class ProtocolSettings;

/**
 * @brief Factory class for creating NeoSystem instances with proper shared_ptr management
 */
class NeoSystemFactory
{
   public:
    /**
     * @brief Creates a NeoSystem instance with the specified settings and storage provider
     * @param settings The protocol settings for this Neo system
     * @param storage_provider The storage provider to use
     * @param storage_path The path for persistent storage
     * @return Shared pointer to the created NeoSystem
     */
    static std::shared_ptr<NeoSystem> Create(std::unique_ptr<ProtocolSettings> settings,
                                             std::shared_ptr<persistence::IStoreProvider> storage_provider,
                                             const std::string& storage_path = "");

    /**
     * @brief Creates a NeoSystem instance with the specified settings and storage provider name
     * @param settings The protocol settings for this Neo system
     * @param storage_provider_name The name of the storage provider to use (default: "memory")
     * @param storage_path The path for persistent storage (ignored for memory provider)
     * @return Shared pointer to the created NeoSystem
     */
    static std::shared_ptr<NeoSystem> Create(std::unique_ptr<ProtocolSettings> settings,
                                             const std::string& storage_provider_name = "memory",
                                             const std::string& storage_path = "");

   private:
    // Factory class should not be instantiated
    NeoSystemFactory() = delete;
};

}  // namespace neo