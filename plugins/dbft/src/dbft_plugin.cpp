#include <iostream>
#include <neo/plugins/dbft/dbft_plugin.h>

namespace neo
{
namespace plugins
{
namespace dbft
{

DbftPlugin::DbftPlugin() {}

DbftPlugin::~DbftPlugin() {}

void DbftPlugin::OnStart()
{
    std::cout << "DbftPlugin started" << std::endl;
}

void DbftPlugin::OnStop()
{
    std::cout << "DbftPlugin stopped" << std::endl;
}

}  // namespace dbft
}  // namespace plugins
}  // namespace neo
