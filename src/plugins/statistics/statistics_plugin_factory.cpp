#include <neo/plugins/statistics/statistics_plugin_factory.h>
#include <neo/plugins/statistics/statistics_plugin.h>
#include <memory>

namespace neo::plugins::statistics
{
    std::shared_ptr<Plugin> StatisticsPluginFactory::CreatePlugin()
    {
        return std::make_shared<StatisticsPlugin>();
    }
}
