#include <neo/plugins/plugin.h>
#include <neo/plugins/rpc_server_plugin.h>
#include <neo/plugins/application_logs_plugin.h>
#include <neo/plugins/state_service_plugin.h>
#include <neo/plugins/dbft_plugin.h>
#include <neo/plugins/test_plugin.h>

namespace neo::plugins
{
    // Register plugins
    static struct PluginRegistration
    {
        PluginRegistration()
        {
            // Get plugin manager
            auto& manager = PluginManager::GetInstance();

            // Register RPC server plugin
            manager.RegisterPluginFactory(std::make_shared<RpcServerPluginFactory>());

            // Register application logs plugin
            manager.RegisterPluginFactory(std::make_shared<ApplicationLogsPluginFactory>());

            // Register state service plugin
            manager.RegisterPluginFactory(std::make_shared<StateServicePluginFactory>());

            // Register DBFT plugin
            manager.RegisterPluginFactory(std::make_shared<DBFTPluginFactory>());

            // Register test plugin
            manager.RegisterPluginFactory(std::make_shared<TestPluginFactory>());
        }
    } pluginRegistration;
}
