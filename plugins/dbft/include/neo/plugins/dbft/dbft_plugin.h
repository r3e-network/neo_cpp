#pragma once

#include <neo/core/plugin.h>

namespace neo
{
namespace plugins
{
namespace dbft
{

class DbftPlugin : public core::Plugin
{
  public:
    DbftPlugin();
    ~DbftPlugin() override;

    void OnStart() override;
    void OnStop() override;
};

}  // namespace dbft
}  // namespace plugins
}  // namespace neo
