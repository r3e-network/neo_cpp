#include <neo/plugins/applicationlogs/applicationlogs_plugin.h>
#include <iostream>

namespace neo {
namespace plugins {
namespace applicationlogs {

ApplicationLogsPlugin::ApplicationLogsPlugin(const std::string& path)
    : PluginBase("ApplicationLogs", "Plugin for application logs", "1.0.0", "Neo Team"), path_(path) {
}

ApplicationLogsPlugin::~ApplicationLogsPlugin() {
}

bool ApplicationLogsPlugin::OnStart() {
    std::cout << "ApplicationLogsPlugin started with path: " << path_ << std::endl;
    return true;
}

bool ApplicationLogsPlugin::OnStop() {
    std::cout << "ApplicationLogsPlugin stopped" << std::endl;
    return true;
}

} // namespace applicationlogs
} // namespace plugins
} // namespace neo
