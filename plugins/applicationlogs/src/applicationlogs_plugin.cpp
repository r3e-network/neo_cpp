#include <neo/plugins/applicationlogs/applicationlogs_plugin.h>
#include <iostream>

namespace neo {
namespace plugins {
namespace applicationlogs {

ApplicationLogsPlugin::ApplicationLogsPlugin(const std::string& path)
    : path_(path) {
}

ApplicationLogsPlugin::~ApplicationLogsPlugin() {
}

void ApplicationLogsPlugin::OnStart() {
    std::cout << "ApplicationLogsPlugin started with path: " << path_ << std::endl;
}

void ApplicationLogsPlugin::OnStop() {
    std::cout << "ApplicationLogsPlugin stopped" << std::endl;
}

} // namespace applicationlogs
} // namespace plugins
} // namespace neo
