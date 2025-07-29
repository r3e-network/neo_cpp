#pragma once

#include <neo/plugins/plugin_base.h>
#include <string>

namespace neo
{
namespace plugins
{
namespace applicationlogs
{

class ApplicationLogsPlugin : public PluginBase
{
  public:
    ApplicationLogsPlugin(const std::string& path);
    ~ApplicationLogsPlugin() override;

  protected:
    bool OnStart() override;
    bool OnStop() override;

  private:
    std::string path_;
};

}  // namespace applicationlogs
}  // namespace plugins
}  // namespace neo
