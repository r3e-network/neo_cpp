#pragma once

#include <neo/plugins/plugin_base.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace neo::plugins
{
/**
 * @brief Represents a test plugin.
 */
class TestPlugin : public PluginBase
{
   public:
    /**
     * @brief Constructs a TestPlugin.
     */
    TestPlugin();

    /**
     * @brief Logs a message.
     * @param message The message.
     */
    void LogMessage(const std::string& message);

    /**
     * @brief Tests the OnMessage method.
     * @param message The message.
     * @return True if the message was handled, false otherwise.
     */
    bool TestOnMessage(const std::string& message);

   protected:
    /**
     * @brief Initializes the plugin.
     * @param settings The settings.
     * @return True if the plugin was initialized, false otherwise.
     */
    bool OnInitialize(const std::unordered_map<std::string, std::string>& settings) override;

    /**
     * @brief Starts the plugin.
     * @return True if the plugin was started, false otherwise.
     */
    bool OnStart() override;

    /**
     * @brief Stops the plugin.
     * @return True if the plugin was stopped, false otherwise.
     */
    bool OnStop() override;

    /**
     * @brief Called when a message is received.
     * @param message The message.
     * @return True if the message was handled, false otherwise.
     */
    virtual bool OnMessage(const std::string& message);
};

/**
 * @brief Represents a test plugin factory.
 */
class TestPluginFactory : public PluginFactoryBase<TestPlugin>
{
};
}  // namespace neo::plugins
