/**
 * @file store_factory_complete.cpp
 * @brief Factory pattern implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/logging.h>
#include <neo/persistence/file_store.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/store_factory.h>

#ifdef NEO_HAS_ROCKSDB
#include <neo/persistence/rocksdb_store.h>
#endif

namespace neo::persistence
{
#ifdef NEO_HAS_ROCKSDB
// Factory function declarations
std::shared_ptr<IStoreProvider> CreateRocksDbStoreProvider(const RocksDbConfig& config = RocksDbConfig{});
#endif

std::shared_ptr<IStoreProvider> StoreFactory::get_store_provider(const std::string& engine)
{
    LOG_INFO("Creating store provider for engine: {}", engine);

    if (engine == "memory" || engine.empty())
    {
        return std::make_shared<MemoryStoreProvider>();
    }
    else if (engine == "file")
    {
        return std::make_shared<FileStoreProvider>();
    }
#ifdef NEO_HAS_ROCKSDB
    else if (engine == "rocksdb")
    {
        LOG_INFO("Using RocksDB storage provider");
        return CreateRocksDbStoreProvider();
    }
    else if (engine == "leveldb")
    {
        LOG_WARNING("LevelDB provider unavailable, using RocksDB instead");
        return CreateRocksDbStoreProvider();
    }
#else
    else if (engine == "rocksdb" || engine == "leveldb")
    {
        LOG_WARNING("RocksDB/LevelDB not available, falling back to memory store");
        return std::make_shared<MemoryStoreProvider>();
    }
#endif

    LOG_WARNING("Unknown storage engine '{}', defaulting to memory", engine);
    return std::make_shared<MemoryStoreProvider>();
}

std::shared_ptr<IStoreProvider> StoreFactory::get_store_provider(
    const std::string& engine, const std::unordered_map<std::string, std::string>& config)
{
    LOG_INFO("Creating store provider for engine: {} with config", engine);

    if (engine == "memory" || engine.empty())
    {
        return std::make_shared<MemoryStoreProvider>();
    }
    else if (engine == "file")
    {
        auto it = config.find("db_path");
        std::string base = (it != config.end()) ? it->second : "./data/file-store";
        return std::make_shared<FileStoreProvider>(base);
    }
#ifdef NEO_HAS_ROCKSDB
    else if (engine == "rocksdb")
    {
        // Configure RocksDB based on provided configuration
        RocksDbConfig rocksdb_config;

        auto it = config.find("db_path");
        if (it != config.end())
        {
            rocksdb_config.db_path = it->second;
        }

        it = config.find("write_buffer_size");
        if (it != config.end())
        {
            try
            {
                rocksdb_config.write_buffer_size = std::stoull(it->second);
            }
            catch (...)
            {
                LOG_WARNING("Invalid write_buffer_size value: {}", it->second);
            }
        }

        it = config.find("block_cache_size");
        if (it != config.end())
        {
            try
            {
                rocksdb_config.block_cache_size = std::stoull(it->second);
            }
            catch (...)
            {
                LOG_WARNING("Invalid block_cache_size value: {}", it->second);
            }
        }

        it = config.find("compression_enabled");
        if (it != config.end())
        {
            rocksdb_config.compression_enabled = (it->second == "true" || it->second == "1");
        }

        it = config.find("use_bloom_filter");
        if (it != config.end())
        {
            rocksdb_config.use_bloom_filter = (it->second == "true" || it->second == "1");
        }

        LOG_INFO("Using RocksDB storage provider with custom config");
        return CreateRocksDbStoreProvider(rocksdb_config);
    }
#else
    else if (engine == "rocksdb")
    {
        LOG_WARNING("RocksDB not available, falling back to memory store");
        return std::make_shared<MemoryStoreProvider>();
    }
#endif

    LOG_WARNING("Unknown storage engine '{}', defaulting to memory", engine);
    return std::make_shared<MemoryStoreProvider>();
}
}  // namespace neo::persistence
