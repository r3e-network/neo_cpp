#pragma once

#include <neo/plugins/plugin.h>

#include <memory>

namespace neo::plugins::statistics
{
/**
 * @brief Factory for creating statistics plugins.
 */
class StatisticsPluginFactory : public PluginFactory
{
   public:
    /**
     * @brief Creates a plugin.
     * @return The plugin.
     */
    std::shared_ptr<Plugin> CreatePlugin() override;
};
}  // namespace neo::plugins::statistics
