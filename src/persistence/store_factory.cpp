#include "neo/persistence/store_factory.h"
#include <stdexcept>

namespace neo::persistence
{
    std::unordered_map<std::string, std::shared_ptr<IStoreProvider>> StoreFactory::providers_;
    std::mutex StoreFactory::providers_mutex_;
    bool StoreFactory::initialized_ = false;

    std::shared_ptr<IStoreProvider> StoreFactory::get_store_provider(const std::string& provider_name)
    {
        ensure_initialized();
        
        std::lock_guard<std::mutex> lock(providers_mutex_);
        auto it = providers_.find(provider_name);
        if (it != providers_.end())
        {
            return it->second;
        }
        
        return nullptr;
    }

    std::shared_ptr<IStoreProvider> StoreFactory::get_default_store_provider()
    {
        return get_store_provider("memory");
    }

    void StoreFactory::register_store_provider(const std::string& name, std::shared_ptr<IStoreProvider> provider)
    {
        if (!provider)
        {
            throw std::invalid_argument("Store provider cannot be null");
        }
        
        std::lock_guard<std::mutex> lock(providers_mutex_);
        providers_[name] = provider;
    }

    std::vector<std::string> StoreFactory::get_available_providers()
    {
        ensure_initialized();
        
        std::lock_guard<std::mutex> lock(providers_mutex_);
        std::vector<std::string> names;
        names.reserve(providers_.size());
        
        for (const auto& pair : providers_)
        {
            names.push_back(pair.first);
        }
        
        return names;
    }

    std::unique_ptr<IStore> StoreFactory::create_store(const std::string& provider_name, const std::string& path)
    {
        auto provider = get_store_provider(provider_name);
        if (!provider)
        {
            throw std::runtime_error("Store provider not found: " + provider_name);
        }
        
        return provider->GetStore(path);
    }

    void StoreFactory::initialize()
    {
        if (initialized_)
            return;
            
        // Register memory store provider
        register_store_provider("memory", std::make_shared<MemoryStoreProvider>());
        
// TODO: Implement RocksDB provider when NEO_HAS_ROCKSDB is defined
// #ifdef NEO_HAS_ROCKSDB
//         // Register RocksDB store provider if available
//         register_store_provider("rocksdb", std::make_shared<RocksDBStoreProvider>());
// #endif
        
        initialized_ = true;
    }

    void StoreFactory::ensure_initialized()
    {
        if (!initialized_)
        {
            std::lock_guard<std::mutex> lock(providers_mutex_);
            if (!initialized_)
            {
                initialize();
            }
        }
    }
}