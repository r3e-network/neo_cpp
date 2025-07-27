#include <neo/core/logging.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/store_factory.h>

namespace neo::persistence
{
std::shared_ptr<IStoreProvider> StoreFactory::get_store_provider(const std::string& engine)
{
    LOG_INFO("Creating store provider for engine: {}", engine);

    if (engine == "memory" || engine.empty())
    {
        return std::make_shared<MemoryStoreProvider>();
    }

    // Add more store providers here (e.g., LevelDB, RocksDB)

    LOG_WARNING("Unknown storage engine '{}', defaulting to memory", engine);
    return std::make_shared<MemoryStoreProvider>();
}
}  // namespace neo::persistence