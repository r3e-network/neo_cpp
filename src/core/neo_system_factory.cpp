/**
 * @file neo_system_factory.cpp
 * @brief Main Neo system coordinator
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/logging.h>
#include <neo/core/neo_system.h>
#include <neo/core/neo_system_factory.h>
#include <neo/ledger/blockchain.h>
#include <neo/persistence/store_factory.h>
#include <neo/protocol_settings.h>

namespace neo
{

// Private implementation class to handle two-phase initialization
class NeoSystemInit
{
   public:
    static void CompleteInitialization(std::shared_ptr<NeoSystem> neo_system)
    {
        // Now it's safe to use shared_from_this() because the object is owned by a shared_ptr
        try
        {
            // The NeoSystem is now fully constructed and in a shared_ptr
            // We can safely initialize components that need shared_from_this()

            // Initialize blockchain
            neo_system->blockchain_ = std::make_unique<ledger::Blockchain>(neo_system);
            if (neo_system->blockchain_)
            {
                neo_system->blockchain_->Initialize();
            }

            // Load plugins
            neo_system->load_plugins();

            LOG_INFO("NeoSystem fully initialized with shared_ptr and blockchain");
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to complete NeoSystem initialization: {}", e.what());
            throw;
        }
    }
};

std::shared_ptr<NeoSystem> NeoSystemFactory::Create(std::unique_ptr<ProtocolSettings> settings,
                                                    std::shared_ptr<persistence::IStoreProvider> storage_provider,
                                                    const std::string& storage_path)
{
    // Create the NeoSystem instance
    auto neo_system = std::make_shared<NeoSystem>(std::move(settings), storage_provider, storage_path);

    // Now that it's in a shared_ptr, complete initialization
    NeoSystemInit::CompleteInitialization(neo_system);

    return neo_system;
}

std::shared_ptr<NeoSystem> NeoSystemFactory::Create(std::unique_ptr<ProtocolSettings> settings,
                                                    const std::string& storage_provider_name,
                                                    const std::string& storage_path)
{
    auto storage_provider = persistence::StoreFactory::get_store_provider(storage_provider_name);
    return Create(std::move(settings), storage_provider, storage_path);
}

}  // namespace neo