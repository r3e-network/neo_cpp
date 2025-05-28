#pragma once

#include <neo/core/plugin.h>
#include <string>

namespace neo {
namespace plugins {
namespace applicationlogs {

class ApplicationLogsPlugin : public core::Plugin {
public:
    ApplicationLogsPlugin(const std::string& path);
    ~ApplicationLogsPlugin() override;

    void OnStart() override;
    void OnStop() override;

private:
    std::string path_;
};

} // namespace applicationlogs
} // namespace plugins
} // namespace neo
