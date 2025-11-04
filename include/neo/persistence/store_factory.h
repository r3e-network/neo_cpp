/**
 * @file store_factory.h
 * @brief Factory pattern implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/persistence/file_store.h>
#include <neo/persistence/istore.h>
#include <neo/persistence/memory_store.h>
#ifdef NEO_HAS_ROCKSDB
#include <neo/persistence/rocksdb_store.h>
#endif
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace neo::persistence
{
/**
 * @brief Factory class for creating store providers and stores.
 *
 * The StoreFactory provides a centralized way to create different types
 * of storage providers (memory, RocksDB, etc.) and manages their lifecycle.
 */
class StoreFactory
{
   public:
    /**
     * @brief Gets a store provider by name.
     * @param provider_name The name of the provider ("memory", "rocksdb", etc.)
     * @return Shared pointer to the store provider, or nullptr if not found
     */
    static std::shared_ptr<IStoreProvider> get_store_provider(const std::string& provider_name);

    /**
     * @brief Gets a store provider by name with configuration.
     * @param provider_name The name of the provider ("memory", "rocksdb", etc.)
     * @param config Configuration parameters for the provider
     * @return Shared pointer to the store provider, or nullptr if not found
     */
    static std::shared_ptr<IStoreProvider> get_store_provider(
        const std::string& provider_name, const std::unordered_map<std::string, std::string>& config);

    /**
     * @brief Gets the default store provider.
     * @return Shared pointer to the default store provider (memory)
     */
    static std::shared_ptr<IStoreProvider> get_default_store_provider();

    /**
     * @brief Registers a custom store provider.
     * @param name The name of the provider
     * @param provider The store provider instance
     */
    static void register_store_provider(const std::string& name, std::shared_ptr<IStoreProvider> provider);

    /**
     * @brief Gets a list of available store provider names.
     * @return Vector of provider names
     */
    static std::vector<std::string> get_available_providers();

    /**
     * @brief Creates a store directly without going through a provider.
     * @param provider_name The type of store to create
     * @param path The path for the store
     * @return Unique pointer to the created store
     */
    static std::unique_ptr<IStore> create_store(const std::string& provider_name, const std::string& path);

   private:
    static std::unordered_map<std::string, std::shared_ptr<IStoreProvider>> providers_;
    static std::mutex providers_mutex_;
    static bool initialized_;

    /**
     * @brief Initializes the factory with default providers.
     */
    static void initialize();

    /**
     * @brief Ensures the factory is initialized.
     */
    static void ensure_initialized();
};
}  // namespace neo::persistence
