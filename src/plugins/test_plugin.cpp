#include <iostream>
#include <neo/plugins/test_plugin.h>

namespace neo::plugins
{
TestPlugin::TestPlugin() : PluginBase("Test", "A test plugin", "1.0", "Neo C++ Team") {}

bool TestPlugin::OnInitialize(const std::unordered_map<std::string, std::string>& settings)
{
    std::cout << "TestPlugin initialized" << std::endl;
    return true;
}

bool TestPlugin::OnStart()
{
    std::cout << "TestPlugin started" << std::endl;
    return true;
}

bool TestPlugin::OnStop()
{
    std::cout << "TestPlugin stopped" << std::endl;
    return true;
}

void TestPlugin::LogMessage(const std::string& message)
{
    std::cout << "TestPlugin: " << message << std::endl;
}

bool TestPlugin::TestOnMessage(const std::string& message)
{
    return OnMessage(message);
}

bool TestPlugin::OnMessage(const std::string& message)
{
    std::cout << "TestPlugin received message: " << message << std::endl;
    return true;
}
}  // namespace neo::plugins
