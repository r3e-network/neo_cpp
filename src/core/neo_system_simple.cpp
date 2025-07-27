#include "neo/core/logging.h"
#include "neo/core/neo_system.h"
#include "neo/ledger/blockchain.h"
#include "neo/ledger/memory_pool.h"
#include "neo/persistence/memory_store.h"
#include "neo/persistence/store_factory.h"
#include "neo/protocol_settings.h"

namespace neo
{

// Simple NeoSystem that doesn't use threads or shared_from_this
class SimpleNeoSystem
{
  private:
    std::unique_ptr<ProtocolSettings> settings_;
    std::shared_ptr<persistence::IStoreProvider> storage_provider_;
    std::shared_ptr<persistence::IStore> store_;

  public:
    SimpleNeoSystem(std::unique_ptr<ProtocolSettings> settings, const std::string& storage_provider_name,
                    const std::string& storage_path)
        : settings_(std::move(settings))
    {
        LOG_INFO("Initializing SimpleNeoSystem...");

        if (!settings_)
        {
            throw std::invalid_argument("Settings cannot be null");
        }

        // Create storage provider
        storage_provider_ = persistence::StoreFactory::get_store_provider(storage_provider_name);
        if (!storage_provider_)
        {
            throw std::invalid_argument("Storage provider cannot be null");
        }

        // Get store
        store_ = storage_provider_->GetStore(storage_path);

        LOG_INFO("SimpleNeoSystem initialized successfully");
    }

    ~SimpleNeoSystem()
    {
        LOG_INFO("Shutting down SimpleNeoSystem");
    }

    ProtocolSettings* GetSettings() const
    {
        return settings_.get();
    }
    persistence::IStore* GetStore() const
    {
        return store_.get();
    }
};

// Factory function to create a simple NeoSystem without threading issues
std::unique_ptr<SimpleNeoSystem> CreateSimpleNeoSystem(std::unique_ptr<ProtocolSettings> settings,
                                                       const std::string& storage_provider_name,
                                                       const std::string& storage_path)
{
    return std::make_unique<SimpleNeoSystem>(std::move(settings), storage_provider_name, storage_path);
}

}  // namespace neo